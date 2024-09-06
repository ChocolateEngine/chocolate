#include "render.h"

#include "imgui.h"
#include "imgui_impl_vulkan.h"

bool vk_render_sync_create( r_window_data_t* window )
{
	VkFenceCreateInfo fence_info{ VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
	fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;  // we want to start signaled, so we dont just end up in a deadlock waiting on the gpu when the gpu isn't doing anything

	VkSemaphoreCreateInfo semaphore_info{ VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };

	window->fence_render        = ch_malloc< VkFence >( window->swap_image_count );
	window->semaphore_swapchain = ch_malloc< VkSemaphore >( window->swap_image_count );
	window->semaphore_render    = ch_malloc< VkSemaphore >( window->swap_image_count );
	
	for ( u32 i = 0; i < window->swap_image_count; ++i )
	{
		vk_check( vkCreateFence( g_vk_device, &fence_info, nullptr, &window->fence_render[ i ] ), "Failed to create render fence" );
		vk_check( vkCreateSemaphore( g_vk_device, &semaphore_info, nullptr, &window->semaphore_swapchain[ i ] ), "Failed to create swapchain semaphore" );
		vk_check( vkCreateSemaphore( g_vk_device, &semaphore_info, nullptr, &window->semaphore_render[ i ] ), "Failed to create render semaphore" );
	}

	return true;
}


void vk_render_sync_destroy( r_window_data_t* window )
{
	for ( u32 i = 0; i < window->swap_image_count; ++i )
	{
		if ( window->fence_render[ i ] != VK_NULL_HANDLE )
			vkDestroyFence( g_vk_device, window->fence_render[ i ], nullptr );

		if ( window->semaphore_swapchain[ i ] != VK_NULL_HANDLE )
			vkDestroySemaphore( g_vk_device, window->semaphore_swapchain[ i ], nullptr );

		if ( window->semaphore_render[ i ] != VK_NULL_HANDLE )
			vkDestroySemaphore( g_vk_device, window->semaphore_render[ i ], nullptr );
	}

	ch_free( window->fence_render );
	ch_free( window->semaphore_swapchain );
	ch_free( window->semaphore_render );

	window->fence_render        = nullptr;
	window->semaphore_swapchain = nullptr;
	window->semaphore_render    = nullptr;
}


void vk_render_sync_reset( r_window_data_t* window )
{
//	for ( u32 i = 0; i < window->swap_image_count; ++i )
//	{
//		vk_check( vkResetFences( g_vk_device, 1, &window->fence_render[ i ] ), "Failed to reset render fence" );
//	}
}


void vk_blit_image_to_image( VkCommandBuffer c, VkImage src, VkImage dst, VkExtent2D src_size, VkExtent2D dst_size )
{
	VkImageBlit2 blit_region{ VK_STRUCTURE_TYPE_IMAGE_BLIT_2 };

	// don't bother with offset 0, we are just going to copy the whole image
	blit_region.srcOffsets[ 1 ].x             = src_size.width;
	blit_region.srcOffsets[ 1 ].y             = src_size.height;
	blit_region.srcOffsets[ 1 ].z             = 1;

	blit_region.dstOffsets[ 1 ].x             = dst_size.width;
	blit_region.dstOffsets[ 1 ].y             = dst_size.height;
	blit_region.dstOffsets[ 1 ].z             = 1;

	blit_region.srcSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
	blit_region.srcSubresource.baseArrayLayer = 0;
	blit_region.srcSubresource.layerCount     = 1;
	blit_region.srcSubresource.mipLevel       = 0;

	blit_region.dstSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
	blit_region.dstSubresource.baseArrayLayer = 0;
	blit_region.dstSubresource.layerCount     = 1;
	blit_region.dstSubresource.mipLevel       = 0;

	VkBlitImageInfo2 blit_info{ VK_STRUCTURE_TYPE_BLIT_IMAGE_INFO_2 };
	blit_info.dstImage       = dst;
	blit_info.dstImageLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	blit_info.srcImage       = src;
	blit_info.srcImageLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
	blit_info.filter         = VK_FILTER_LINEAR;
	blit_info.regionCount    = 1;
	blit_info.pRegions       = &blit_region;

	vkCmdBlitImage2( c, &blit_info );
}


// TODO: move this elsewhere later
// https://vkguide.dev/docs/new_chapter_1/vulkan_mainloop_code/
// TODO: read these - https://github.com/KhronosGroup/Vulkan-Docs/wiki/Synchronization-Examples - https://gpuopen.com/learn/vulkan-barriers-explained/
void vk_transition_image( VkCommandBuffer c, VkImage image, VkImageLayout current_layout, VkImageLayout new_layout )
{
	VkImageMemoryBarrier2 barrier{ VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2 };

	barrier.image                           = image;

	barrier.srcStageMask                    = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;  // not optimal, this will stop all commands when it hits this barrier
	barrier.srcAccessMask                   = VK_ACCESS_2_MEMORY_WRITE_BIT;
	barrier.dstStageMask                    = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
	barrier.dstAccessMask                   = VK_ACCESS_2_MEMORY_WRITE_BIT | VK_ACCESS_2_MEMORY_READ_BIT;

	barrier.oldLayout                       = current_layout;
	barrier.newLayout                       = new_layout;

	barrier.subresourceRange.aspectMask     = ( new_layout == VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL ) ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;
	barrier.subresourceRange.baseMipLevel   = 0;
	barrier.subresourceRange.levelCount     = VK_REMAINING_MIP_LEVELS;  // transition all mips
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount     = VK_REMAINING_ARRAY_LAYERS;  // transition all layers

	VkDependencyInfoKHR dep_info{ VK_STRUCTURE_TYPE_DEPENDENCY_INFO_KHR };
	dep_info.imageMemoryBarrierCount = 1;
	dep_info.pImageMemoryBarriers    = &barrier;

	vkCmdPipelineBarrier2( c, &dep_info );
}


static u32 vk_get_next_image( ChHandle_t window_handle, r_window_data_t* window )
{
	u32 frame = window->frame_index;

	// wait for the last frame to finish rendering, timeout of 1 second, blocks the thread
	vk_check( vkWaitForFences( g_vk_device, 1, &window->fence_render[ frame ], VK_TRUE, 1000000000 ), "Failed to wait for render fence" );

	window->delete_queue.flush();

	vk_check( vkResetFences( g_vk_device, 1, &window->fence_render[ frame ] ), "Failed to reset render fence" );

	// request an image from the swapchain, blocks the thread
	// pass semaphore_swapchain to it so we can sync other operations with having an image ready to render
	u32      swapchain_index = 0;
	VkResult frame_result      = vkAcquireNextImageKHR( g_vk_device, window->swapchain, 1000000000, window->semaphore_swapchain[ frame ], VK_NULL_HANDLE, &swapchain_index );

	if ( frame_result == VK_ERROR_OUT_OF_DATE_KHR )
	{
		// Recreate all resources.
		printf( "Out of date swapchain! Must reset\n" );
		vk_reset( window_handle, window, e_render_reset_flags_resize );
		return vk_get_next_image( window_handle, window );

		return UINT32_MAX;
	}
	else if ( frame_result != VK_SUCCESS && frame_result != VK_SUBOPTIMAL_KHR )
	{
		// Classic typo must remain.
		vk_check( frame_result, "Failed ot acquire swapchain image!" );
		return UINT32_MAX;
	}

	return swapchain_index;
}


// TEMP
static u64 g_frame_number = 0;

static void vk_record_commands_window( r_window_data_t* window, u32 swap_index )
{
	u32                      frame = window->frame_index;

	VkCommandBufferBeginInfo begin_info{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
	begin_info.flags  = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;  // we are only going to submit this command buffer once

	VkCommandBuffer c = window->command_buffers[ frame ];

	vk_check( vkResetCommandBuffer( c, 0 ), "Failed to reset command buffer" );
	vk_check( vkBeginCommandBuffer( c, &begin_info ), "Failed to begin command buffer" );

	// we have to transition the draw image first
	// https://registry.khronos.org/vulkan/specs/1.3-extensions/html/chap12.html#resources-image-layouts
	vk_transition_image( c, window->draw_image.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL );

	// ---------------------------------------------------------------
	// start drawing

	// TEMP - cycled clear color, flash with 120 frame period
	VkClearColorValue clear_value;
	float             flash = std::abs( std::sin( g_frame_number / 120.f ) );
	clear_value             = { { 0.0f, 0.0f, flash, 1.0f } };
//
//	// tells us what part of the draw image to clear
//	VkImageSubresourceRange clear_range{ VK_IMAGE_ASPECT_COLOR_BIT, 0, VK_REMAINING_MIP_LEVELS, 0, VK_REMAINING_ARRAY_LAYERS };
//
//	// clear image
///	vkCmdClearColorImage( c, window->draw_image.image, VK_IMAGE_LAYOUT_GENERAL, &clear_value, 1, &clear_range );

	// bind the gradient drawing compute pipeline
	vkCmdBindPipeline( c, VK_PIPELINE_BIND_POINT_COMPUTE, g_pipeline_gradient );

	// bind the descriptor set containing the draw image for the compute pipeline
	vkCmdBindDescriptorSets( c, VK_PIPELINE_BIND_POINT_COMPUTE, g_pipeline_gradient_layout, 0, 1, &window->desc_draw_image, 0, nullptr );

	// execute the compute pipeline dispatch. We are using 16x16 workgroup size so we need to divide by it
	vkCmdDispatch( c, std::ceil( window->swap_extent.width / 16.0 ), std::ceil( window->swap_extent.height / 16.0 ), 1 );

	// ---------------------------------------------------------------
	// end drawing

	// transition the draw image and swap image to be ready for copying
	vk_transition_image( c, window->draw_image.image, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL );
	vk_transition_image( c, window->swap_images[ swap_index ], VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL );

	// copy the draw image into the swapchain
	vk_blit_image_to_image( c, window->draw_image.image, window->swap_images[ swap_index ], window->swap_extent, window->swap_extent );

	// imgui test
	if ( ImGui::GetDrawData() )
	{
		vk_transition_image( c, window->swap_images[ swap_index ], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL );

		VkClearValue clear{};
		clear.color = { 0.0f, 0.0f, 0.0f, 1.0f };

		VkRenderingAttachmentInfo color_attachment{ VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO };
		color_attachment.imageView   = window->swap_image_views[ swap_index ];
		color_attachment.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
		// color_attachment.loadOp      = clear ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD;
		// color_attachment.loadOp      = VK_ATTACHMENT_LOAD_OP_CLEAR;
		color_attachment.loadOp      = VK_ATTACHMENT_LOAD_OP_LOAD;
		color_attachment.storeOp     = VK_ATTACHMENT_STORE_OP_STORE;

		// if ( clear )
		{
		//	color_attachment.clearValue = clear;
		}

		VkRenderingInfo render_info{ VK_STRUCTURE_TYPE_RENDERING_INFO };
		render_info.colorAttachmentCount = 1;
		render_info.pColorAttachments    = &color_attachment;
		render_info.renderArea.extent    = window->swap_extent;
		render_info.layerCount           = 1;

		vkCmdBeginRendering( c, &render_info );

		ImGui_ImplVulkan_RenderDrawData( ImGui::GetDrawData(), c );

		vkCmdEndRendering( c );

		// make the swapchain presentable
		vk_transition_image( c, window->swap_images[ swap_index ], VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR );
	}
	else
	{
		// make the swapchain presentable
		vk_transition_image( c, window->swap_images[ swap_index ], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR );
	}

	// end the command buffer
	vk_check( vkEndCommandBuffer( c ), "Failed to end command buffer" );
}


static void vk_submit_command_buffer( r_window_data_t* window )
{
	u32                   frame = window->frame_index;

	// wait for the swapchain to be ready
	VkSemaphoreSubmitInfo semaphore_wait_info{ VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO };
	semaphore_wait_info.semaphore   = window->semaphore_swapchain[ frame ];
	semaphore_wait_info.stageMask   = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
	semaphore_wait_info.deviceIndex = 0;
	semaphore_wait_info.value       = 1;

	// signal that we are done rendering so we can present
	VkSemaphoreSubmitInfo semaphore_signal_info{ VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO };
	semaphore_signal_info.semaphore   = window->semaphore_render[ frame ];
	semaphore_signal_info.stageMask   = VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT;
	semaphore_signal_info.deviceIndex = 0;
	semaphore_signal_info.value       = 1;

	VkCommandBufferSubmitInfo cmd_submit_info{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO };
	cmd_submit_info.commandBuffer = window->command_buffers[ frame ];
	cmd_submit_info.deviceMask    = 0;

	VkSubmitInfo2 submit_info{ VK_STRUCTURE_TYPE_SUBMIT_INFO_2 };
	submit_info.waitSemaphoreInfoCount   = 1;
	submit_info.pWaitSemaphoreInfos      = &semaphore_wait_info;

	submit_info.signalSemaphoreInfoCount = 1;
	submit_info.pSignalSemaphoreInfos    = &semaphore_signal_info;

	submit_info.commandBufferInfoCount   = 1;
	submit_info.pCommandBufferInfos      = &cmd_submit_info;

	vk_check( vkQueueSubmit2( g_vk_queue_graphics, 1, &submit_info, window->fence_render[ frame ] ), "Failed to submit command buffer" );
}


void vk_draw( ChHandle_t window_handle, r_window_data_t* window )
{
	u32 frame = window->frame_index;

	// get next image
	u32 swapchain_index = vk_get_next_image( window_handle, window );

	if ( swapchain_index == UINT32_MAX )
	{
		// Recreate all resources.
		printf( "Out of date swapchain! Must reset\n" );
		return;
	}

	// record commands
	vk_record_commands_window( window, swapchain_index );

	// submit command buffer
	vk_submit_command_buffer( window );

	// present the window
	VkPresentInfoKHR present_info{ VK_STRUCTURE_TYPE_PRESENT_INFO_KHR };
	present_info.swapchainCount     = 1;
	present_info.pSwapchains        = &window->swapchain;
	present_info.pImageIndices      = &swapchain_index;

	// wait for the command buffer to finish executing before presenting
	present_info.waitSemaphoreCount = 1;
	present_info.pWaitSemaphores    = &window->semaphore_render[ frame ];

	vk_check( vkQueuePresentKHR( g_vk_queue_graphics, &present_info ), "Failed to present window" );

	g_frame_number++;
}


void vk_reset( ChHandle_t window_handle, r_window_data_t* window, e_render_reset_flags flags )
{
	// recreate swapchain
	vk_swapchain_recreate( window );

	// recreate draw image
	vk_backbuffer_destroy( window );

	if ( !vk_backbuffer_create( window ) )
	{
		Log_Error( "Failed to create draw image\n" );
		return;
	}

	// update window descriptors
	vk_descriptor_update_window( window );

	// call the window reset callback
	if ( window->fn_on_reset )
	{
		window->fn_on_reset( window_handle, flags );
	}
}

