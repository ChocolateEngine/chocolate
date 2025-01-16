#include "render.h"


CONVAR_BOOL_NAME_CMD( r_msaa, "vk.msaa.enabled", false, CVARF_ARCHIVE, "Enable/Disable MSAA Globally" )
{
	vk_reset_all( e_render_reset_flags_msaa );
}


CONVAR_INT_NAME_CMD( r_msaa_samples, "vk.msaa.samples", 4, CVARF_ARCHIVE, "Set the Default Amount of MSAA Samples Globally" )
{
	if ( !r_msaa )
		return;

	vk_reset_all( e_render_reset_flags_msaa );
}


static void render_scale_callback( float prev_value, float& new_value )
{
	vkDeviceWaitIdle( g_vk_device );

	// do this on all windows
	for ( u32 i = 0; i < g_windows.capacity; i++ )
	{
		if ( !g_windows.use_list[ i ] )
			continue;

		r_window_data_t& window_data = g_windows.data[ i ];
		window_data.need_resize      = true;
	}
}


CONVAR_RANGE_FLOAT_NAME( vk_render_scale, "vk.render.scale", 1.0, 0.01, 4.0, 0, "Scale the window backbuffer", &render_scale_callback );


static bool vk_backbuffer_create_depth( r_window_data_t* window )
{
	return false;
}


bool vk_backbuffer_create( r_window_data_t* window )
{
	VkImageCreateInfo image_info{ VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
	image_info.imageType     = VK_IMAGE_TYPE_2D;
	image_info.mipLevels     = 1;
	image_info.arrayLayers   = 1;

	// hardcoding the draw format to 32 bit float
	image_info.format        = g_draw_format;

	// always match window size
	image_info.extent.depth  = 1;

	// image_info.extent.width  = window->swap_extent.width;
	// image_info.extent.height = window->swap_extent.height;

	image_info.extent.width  = std::max( 1.f, window->swap_extent.width * vk_render_scale );
	image_info.extent.height = std::max( 1.f, window->swap_extent.height * vk_render_scale );

	// for MSAA. we will not be using it by default, so default it to 1 sample per pixel.
	image_info.samples     = VK_SAMPLE_COUNT_1_BIT;

	// optimal tiling, which means the image is stored on the best gpu format
	image_info.tiling        = VK_IMAGE_TILING_OPTIMAL;

	// has both transfers so we can copy from and into the image, storage to be used in compute shaders, and color attachment for rendering
	image_info.usage         = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

	// allocate from gpu memory
	VmaAllocationCreateInfo alloc_info{};
	alloc_info.usage         = VMA_MEMORY_USAGE_GPU_ONLY;
	alloc_info.requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

	// create the image
	if ( vk_check_e( vmaCreateImage( g_vma, &image_info, &alloc_info, &window->draw_image.image, &window->draw_image.memory, nullptr ), "Failed to create image for window!" ) )
		return false;

	// build a image-view for the draw image to use for rendering
	VkImageViewCreateInfo view_info{ VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };

	view_info.viewType                        = VK_IMAGE_VIEW_TYPE_2D;
	view_info.image                           = window->draw_image.image;
	view_info.format                          = g_draw_format;
	view_info.subresourceRange.baseMipLevel   = 0;
	view_info.subresourceRange.levelCount     = 1;
	view_info.subresourceRange.baseArrayLayer = 0;
	view_info.subresourceRange.layerCount     = 1;
	view_info.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;

	if ( vk_check_e( vkCreateImageView( g_vk_device, &view_info, nullptr, &window->draw_image.view ), "Failed to create image view for window!" ) )
		return false;

	window->draw_image.extent.width  = image_info.extent.width;
	window->draw_image.extent.height = image_info.extent.height;
	window->draw_image.format        = g_draw_format;

	return true;
}


void vk_backbuffer_destroy( r_window_data_t* window )
{
	if ( window->draw_image.view != VK_NULL_HANDLE )
		vkDestroyImageView( g_vk_device, window->draw_image.view, nullptr );

	if ( window->draw_image.image != VK_NULL_HANDLE )
		vmaDestroyImage( g_vma, window->draw_image.image, window->draw_image.memory );
}

