#include "render.h"


CONVAR_BOOL_NAME_CMD( r_msaa_enabled, "vk.msaa.enabled", false, CVARF_ARCHIVE, "Enable/Disable MSAA Globally" )
{
	vk_reset_all( e_render_reset_flags_msaa );
}


CONVAR_INT_NAME_CMD( r_msaa_samples, "vk.msaa.samples", 4, CVARF_ARCHIVE, "Set the Default Amount of MSAA Samples Globally" )
{
	if ( !r_msaa_enabled )
		return;

	vk_reset_all( e_render_reset_flags_msaa );
}


CONVAR_INT_NAME_CMD( r_msaa_textures, "vk.msaa.textures", 0, CVARF_ARCHIVE, "Allow MSAA to be applied to textures (sample shading), Very Expensive!" )
{
	if ( !r_msaa_enabled )
		return;

	vk_reset_all( e_render_reset_flags_msaa );
}


CONVAR_FLOAT_NAME_CMD( r_msaa_textures_min, "vk.msaa.textures.min", 1.f, CVARF_ARCHIVE, "Minimum Sample Shading amount" )
{
	if ( !r_msaa_enabled )
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


VkSampleCountFlags vk_msaa_get_max_samples()
{
	return g_vk_device_properties.limits.framebufferColorSampleCounts & g_vk_device_properties.limits.framebufferDepthSampleCounts;
}


#if 0
VkSampleCountFlagBits vk_msaa_find_max_samples()
{
	VkSampleCountFlags counts = g_vk_device_properties.limits.framebufferColorSampleCounts & g_vk_device_properties.limits.framebufferDepthSampleCounts;
	if ( counts & VK_SAMPLE_COUNT_64_BIT ) return VK_SAMPLE_COUNT_64_BIT;
	if ( counts & VK_SAMPLE_COUNT_32_BIT ) return VK_SAMPLE_COUNT_32_BIT;
	if ( counts & VK_SAMPLE_COUNT_16_BIT ) return VK_SAMPLE_COUNT_16_BIT;
	if ( counts & VK_SAMPLE_COUNT_8_BIT ) return VK_SAMPLE_COUNT_8_BIT;
	if ( counts & VK_SAMPLE_COUNT_4_BIT ) return VK_SAMPLE_COUNT_4_BIT;
	if ( counts & VK_SAMPLE_COUNT_2_BIT ) return VK_SAMPLE_COUNT_2_BIT;

	return VK_SAMPLE_COUNT_1_BIT;
}
#endif


VkSampleCountFlagBits vk_msaa_get_samples()
{
	if ( !r_msaa_enabled )
		return VK_SAMPLE_COUNT_1_BIT;

	VkSampleCountFlags max_samples = vk_msaa_get_max_samples();

	if ( r_msaa_samples >= 64 && max_samples & VK_SAMPLE_COUNT_64_BIT )
		return VK_SAMPLE_COUNT_64_BIT;

	if ( r_msaa_samples >= 32 && max_samples & VK_SAMPLE_COUNT_32_BIT )
		return VK_SAMPLE_COUNT_32_BIT;

	if ( r_msaa_samples >= 16 && max_samples & VK_SAMPLE_COUNT_16_BIT )
		return VK_SAMPLE_COUNT_16_BIT;

	if ( r_msaa_samples >= 8 && max_samples & VK_SAMPLE_COUNT_8_BIT )
		return VK_SAMPLE_COUNT_8_BIT;

	if ( r_msaa_samples >= 4 && max_samples & VK_SAMPLE_COUNT_4_BIT )
		return VK_SAMPLE_COUNT_4_BIT;

	if ( r_msaa_samples >= 2 && max_samples & VK_SAMPLE_COUNT_2_BIT )
		return VK_SAMPLE_COUNT_2_BIT;

	Log_Warn( gLC_Render, "Minimum Sample Count with MSAA on is 2!\n" );
	return VK_SAMPLE_COUNT_2_BIT;
}


VkSampleCountFlagBits vk_msaa_get_samples( bool use_msaa )
{
	if ( !use_msaa || !r_msaa_enabled )
		return VK_SAMPLE_COUNT_1_BIT;

	VkSampleCountFlags max_samples = vk_msaa_get_max_samples();

	if ( r_msaa_samples >= 64 && max_samples & VK_SAMPLE_COUNT_64_BIT )
		return VK_SAMPLE_COUNT_64_BIT;

	if ( r_msaa_samples >= 32 && max_samples & VK_SAMPLE_COUNT_32_BIT )
		return VK_SAMPLE_COUNT_32_BIT;

	if ( r_msaa_samples >= 16 && max_samples & VK_SAMPLE_COUNT_16_BIT )
		return VK_SAMPLE_COUNT_16_BIT;

	if ( r_msaa_samples >= 8 && max_samples & VK_SAMPLE_COUNT_8_BIT )
		return VK_SAMPLE_COUNT_8_BIT;

	if ( r_msaa_samples >= 4 && max_samples & VK_SAMPLE_COUNT_4_BIT )
		return VK_SAMPLE_COUNT_4_BIT;

	if ( r_msaa_samples >= 2 && max_samples & VK_SAMPLE_COUNT_2_BIT )
		return VK_SAMPLE_COUNT_2_BIT;

	Log_Warn( gLC_Render, "Minimum Sample Count with MSAA on is 2!\n" );
	return VK_SAMPLE_COUNT_2_BIT;
}


bool vk_backbuffer_create_depth( r_window_data_t* window )
{
	VkImageCreateInfo image_info{ VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
	image_info.imageType     = VK_IMAGE_TYPE_2D;
	image_info.mipLevels     = 1;
	image_info.arrayLayers   = 1;

	// hardcoding the depth format here
	image_info.format        = g_depth_format;

	// always match window size
	image_info.extent.depth  = 1;

	// image_info.extent.width  = window->swap_extent.width;
	// image_info.extent.height = window->swap_extent.height;

	image_info.extent.width  = std::max( 1.f, window->swap_extent.width * vk_render_scale );
	image_info.extent.height = std::max( 1.f, window->swap_extent.height * vk_render_scale );

	// This is the depth buffer, so no MSAA here
	image_info.samples       = vk_msaa_get_samples();

	// optimal tiling, which means the image is stored on the best gpu format
	image_info.tiling        = VK_IMAGE_TILING_OPTIMAL;

	// has both transfers so we can copy from and into the image, storage to be used in compute shaders, and color attachment for rendering
	image_info.usage         = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;

	// allocate from gpu memory
	VmaAllocationCreateInfo alloc_info{};
	alloc_info.usage         = VMA_MEMORY_USAGE_GPU_ONLY;
	alloc_info.requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

	// create the image
	if ( vk_check_e( vmaCreateImage( g_vma, &image_info, &alloc_info, &window->draw_image_depth.image, &window->draw_image_depth.memory, nullptr ), "Failed to create depth image for window!" ) )
		return false;

	// build a image-view for the draw image to use for rendering
	VkImageViewCreateInfo view_info{ VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };

	view_info.viewType                        = VK_IMAGE_VIEW_TYPE_2D;
	view_info.image                           = window->draw_image_depth.image;
	view_info.format                          = image_info.format;
	view_info.subresourceRange.baseMipLevel   = 0;
	view_info.subresourceRange.levelCount     = 1;
	view_info.subresourceRange.baseArrayLayer = 0;
	view_info.subresourceRange.layerCount     = 1;
	view_info.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_DEPTH_BIT;

	if ( vk_check_e( vkCreateImageView( g_vk_device, &view_info, nullptr, &window->draw_image_depth.view ), "Failed to create depth image view for window!" ) )
		return false;

	window->draw_image_depth.extent.width  = image_info.extent.width;
	window->draw_image_depth.extent.height = image_info.extent.height;
	window->draw_image_depth.format        = image_info.format;

	vk_set_name( VK_OBJECT_TYPE_IMAGE, (u64)window->draw_image_depth.image, "Backbuffer - Depth" );
	vk_set_name( VK_OBJECT_TYPE_IMAGE_VIEW, (u64)window->draw_image_depth.view, "Backbuffer - Depth" );

	return true;
}


bool vk_backbuffer_create_color( r_window_data_t* window )
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

	image_info.samples       = vk_msaa_get_samples();

	// optimal tiling, which means the image is stored on the best gpu format
	image_info.tiling        = VK_IMAGE_TILING_OPTIMAL;

	// has both transfers so we can copy from and into the image, storage to be used in compute shaders, and color attachment for rendering
	// NOTE: you can only use VK_IMAGE_USAGE_STORAGE_BIT with MSAA if the shaderStorageImageMultisample feature is enabled
	// image_info.usage         = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	image_info.usage         = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

	// allocate from gpu memory
	VmaAllocationCreateInfo alloc_info{};
	alloc_info.usage         = VMA_MEMORY_USAGE_GPU_ONLY;
	alloc_info.requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

	// create the image
	if ( vk_check_e( vmaCreateImage( g_vma, &image_info, &alloc_info, &window->draw_image.image, &window->draw_image.memory, nullptr ), "Failed to create color image for window!" ) )
		return false;

	// build a image-view for the draw image to use for rendering
	VkImageViewCreateInfo view_info{ VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };

	view_info.viewType                        = VK_IMAGE_VIEW_TYPE_2D;
	view_info.image                           = window->draw_image.image;
	view_info.format                          = image_info.format;
	view_info.subresourceRange.baseMipLevel   = 0;
	view_info.subresourceRange.levelCount     = 1;
	view_info.subresourceRange.baseArrayLayer = 0;
	view_info.subresourceRange.layerCount     = 1;
	view_info.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;

	if ( vk_check_e( vkCreateImageView( g_vk_device, &view_info, nullptr, &window->draw_image.view ), "Failed to create color image view for window!" ) )
		return false;

	window->draw_image.extent.width  = image_info.extent.width;
	window->draw_image.extent.height = image_info.extent.height;
	window->draw_image.format        = image_info.format;

	vk_set_name( VK_OBJECT_TYPE_IMAGE, (u64)window->draw_image.image, "Backbuffer - Color" );
	vk_set_name( VK_OBJECT_TYPE_IMAGE_VIEW, (u64)window->draw_image.view, "Backbuffer - Color" );

	return true;
}


bool vk_backbuffer_create_resolve( r_window_data_t* window )
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
	image_info.samples       = VK_SAMPLE_COUNT_1_BIT;

	// optimal tiling, which means the image is stored on the best gpu format
	image_info.tiling        = VK_IMAGE_TILING_OPTIMAL;

	// has both transfers so we can copy from and into the image, storage to be used in compute shaders, and color attachment for rendering
	image_info.usage         = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

	// allocate from gpu memory
	VmaAllocationCreateInfo alloc_info{};
	alloc_info.usage         = VMA_MEMORY_USAGE_GPU_ONLY;
	alloc_info.requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

	// create the image
	if ( vk_check_e( vmaCreateImage( g_vma, &image_info, &alloc_info, &window->draw_image_resolve.image, &window->draw_image_resolve.memory, nullptr ), "Failed to create resolve image for window!" ) )
		return false;

	// build a image-view for the draw image to use for rendering
	VkImageViewCreateInfo view_info{ VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };

	view_info.viewType                        = VK_IMAGE_VIEW_TYPE_2D;
	view_info.image                           = window->draw_image_resolve.image;
	view_info.format                          = image_info.format;
	view_info.subresourceRange.baseMipLevel   = 0;
	view_info.subresourceRange.levelCount     = 1;
	view_info.subresourceRange.baseArrayLayer = 0;
	view_info.subresourceRange.layerCount     = 1;
	view_info.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;

	if ( vk_check_e( vkCreateImageView( g_vk_device, &view_info, nullptr, &window->draw_image_resolve.view ), "Failed to create resolve image view for window!" ) )
		return false;

	window->draw_image_resolve.extent.width  = image_info.extent.width;
	window->draw_image_resolve.extent.height = image_info.extent.height;
	window->draw_image_resolve.format        = image_info.format;

	vk_set_name( VK_OBJECT_TYPE_IMAGE, (u64)window->draw_image_resolve.image, "Backbuffer - Resolve" );
	vk_set_name( VK_OBJECT_TYPE_IMAGE_VIEW, (u64)window->draw_image_resolve.view, "Backbuffer - Resolve" );

	return true;
}


bool vk_backbuffer_create( r_window_data_t* window )
{
#if 1

	if ( !vk_backbuffer_create_depth( window ) )
		return false;

	if ( !vk_backbuffer_create_color( window ) )
		return false;

	if ( r_msaa_enabled )
	{
		if ( !vk_backbuffer_create_resolve( window ) )
				return false;
	}

	return true;

#else
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
#endif
}


void vk_backbuffer_destroy( r_window_data_t* window )
{
	// Destroy Color Image
	if ( window->draw_image.view != VK_NULL_HANDLE )
		vkDestroyImageView( g_vk_device, window->draw_image.view, nullptr );

	if ( window->draw_image.image != VK_NULL_HANDLE )
		vmaDestroyImage( g_vma, window->draw_image.image, window->draw_image.memory );

	// Destroy Depth Image
	if ( window->draw_image_depth.view != VK_NULL_HANDLE )
		vkDestroyImageView( g_vk_device, window->draw_image_depth.view, nullptr );

	if ( window->draw_image_depth.image != VK_NULL_HANDLE )
		vmaDestroyImage( g_vma, window->draw_image_depth.image, window->draw_image_depth.memory );

	// Destroy Resolve Image
	if ( window->draw_image_resolve.view != VK_NULL_HANDLE )
		vkDestroyImageView( g_vk_device, window->draw_image_resolve.view, nullptr );

	if ( window->draw_image_resolve.image != VK_NULL_HANDLE )
		vmaDestroyImage( g_vma, window->draw_image_resolve.image, window->draw_image_resolve.memory );

	memset( &window->draw_image, 0, sizeof( vk_image_t ) );
	memset( &window->draw_image_depth, 0, sizeof( vk_image_t ) );
	memset( &window->draw_image_resolve, 0, sizeof( vk_image_t ) );
}

