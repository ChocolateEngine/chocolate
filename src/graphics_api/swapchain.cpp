#include "core/platform.h"
#include "core/log.h"
#include "util.h"

#include "render_vk.h"


VkFormat                          gColorFormat = VK_FORMAT_B8G8R8A8_SRGB;
//VkFormat                          gColorFormat = VK_FORMAT_B8G8R8A8_UNORM;
VkColorSpaceKHR                   gColorSpace  = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;


bool VK_CreateSwapchain( WindowVK* window, VkSwapchainKHR spOldSwapchain )
{
	if ( window == nullptr )
		return false;

	VkSurfaceCapabilitiesKHR surfaceCapabilities = VK_GetSurfaceCapabilities();
	u32                      imageCount          = VK_GetSurfaceImageCount();

	window->swapSurfaceFormat                    = VK_ChooseSwapSurfaceFormat();
	window->swapPresentMode                      = VK_ChooseSwapPresentMode();
	window->swapExtent                           = VK_ChooseSwapExtent( window );
	window->swapImageCount                       = imageCount;

	VkSwapchainCreateInfoKHR createInfo{ VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR };
	createInfo.surface               = window->surface;
	createInfo.minImageCount         = imageCount;
	createInfo.imageFormat           = window->swapSurfaceFormat.format;
	createInfo.imageColorSpace       = window->swapSurfaceFormat.colorSpace;
	createInfo.imageExtent           = window->swapExtent;
	createInfo.imageArrayLayers      = 1;
	createInfo.imageUsage            = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	createInfo.preTransform          = surfaceCapabilities.currentTransform;
	createInfo.compositeAlpha        = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	createInfo.presentMode           = window->swapPresentMode;
	createInfo.clipped               = VK_TRUE;
	createInfo.oldSwapchain          = spOldSwapchain;

	u32 queueFamilyIndices[ 2 ];

	if ( gGraphicsAPIData.aQueueFamilyGraphics != gGraphicsAPIData.aQueueFamilyTransfer )
	{
		queueFamilyIndices[ 0 ]          = gGraphicsAPIData.aQueueFamilyGraphics;
		queueFamilyIndices[ 1 ]          = gGraphicsAPIData.aQueueFamilyTransfer;

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

	VK_CheckResult( vkCreateSwapchainKHR( VK_GetDevice(), &createInfo, NULL, &window->swapchain ), "Failed to create swapchain" );

	if ( spOldSwapchain )
		vkDestroySwapchainKHR( VK_GetDevice(), spOldSwapchain, NULL );

	window->swapImages     = ch_malloc_count< VkImage >( imageCount );
	window->swapImageViews = ch_malloc_count< VkImageView >( imageCount );

	VK_CheckResult( vkGetSwapchainImagesKHR( VK_GetDevice(), window->swapchain, &imageCount, window->swapImages ), "Failed to get swapchain images" );

	for ( u32 i = 0; i < imageCount; ++i )
	{
		VkImageViewCreateInfo aImageViewInfo{ VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
		aImageViewInfo.pNext                           = nullptr;
		aImageViewInfo.flags                           = 0;
		aImageViewInfo.image                           = window->swapImages[ i ];
		aImageViewInfo.viewType                        = VK_IMAGE_VIEW_TYPE_2D;
		aImageViewInfo.format                          = gColorFormat;
		aImageViewInfo.components.r                    = VK_COMPONENT_SWIZZLE_IDENTITY;
		aImageViewInfo.components.g                    = VK_COMPONENT_SWIZZLE_IDENTITY;
		aImageViewInfo.components.b                    = VK_COMPONENT_SWIZZLE_IDENTITY;
		aImageViewInfo.components.a                    = VK_COMPONENT_SWIZZLE_IDENTITY;
		aImageViewInfo.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
		aImageViewInfo.subresourceRange.baseMipLevel   = 0;
		aImageViewInfo.subresourceRange.levelCount     = 1;
		aImageViewInfo.subresourceRange.baseArrayLayer = 0;
		aImageViewInfo.subresourceRange.layerCount     = 1;

		VK_CheckResult( vkCreateImageView( VK_GetDevice(), &aImageViewInfo, nullptr, &window->swapImageViews[ i ] ), "Failed to create image view" );

		VK_SetObjectName( VK_OBJECT_TYPE_IMAGE, (u64)window->swapImages[ i ], "Swapchain Image" );
		VK_SetObjectName( VK_OBJECT_TYPE_IMAGE_VIEW, (u64)window->swapImageViews[ i ], "Swapchain Image View" );
	}

	return true;
}


void VK_DestroySwapchain( WindowVK* window )
{
	VK_WaitForGraphicsQueue();
	VK_WaitForTransferQueue();

	if ( window->swapImageViews )
	{
		for ( u32 i = 0; i < window->swapImageCount; i++ )
		{
			vkDestroyImageView( VK_GetDevice(), window->swapImageViews[ i ], nullptr );
		}

		free( window->swapImageViews );
	}

	// destroyed with vkDestroySwapchainKHR
	if ( window->swapImages )
	{
		free( window->swapImages );
	}

	if ( window->swapchain )
		vkDestroySwapchainKHR( VK_GetDevice(), window->swapchain, NULL );

	window->swapImageViews = nullptr;
	window->swapImages     = nullptr;
	window->swapchain      = VK_NULL_HANDLE;
}


void VK_RecreateSwapchain( WindowVK* window )
{
	VK_WaitForGraphicsQueue();
	VK_WaitForTransferQueue();

	if ( window->swapImageViews )
	{
		for ( u32 i = 0; i < window->swapImageCount; i++ )
		{
			vkDestroyImageView( VK_GetDevice(), window->swapImageViews[ i ], nullptr );
		}

		free( window->swapImageViews );
	}

	VK_UpdateSwapchainInfo( window->surface );
	VK_CreateSwapchain( window, window->swapchain );
}

