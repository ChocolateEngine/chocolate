#include "swapchain.h"

#include "graphics/renderertypes.h"

#include "instance.h"
#include "gutil.hh"
#include "rendertarget.h"

#include "config.hh"
#include "core/log.h"

VkSurfaceFormatKHR ChooseSwapSurfaceFormat( const std::vector< VkSurfaceFormatKHR > &srAvailableFormats )
{
	for ( const auto& availableFormat : srAvailableFormats )
	{
		if ( availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR )
		{
			return availableFormat;
		}
	}
	return srAvailableFormats[ 0 ];
}

VkPresentModeKHR ChooseSwapPresentMode( const std::vector< VkPresentModeKHR > &srAvailablePresentModes )
{
    if ( !GetOption( "VSync" ) )
	    return VK_PRESENT_MODE_IMMEDIATE_KHR;

	for ( const auto& availablePresentMode : srAvailablePresentModes )
	{
		if ( availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR )
			return availablePresentMode;
	}
	return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D ChooseSwapExtent( const VkSurfaceCapabilitiesKHR &srCapabilities )
{
	if ( srCapabilities.currentExtent.width != UINT32_MAX )
		return srCapabilities.currentExtent; 

	else
	{
		VkExtent2D size = {
			( unsigned int )GetGInstance().GetWindow().aWidth,
            ( unsigned int )GetGInstance().GetWindow().aHeight
		};

	       size.width  = std::max( srCapabilities.minImageExtent.width,  std::min( srCapabilities.maxImageExtent.width,  size.width  ) );
	       size.height = std::max( srCapabilities.minImageExtent.height, std::min( srCapabilities.maxImageExtent.height, size.height ) );

		return size;
	}
}

std::vector< VkImageView > CreateImageViews( const std::vector< VkImage >& srImages ) 
{
	std::vector< VkImageView > views;
	views.resize( srImages.size() );
	for ( int i = 0; i < srImages.size(); ++i )
	{
		VkImageViewCreateInfo aImageViewInfo           = {};
		aImageViewInfo.sType                           = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		aImageViewInfo.pNext                           = nullptr;
		aImageViewInfo.flags                           = 0;
		aImageViewInfo.image                           = srImages[ i ];
		aImageViewInfo.viewType                        = VK_IMAGE_VIEW_TYPE_2D;
		aImageViewInfo.format                          = VK_FORMAT_B8G8R8A8_SRGB;
		aImageViewInfo.components.r                    = VK_COMPONENT_SWIZZLE_IDENTITY;
		aImageViewInfo.components.g                    = VK_COMPONENT_SWIZZLE_IDENTITY;
		aImageViewInfo.components.b                    = VK_COMPONENT_SWIZZLE_IDENTITY;
		aImageViewInfo.components.a                    = VK_COMPONENT_SWIZZLE_IDENTITY;
		aImageViewInfo.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
		aImageViewInfo.subresourceRange.baseMipLevel   = 0;
		aImageViewInfo.subresourceRange.levelCount     = 1;
		aImageViewInfo.subresourceRange.baseArrayLayer = 0;
		aImageViewInfo.subresourceRange.layerCount     = 1;

		CheckVKResult( vkCreateImageView( GetLogicDevice(), &aImageViewInfo, nullptr, &views[ i ] ), "Failed to create image view!" );
	}

	return views;
}

Swapchain::Swapchain()
{
    SwapChainSupportInfo swapChainSupport = CheckSwapChainSupport( GetPhysicalDevice() );

	aSurfaceFormat 		= ChooseSwapSurfaceFormat( swapChainSupport.aFormats );
	aPresentMode 	    = ChooseSwapPresentMode( swapChainSupport.aPresentModes );
	aExtent 		    = ChooseSwapExtent( swapChainSupport.aCapabilities );

	uint32_t imageCount = swapChainSupport.aCapabilities.minImageCount + 1;
	if ( swapChainSupport.aCapabilities.maxImageCount > 0 && imageCount > swapChainSupport.aCapabilities.maxImageCount )
		imageCount = swapChainSupport.aCapabilities.maxImageCount;

	VkSwapchainCreateInfoKHR createInfo = {
	    .sType 		        = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
	    .surface 		    = GetGInstance().GetSurface(),
	    .minImageCount 	    = imageCount,
	    .imageFormat 		= aSurfaceFormat.format,
	    .imageColorSpace 	= aSurfaceFormat.colorSpace,
	    .imageExtent 		= aExtent,
	    .imageArrayLayers	= 1,
	    .imageUsage		    = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
    };

    QueueFamilyIndices indices 	    = FindQueueFamilies( GetPhysicalDevice() );
	uint32_t queueFamilyIndices[  ] = { ( uint32_t )indices.aGraphicsFamily, ( uint32_t )indices.aPresentFamily };

	if ( indices.aGraphicsFamily != indices.aPresentFamily )
	{
		createInfo.imageSharingMode 		= VK_SHARING_MODE_CONCURRENT;
		createInfo.queueFamilyIndexCount 	= 2;
		createInfo.pQueueFamilyIndices 		= queueFamilyIndices;
	}
	else
	{
		createInfo.imageSharingMode 		= VK_SHARING_MODE_EXCLUSIVE;
		createInfo.queueFamilyIndexCount 	= 0; // Optional
		createInfo.pQueueFamilyIndices 		= NULL; // Optional
	}

	createInfo.preTransform 	= swapChainSupport.aCapabilities.currentTransform;
	createInfo.compositeAlpha 	= VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	createInfo.presentMode 		= aPresentMode;
	createInfo.clipped		    = VK_TRUE;
	createInfo.oldSwapchain 	= VK_NULL_HANDLE;

	CheckVKResult( vkCreateSwapchainKHR( GetLogicDevice(), &createInfo, NULL, &aSwapChain ), "Failed to create swap chain!" );
	
	vkGetSwapchainImagesKHR( GetLogicDevice(), aSwapChain, &imageCount, NULL );
	aImages.resize( imageCount );
	vkGetSwapchainImagesKHR( GetLogicDevice(), aSwapChain, &imageCount, aImages.data(  ) );
	aImageViews = CreateImageViews( aImages );
}

Swapchain::~Swapchain()
{
    vkDestroySwapchainKHR( GetLogicDevice(), aSwapChain, NULL );
}

Swapchain &GetSwapchain()
{
	static Swapchain swapchain;
	return swapchain;
}