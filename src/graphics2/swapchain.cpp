#include "core/platform.h"
#include "core/log.h"
#include "util.h"

#include "render_vk.h"

#include <algorithm>


static VkSurfaceFormatKHR         gSurfaceFormat;
static VkPresentModeKHR           gPresentMode;
static VkExtent2D                 gExtent;
static VkSwapchainKHR             gSwapChain;
static std::vector< VkImage >     gImages;
static std::vector< VkImageView > gImageViews;


// constexpr VkFormat                gColorFormat = VK_FORMAT_B8G8R8A8_SRGB;
// constexpr VkColorSpaceKHR         gColorSpace  = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;

VkFormat                          gColorFormat = VK_FORMAT_B8G8R8A8_UNORM;
VkColorSpaceKHR                   gColorSpace  = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;

void VK_CreateSwapchain( VkSwapchainKHR spOldSwapchain )
{
	auto swapCapabilities = VK_GetSwapCapabilities();
	gSurfaceFormat        = VK_ChooseSwapSurfaceFormat();
	gPresentMode          = VK_ChooseSwapPresentMode();
	gExtent               = VK_ChooseSwapExtent();

	uint32_t imageCount   = swapCapabilities.minImageCount;

	if ( swapCapabilities.maxImageCount > 0 && imageCount > swapCapabilities.maxImageCount )
		imageCount = swapCapabilities.maxImageCount;

	VkSwapchainCreateInfoKHR createInfo{ VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR };
	createInfo.surface               = VK_GetSurface();
	createInfo.minImageCount         = imageCount;
	createInfo.imageFormat           = gSurfaceFormat.format;
	createInfo.imageColorSpace       = gSurfaceFormat.colorSpace;
	createInfo.imageExtent           = gExtent;
	createInfo.imageArrayLayers      = 1;
	createInfo.imageUsage            = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	createInfo.preTransform          = swapCapabilities.currentTransform;
	createInfo.compositeAlpha        = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	createInfo.presentMode           = gPresentMode;
	createInfo.clipped               = VK_TRUE;
	createInfo.oldSwapchain          = spOldSwapchain;

	uint32_t queueFamilyIndices[ 2 ] = { 0, 0 };
	VK_FindQueueFamilies( VK_GetPhysicalDevice(), &queueFamilyIndices[ 0 ], &queueFamilyIndices[ 1 ] );

	if ( queueFamilyIndices[ 0 ] != queueFamilyIndices[ 1 ] )
	{
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

	VK_CheckResult( vkCreateSwapchainKHR( VK_GetDevice(), &createInfo, NULL, &gSwapChain ), "Failed to create swapchain" );

	if ( spOldSwapchain )
		vkDestroySwapchainKHR( VK_GetDevice(), spOldSwapchain, NULL );

	VK_CheckResult( vkGetSwapchainImagesKHR( VK_GetDevice(), gSwapChain, &imageCount, NULL ), "Failed to get swapchain images" );
	gImages.resize( imageCount );
	VK_CheckResult( vkGetSwapchainImagesKHR( VK_GetDevice(), gSwapChain, &imageCount, gImages.data() ), "Failed to get swapchain images" );

	gImageViews.resize( gImages.size() );
	for ( int i = 0; i < gImages.size(); ++i )
	{
		VkImageViewCreateInfo aImageViewInfo{ VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
		aImageViewInfo.pNext                           = nullptr;
		aImageViewInfo.flags                           = 0;
		aImageViewInfo.image                           = gImages[ i ];
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

		VK_CheckResult( vkCreateImageView( VK_GetDevice(), &aImageViewInfo, nullptr, &gImageViews[ i ] ), "Failed to create image view" );
	}
}

void VK_DestroySwapchain()
{
	VK_WaitForPresentQueue();

	for ( auto& imgView : gImageViews )
		vkDestroyImageView( VK_GetDevice(), imgView, nullptr );

	vkDestroySwapchainKHR( VK_GetDevice(), gSwapChain, NULL );

	gImageViews.clear();
	gImages.clear();  // destroyed with vkDestroySwapchainKHR

	gSwapChain = nullptr;
}

void VK_RecreateSwapchain()
{
	VK_WaitForPresentQueue();

	for ( auto& imgView : gImageViews )
		vkDestroyImageView( VK_GetDevice(), imgView, nullptr );

	VK_UpdateSwapchainInfo();
	VK_CreateSwapchain( gSwapChain );
}


u32 VK_GetSwapImageCount()
{
	return (u32)gImages.size();
}


const std::vector< VkImage >& VK_GetSwapImages()
{
	return gImages;
}


const std::vector< VkImageView >& VK_GetSwapImageViews()
{
	return gImageViews;
}


const VkExtent2D& VK_GetSwapExtent()
{
	return gExtent;
}


VkSurfaceFormatKHR VK_GetSwapSurfaceFormat()
{
	return gSurfaceFormat;
}


VkFormat VK_GetSwapFormat()
{
	return gSurfaceFormat.format;
}


VkColorSpaceKHR VK_GetSwapColorSpace()
{
	return gSurfaceFormat.colorSpace;
}


VkSwapchainKHR VK_GetSwapchain()
{
	return gSwapChain;
}

