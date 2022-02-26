#include "swapchain.h"

#include "graphics/renderertypes.h"

#include "device.h"
#include "instance.h"
#include "gutil.hh"

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
			( unsigned int )gInstance.GetWindow().aWidth,
            ( unsigned int )gInstance.GetWindow().aHeight
		};

	       size.width  = std::max( srCapabilities.minImageExtent.width,  std::min( srCapabilities.maxImageExtent.width,  size.width  ) );
	       size.height = std::max( srCapabilities.minImageExtent.height, std::min( srCapabilities.maxImageExtent.height, size.height ) );

		return size;
	}
}

Swapchain::Swapchain()
{
    SwapChainSupportInfo 	swapChainSupport    = CheckSwapChainSupport( gDevice.GetPhysicalDevice() );

	VkSurfaceFormatKHR 	    aSurfaceFormat 		= ChooseSwapSurfaceFormat( swapChainSupport.aFormats );
	VkPresentModeKHR 	    aPresentMode 	    = ChooseSwapPresentMode( swapChainSupport.aPresentModes );
	VkExtent2D 		        aExtent 		    = ChooseSwapExtent( swapChainSupport.aCapabilities );

	uint32_t imageCount 				= swapChainSupport.aCapabilities.minImageCount + 1;
	if ( swapChainSupport.aCapabilities.maxImageCount > 0 && imageCount > swapChainSupport.aCapabilities.maxImageCount )
		imageCount = swapChainSupport.aCapabilities.maxImageCount;

	VkSwapchainCreateInfoKHR createInfo = {
	    .sType 		        = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
	    .surface 		    = gInstance.GetSurface(),
	    .minImageCount 	    = imageCount,
	    .imageFormat 		= aSurfaceFormat.format,
	    .imageColorSpace 	= aSurfaceFormat.colorSpace,
	    .imageExtent 		= aExtent,
	    .imageArrayLayers	= 1,
	    .imageUsage		    = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
    };

    QueueFamilyIndices indices 	    = FindQueueFamilies( gDevice.GetPhysicalDevice() );
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

	if ( vkCreateSwapchainKHR( gDevice.GetDevice(), &createInfo, NULL, &aSwapChain ) != VK_SUCCESS )
		LogFatal( "Failed to create swap chain!" );
	
	vkGetSwapchainImagesKHR( gDevice.GetDevice(), aSwapChain, &imageCount, NULL );
	aImages.resize( imageCount );
	vkGetSwapchainImagesKHR( gDevice.GetDevice(), aSwapChain, &imageCount, aImages.data(  ) );
}

Swapchain::~Swapchain()
{
    vkDestroySwapchainKHR( gDevice.GetDevice(), aSwapChain, NULL );
}

Swapchain gSwapChain{};