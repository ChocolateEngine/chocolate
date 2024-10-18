#include "core/platform.h"
#include "core/log.h"
#include "util.h"

#include "render_vk.h"

#include "imgui_impl_vulkan.h"

#include <thread>
#include <mutex>

#include "imgui.h"
#include "imgui_impl_vulkan.h"


VkCommandBuffer                 gSingleCommandBuffer;
VkCommandBuffer                 gSingleCommandBufferTransfer;

ResourceList< VkCommandBuffer > gCommandBuffers;

bool                            gInTransferQueue = false;
bool                            gInGraphicsQueue = false;

std::mutex                      gGraphicsMutex;


VkCommandBuffer                 VK_GetCommandBuffer( ch_handle_t cmd )
{
	return *gCommandBuffers.Get( cmd );
}


void VK_CheckFenceStatus( WindowVK* window )
{
	// VK_WaitForPresentQueue();

	VkResult result = VK_SUCCESS;

	result          = vkGetFenceStatus( VK_GetDevice(), window->fences[ 0 ] );
	CH_ASSERT( result != VK_ERROR_DEVICE_LOST );

	if ( result == VK_ERROR_DEVICE_LOST )
		Log_Fatal( "fence 0 status device lost\n" );

	result = vkGetFenceStatus( VK_GetDevice(), window->fences[ 1 ] );
	CH_ASSERT( result != VK_ERROR_DEVICE_LOST );

	if ( result == VK_ERROR_DEVICE_LOST )
		Log_Fatal( "fence 1 status device lost\n" );

	VK_CheckResult( vkDeviceWaitIdle( VK_GetDevice() ), "Failed waiting for device on fence status" );
}


bool VK_CreateFences( WindowVK* window )
{
	window->fences         = ch_malloc< VkFence >( window->swapImageCount );
	window->fencesInFlight = ch_malloc< VkFence >( window->swapImageCount );

	VkFenceCreateInfo info{ VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
	info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	for ( u32 i = 0; i < window->swapImageCount; i++ )
	{
		if ( !VK_CheckResultE( vkCreateFence( VK_GetDevice(), &info, nullptr, &window->fences[ i ] ), "Failed to create fence!" ) )
			return false;
	}

	return true;
}


void VK_DestroyFences( WindowVK* window )
{
	if ( !window->fences )
		return;

	for ( u8 i = 0; i < window->swapImageCount; i++ )
	{
		vkDestroyFence( VK_GetDevice(), window->fences[ i ], nullptr );
	}

	free( window->fences );
	free( window->fencesInFlight );
}


bool VK_CreateSemaphores( WindowVK* window )
{
	window->imageAvailableSemaphores = ch_calloc< VkSemaphore >( window->swapImageCount );
	window->renderFinishedSemaphores = ch_calloc< VkSemaphore >( window->swapImageCount );

	VkSemaphoreCreateInfo info{ VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };

	for ( u32 i = 0; i < window->swapImageCount; i++ )
	{
		if ( !VK_CheckResultE( vkCreateSemaphore( VK_GetDevice(), &info, nullptr, &window->imageAvailableSemaphores[ i ] ), "Failed to create image semaphore!" ) )
			return false;

		if ( !VK_CheckResultE( vkCreateSemaphore( VK_GetDevice(), &info, nullptr, &window->renderFinishedSemaphores[ i ] ), "Failed to create render semaphore!" ) )
			return false;
	}

	return true;
}


void VK_DestroySemaphores( WindowVK* window )
{
	if ( window->imageAvailableSemaphores )
	{
		for ( u8 i = 0; i < window->swapImageCount; i++ )
		{
			vkDestroySemaphore( VK_GetDevice(), window->imageAvailableSemaphores[ i ], nullptr );
		}

		free( window->imageAvailableSemaphores );
	}

	if ( window->renderFinishedSemaphores )
	{
		for ( u8 i = 0; i < window->swapImageCount; i++ )
		{
			vkDestroySemaphore( VK_GetDevice(), window->renderFinishedSemaphores[ i ], nullptr );
		}

		free( window->renderFinishedSemaphores );
	}
}


bool VK_AllocateCommands( WindowVK* window )
{
	window->commandBuffers = CH_MALLOC( VkCommandBuffer, window->swapImageCount );

	if ( window->commandBuffers == nullptr )
		return false;

	window->commandBufferHandles = CH_MALLOC( ch_handle_t, window->swapImageCount );

	if ( window->commandBufferHandles == nullptr )
		return false;

	// Allocate primary command buffers
	VkCommandBufferAllocateInfo primAlloc{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
	primAlloc.pNext              = nullptr;
	primAlloc.commandPool        = VK_GetPrimaryCommandPool();
	primAlloc.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	primAlloc.commandBufferCount = window->swapImageCount;

	if ( !VK_CheckResultE( vkAllocateCommandBuffers( VK_GetDevice(), &primAlloc, window->commandBuffers ), "Failed to allocate primary command buffers" ) )
	{
		return false;
	}

	// create handles for these, fun
	for ( u32 i = 0; i < window->swapImageCount; i++ )
	{
		window->commandBufferHandles[ i ] = gCommandBuffers.Add( window->commandBuffers[ i ] );
	}

	return true;
}


void VK_FreeCommands( WindowVK* window )
{
	if ( !window->commandBuffers )
		return;

	for ( u32 i = 0; i < window->swapImageCount; i++ )
	{
		gCommandBuffers.Remove( window->commandBufferHandles[ i ] );
		vkFreeCommandBuffers( VK_GetDevice(), VK_GetPrimaryCommandPool(), 1, &window->commandBuffers[ i ] );
	}

	free( window->commandBuffers );
	free( window->commandBufferHandles );
}


void VK_AllocateOneTimeCommands()
{
	// Allocate one time command buffer
	{
		VkCommandBufferAllocateInfo aCommandBufferAllocateInfo{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
		aCommandBufferAllocateInfo.pNext              = nullptr;
		aCommandBufferAllocateInfo.commandPool        = VK_GetSingleTimeCommandPool();
		aCommandBufferAllocateInfo.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		aCommandBufferAllocateInfo.commandBufferCount = 1;

		VK_CheckResult( vkAllocateCommandBuffers( VK_GetDevice(), &aCommandBufferAllocateInfo, &gSingleCommandBuffer ), "Failed to allocate command buffer!" );
	}

	// Allocate one time transfer command buffer
	{
		VkCommandBufferAllocateInfo aCommandBufferAllocateInfo{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
		aCommandBufferAllocateInfo.pNext              = nullptr;
		aCommandBufferAllocateInfo.commandPool        = VK_GetTransferCommandPool();
		aCommandBufferAllocateInfo.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		aCommandBufferAllocateInfo.commandBufferCount = 1;

		VK_CheckResult( vkAllocateCommandBuffers( VK_GetDevice(), &aCommandBufferAllocateInfo, &gSingleCommandBufferTransfer ), "Failed to allocate command buffer!" );
	}
}


void VK_FreeOneTimeCommands()
{
	if ( gSingleCommandBuffer )
		vkFreeCommandBuffers( VK_GetDevice(), VK_GetSingleTimeCommandPool(), 1, &gSingleCommandBuffer );

	if ( gSingleCommandBufferTransfer )
		vkFreeCommandBuffers( VK_GetDevice(), VK_GetTransferCommandPool(), 1, &gSingleCommandBufferTransfer );
}


VkCommandBuffer VK_BeginOneTimeTransferCommand()
{
	VkCommandBufferBeginInfo aCommandBufferBeginInfo{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
	aCommandBufferBeginInfo.pNext = nullptr;
	aCommandBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	VK_CheckResult( vkBeginCommandBuffer( gSingleCommandBufferTransfer, &aCommandBufferBeginInfo ), "Failed to begin command buffer!" );

	return gSingleCommandBufferTransfer;
}


void VK_EndOneTimeTransferCommand( VkCommandBuffer c )
{
	PROF_SCOPE();

	VK_CheckResult( vkEndCommandBuffer( c ), "Failed to end command buffer!" );

	VkSubmitInfo submitInfo{ VK_STRUCTURE_TYPE_SUBMIT_INFO };
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers    = &c;

	VK_WaitForTransferQueue();
	gGraphicsMutex.lock();

	VK_CheckResult( vkQueueSubmit( VK_GetTransferQueue(), 1, &submitInfo, VK_NULL_HANDLE ), "Failed to Submit OneTimeTransferCommand" );
	gInTransferQueue = true;

	VK_WaitForTransferQueue();
	gGraphicsMutex.unlock();

	VK_ResetCommandPool( VK_GetTransferCommandPool() );
}


VkCommandBuffer VK_BeginOneTimeCommand()
{
	VkCommandBufferBeginInfo aCommandBufferBeginInfo{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
	aCommandBufferBeginInfo.pNext = nullptr;
	aCommandBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	VK_CheckResult( vkBeginCommandBuffer( gSingleCommandBuffer, &aCommandBufferBeginInfo ), "Failed to begin command buffer!" );

	return gSingleCommandBuffer;
}


void VK_EndOneTimeCommand( VkCommandBuffer c )
{
	PROF_SCOPE();

	VK_CheckResult( vkEndCommandBuffer( c ), "Failed to end command buffer!" );

	VkSubmitInfo submitInfo{ VK_STRUCTURE_TYPE_SUBMIT_INFO };
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers    = &c;

	VK_WaitForGraphicsQueue();
	gGraphicsMutex.lock();

	VK_CheckResult( vkQueueSubmit( VK_GetGraphicsQueue(), 1, &submitInfo, VK_NULL_HANDLE ), "Failed to Submit OneTimeCommand" );
	gInGraphicsQueue = true;

	VK_WaitForGraphicsQueue();
	gGraphicsMutex.unlock();

	VK_ResetCommandPool( VK_GetSingleTimeCommandPool() );
}


// legacy?
void VK_OneTimeCommand( std::function< void( VkCommandBuffer ) > sFunc )
{
	VkCommandBuffer c = VK_BeginOneTimeCommand();
	sFunc( c );
	VK_EndOneTimeCommand( c );
}


VkCommandBuffer VK_BeginCommandBuffer( CommandBufferGroup_t& srGroup )
{
	return VK_NULL_HANDLE;
}


void VK_EndCommandBuffer( VkCommandBuffer sCmd )
{
}


void VK_ResetCommandBuffers( CommandBufferGroup_t& srGroup )
{
}


void VK_WaitForTransferQueue()
{
	PROF_SCOPE();

	if ( gInTransferQueue )
		VK_CheckResult( vkQueueWaitIdle( VK_GetTransferQueue() ), "Failed waiting for transfer queue" );

	gInTransferQueue = false;
}


void VK_WaitForGraphicsQueue()
{
	PROF_SCOPE();

	if ( gInGraphicsQueue )
		VK_CheckResult( vkQueueWaitIdle( VK_GetGraphicsQueue() ), "Failed waiting for graphics queue" );

	gInGraphicsQueue = false;
}


void VK_RecordCommands()
{
#if 0
	gGraphicsMutex.lock();

	VK_WaitForPresentQueue();
	VK_WaitForGraphicsQueue();
	VK_ResetCommandPool( VK_GetPrimaryCommandPool() );

	// For each framebuffer, begin a primary
    // command buffer, and record the commands.
	for ( gCmdIndex = 0; gCmdIndex < gCommandBuffers.size(); gCmdIndex++ )
	{
		VkCommandBufferBeginInfo begin{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
		begin.pNext            = nullptr;
		begin.flags            = 0;
		begin.pInheritanceInfo = nullptr;

		VK_CheckResult( vkBeginCommandBuffer( gCommandBuffers[ gCmdIndex ], &begin ), "Failed to begin recording command buffer!" );

		// Run Filter if needed
		// VK_RunFilterShader();

		VkRenderPassBeginInfo renderPassBeginInfo{ VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO };
		renderPassBeginInfo.pNext             = nullptr;
		renderPassBeginInfo.renderPass        = VK_GetRenderPass();
		renderPassBeginInfo.framebuffer       = VK_GetBackBuffer()->aFrameBuffers[ gCmdIndex ];
		renderPassBeginInfo.renderArea.offset = { 0, 0 };
		renderPassBeginInfo.renderArea.extent = VK_GetSwapExtent();

		VkClearValue clearValues[ 2 ];
		clearValues[ 0 ].color              = { gClearR, gClearG, gClearB, 1.0f };
		clearValues[ 1 ].depthStencil       = { 1.0f, 0 };

		renderPassBeginInfo.clearValueCount = ARR_SIZE( clearValues );
		renderPassBeginInfo.pClearValues    = clearValues;

		vkCmdBeginRenderPass( gCommandBuffers[ gCmdIndex ], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE );

		// Render Image
		// VK_BindImageShader();
		// VK_DrawImageShader();

		// Render ImGui
		ImGui::Render();
		ImGui_ImplVulkan_RenderDrawData( ImGui::GetDrawData(), gCommandBuffers[ gCmdIndex ] );

		vkCmdEndRenderPass( gCommandBuffers[ gCmdIndex ] );

		VK_CheckResult( vkEndCommandBuffer( gCommandBuffers[ gCmdIndex ] ), "Failed to end recording command buffer!" );
	}
#endif
}


u32 VK_GetNextImage( ch_handle_t windowHandle, WindowVK* window )
{
	PROF_SCOPE();

	// are we able to move this wait for fences and acquire next image to the start of VK_Present,
	// that way we can record the command buffer while the gpu is still working on the previous frame?
	// this waits for the gpu to finish rendering the last frame
	VK_CheckResult( vkWaitForFences( VK_GetDevice(), 1, &window->fences[ window->frameIndex ], VK_TRUE, UINT64_MAX ), "Failed waiting for fences" );

	u32      imageIndex = UINT32_MAX;
	VkResult res = vkAcquireNextImageKHR( VK_GetDevice(), window->swapchain, UINT64_MAX, window->imageAvailableSemaphores[ window->frameIndex ], VK_NULL_HANDLE, &imageIndex );

	if ( res == VK_ERROR_OUT_OF_DATE_KHR )
	{
		// Recreate all resources.
		printf( "VK_Reset - vkAcquireNextImageKHR\n" );
		VK_Reset( windowHandle, window );

		return VK_GetNextImage( windowHandle, window );
	}

	else if ( res != VK_SUCCESS && res != VK_SUBOPTIMAL_KHR )
	{
		// Classic typo must remain.
		VK_CheckResult( res, "Failed ot acquire swapchain image!" );
		return UINT_MAX;
	}

	return imageIndex;
}


void VK_Present( ch_handle_t windowHandle, WindowVK* window, u32 sImageIndex )
{
	PROF_SCOPE();

	window->fencesInFlight[ sImageIndex ]   = window->fences[ window->frameIndex ];

	VkSemaphore          waitSemaphores[]   = { window->imageAvailableSemaphores[ window->frameIndex ] };
	VkSemaphore          signalSemaphores[] = { window->renderFinishedSemaphores[ window->frameIndex ] };
	VkPipelineStageFlags waitStages[]       = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	VkSubmitInfo         submitInfo{ VK_STRUCTURE_TYPE_SUBMIT_INFO };

	VkCommandBuffer      cmdBuffer  = window->commandBuffers[ sImageIndex ];

	submitInfo.waitSemaphoreCount   = ARR_SIZE( waitSemaphores );
	submitInfo.pWaitSemaphores      = waitSemaphores;  // wait for the image to be available from vkAcquireNextImageKHR
	submitInfo.pWaitDstStageMask    = waitStages;
	submitInfo.commandBufferCount   = 1;
	submitInfo.pCommandBuffers      = &cmdBuffer;  // could i render the command buffers for all windows in one go here?
	submitInfo.signalSemaphoreCount = ARR_SIZE( signalSemaphores );
	submitInfo.pSignalSemaphores    = signalSemaphores;  // signal to the gpu that we have finished executing the command buffer

	VK_CheckResult( vkResetFences( VK_GetDevice(), 1, &window->fences[ window->frameIndex ] ), "Failed to Reset Fences" );

	VK_CheckResult( vkQueueSubmit( VK_GetGraphicsQueue(), 1, &submitInfo, window->fences[ window->frameIndex ] ), "Failed to submit draw command buffer!" );

	VkSwapchainKHR   swapChains[] = { window->swapchain };

	VkPresentInfoKHR presentInfo{ VK_STRUCTURE_TYPE_PRESENT_INFO_KHR };
	presentInfo.pNext              = nullptr;
	presentInfo.waitSemaphoreCount = ARR_SIZE( signalSemaphores );
	presentInfo.pWaitSemaphores    = signalSemaphores;  // wait for the command buffer to be finished executing
	presentInfo.swapchainCount     = 1;
	presentInfo.pSwapchains        = swapChains;  // could i render the swapchains for all windows in one go here?
	presentInfo.pImageIndices      = &sImageIndex;
	presentInfo.pResults           = nullptr;

	// now present the image we just rendered to the screen
	VkResult res                   = vkQueuePresentKHR( VK_GetGraphicsQueue(), &presentInfo );

	if ( res == VK_ERROR_OUT_OF_DATE_KHR || res == VK_SUBOPTIMAL_KHR )
	{
		printf( "VK_Reset - vkQueuePresentKHR\n" );
		VK_CheckResult( vkDeviceWaitIdle( VK_GetDevice() ), "Failed to wait for device to be idle" );
		VK_Reset( windowHandle, window );
	}
	else if ( res != VK_SUCCESS && res != VK_SUBOPTIMAL_KHR )
	{
		VK_CheckResult( res, "Failed to present swapchain image!" );
	}

	// gGraphicsMutex.unlock();

	gInGraphicsQueue = true;
	window->frameIndex = ( window->frameIndex + 1 ) % window->swapImageCount;
}

