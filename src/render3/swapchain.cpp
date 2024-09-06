// --------------------------------------------------------------------------------------------
// Vulkan Swapchain
// --------------------------------------------------------------------------------------------

#include "render.h"


CONVAR_EX_RANGE_INT( r_vk_present_mode, "r.vk.present.mode", 0, 0, 3, "Image Present Mode, 0 - " );


// desired surface format
static VkFormat            g_color_format = VK_FORMAT_B8G8R8A8_SRGB;
static VkColorSpaceKHR     g_color_space  = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;

VkSurfaceCapabilitiesKHR   g_vk_surface_capabilities{};

static VkSurfaceFormatKHR* g_surf_formats;
static u32                 g_surf_format_count;

static VkPresentModeKHR*   g_present_modes;
static u32                 g_present_mode_count;

VkSurfaceFormatKHR         g_vk_surface_format;


u32 vk_get_surface_image_count()
{
	u32 image_count = g_vk_surface_capabilities.minImageCount;

	if ( g_vk_surface_capabilities.maxImageCount > 0 && image_count > g_vk_surface_capabilities.maxImageCount )
		image_count = g_vk_surface_capabilities.maxImageCount;

	return image_count;
}


void vk_swapchain_choose_surface_format()
{
	for ( u32 i = 0; i < g_surf_format_count; i++ )
	{
		if ( g_surf_formats[ i ].format == g_color_format && g_surf_formats[ i ].colorSpace == g_color_space )
		{
			g_vk_surface_format = g_surf_formats[ i ];
			return;
		}
	}

	// if we can't find the format we want, just use the first one
	Log_ErrorF( gLC_Render, "Failed to find desired surface format, using first one\n" );
	g_vk_surface_format = g_surf_formats[ 0 ];
}


VkPresentModeKHR vk_swapchain_choose_present_mode()
{
	// if ( !GetOption( "VSync" ) )
	// return VK_PRESENT_MODE_IMMEDIATE_KHR;

	for ( u32 i = 0; i < g_present_mode_count; i++ )
	{
		// vsync off
		if ( g_present_modes[ i ] == VK_PRESENT_MODE_IMMEDIATE_KHR )
			return g_present_modes[ i ];

		// vsync on
		if ( g_present_modes[ i ] == VK_PRESENT_MODE_MAILBOX_KHR )
			return g_present_modes[ i ];
	}

	// also vsync on?
	return VK_PRESENT_MODE_FIFO_KHR;
}


VkExtent2D vk_swapchain_extent( r_window_data_t* window )
{
	int width = 0, height = 0;
	SDL_GetWindowSize( window->window, &width, &height );

	VkExtent2D size{
		std::max( g_vk_surface_capabilities.minImageExtent.width, std::min( g_vk_surface_capabilities.maxImageExtent.width, (u32)width ) ),
		std::max( g_vk_surface_capabilities.minImageExtent.height, std::min( g_vk_surface_capabilities.maxImageExtent.height, (u32)height ) ),
	};

	return size;
}


void vk_swapchain_update_info( VkSurfaceKHR surface )
{
	if ( !g_vk_physical_device )
	{
		Log_Error( gLC_Render, "VK_UpdateSwapchainInfo(): No Physical Device?\n" );
		return;
	}

	vk_check( vkGetPhysicalDeviceSurfaceCapabilitiesKHR( g_vk_physical_device, surface, &g_vk_surface_capabilities ),
	                "Failed to Get Physical Device Surface Capabilities" );

	vk_swapchain_choose_surface_format();
}


void vk_swapchain_setup_info( device_info_t& info )
{
	g_vk_surface_capabilities = info.surface_capabilities;

	// copy this data, as it will be freed later
	g_surf_format_count    = info.surf_format_count;
	g_surf_formats = ch_malloc< VkSurfaceFormatKHR >( info.surf_format_count );
	memcpy( g_surf_formats, info.surf_formats, sizeof( VkSurfaceFormatKHR ) * info.surf_format_count );

	g_present_mode_count   = info.present_mode_count;
	g_present_modes = ch_malloc< VkPresentModeKHR >( info.present_mode_count );
	memcpy( g_present_modes, info.present_modes, sizeof( VkPresentModeKHR ) * info.present_mode_count );

	vk_swapchain_choose_surface_format();
}


bool vk_swapchain_create( r_window_data_t* window, VkSwapchainKHR old_swapchain )
{
	if ( window == nullptr )
		return false;

	u32 image_count          = vk_get_surface_image_count();

	window->swap_extent      = vk_swapchain_extent( window );
	window->swap_image_count = image_count;

	VkSwapchainCreateInfoKHR createInfo{ VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR };
	createInfo.surface          = window->surface;
	createInfo.minImageCount    = image_count;
	createInfo.imageFormat      = g_vk_surface_format.format;
	createInfo.imageColorSpace  = g_vk_surface_format.colorSpace;
	createInfo.imageExtent      = window->swap_extent;
	createInfo.imageArrayLayers = 1;
	createInfo.imageUsage       = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
	createInfo.preTransform     = g_vk_surface_capabilities.currentTransform;
	createInfo.compositeAlpha   = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	createInfo.presentMode      = vk_swapchain_choose_present_mode();
	createInfo.clipped          = VK_TRUE;
	createInfo.oldSwapchain     = old_swapchain;

	u32 queueFamilyIndices[ 2 ];

	if ( g_vk_queue_family_graphics != g_vk_queue_family_transfer )
	{
		queueFamilyIndices[ 0 ]          = g_vk_queue_family_graphics;
		queueFamilyIndices[ 1 ]          = g_vk_queue_family_transfer;

		createInfo.imageSharingMode      = VK_SHARING_MODE_CONCURRENT;
		createInfo.queueFamilyIndexCount = 2;
		createInfo.pQueueFamilyIndices   = queueFamilyIndices;
	}
	else
	{
		createInfo.imageSharingMode      = VK_SHARING_MODE_EXCLUSIVE;
		createInfo.queueFamilyIndexCount = 0;     // Optional
		createInfo.pQueueFamilyIndices   = NULL;  // Optional
	}

	vk_check( vkCreateSwapchainKHR( g_vk_device, &createInfo, NULL, &window->swapchain ), "Failed to create swapchain" );

	if ( old_swapchain )
		vkDestroySwapchainKHR( g_vk_device, old_swapchain, NULL );

	window->swap_images      = ch_realloc( window->swap_images, image_count );
	window->swap_image_views = ch_realloc( window->swap_image_views, image_count );

	vk_check( vkGetSwapchainImagesKHR( g_vk_device, window->swapchain, &image_count, window->swap_images ), "Failed to get swapchain images" );

	for ( u32 i = 0; i < image_count; ++i )
	{
		VkImageViewCreateInfo view_info{ VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
		view_info.pNext                           = nullptr;
		view_info.flags                           = 0;
		view_info.image                           = window->swap_images[ i ];
		view_info.viewType                        = VK_IMAGE_VIEW_TYPE_2D;
		view_info.format                          = g_vk_surface_format.format;
		view_info.components.r                    = VK_COMPONENT_SWIZZLE_IDENTITY;
		view_info.components.g                    = VK_COMPONENT_SWIZZLE_IDENTITY;
		view_info.components.b                    = VK_COMPONENT_SWIZZLE_IDENTITY;
		view_info.components.a                    = VK_COMPONENT_SWIZZLE_IDENTITY;
		view_info.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
		view_info.subresourceRange.baseMipLevel   = 0;
		view_info.subresourceRange.levelCount     = 1;
		view_info.subresourceRange.baseArrayLayer = 0;
		view_info.subresourceRange.layerCount     = 1;

		vk_check( vkCreateImageView( g_vk_device, &view_info, nullptr, &window->swap_image_views[ i ] ), "Failed to create swapchain image view" );

		vk_set_name( VK_OBJECT_TYPE_IMAGE, (u64)window->swap_images[ i ], "Swapchain Image" );
		vk_set_name( VK_OBJECT_TYPE_IMAGE_VIEW, (u64)window->swap_image_views[ i ], "Swapchain Image View" );
	}

	return true;
}


void vk_swapchain_destroy( r_window_data_t* window )
{
	// wait for nothing to potentially be using the swapchain
	vk_queue_wait_graphics();

	if ( window->swap_image_views )
	{
		for ( u32 i = 0; i < window->swap_image_count; i++ )
		{
			vkDestroyImageView( g_vk_device, window->swap_image_views[ i ], nullptr );
		}

		ch_free( window->swap_image_views );
	}

	// destroyed with vkDestroySwapchainKHR
	if ( window->swap_images )
	{
		ch_free( window->swap_images );
	}

	if ( window->swapchain )
		vkDestroySwapchainKHR( g_vk_device, window->swapchain, NULL );

	window->swap_image_views = nullptr;
	window->swap_images      = nullptr;
	window->swapchain        = VK_NULL_HANDLE;
}


void vk_swapchain_recreate( r_window_data_t* window )
{
	// wait for nothing to potentially be using the swapchain
	vk_queue_wait_graphics();

	if ( window->swap_image_views )
	{
		for ( u32 i = 0; i < window->swap_image_count; i++ )
		{
			vkDestroyImageView( g_vk_device, window->swap_image_views[ i ], nullptr );
		}
	}

	vk_swapchain_update_info( window->surface );
	vk_swapchain_create( window, window->swapchain );
}

