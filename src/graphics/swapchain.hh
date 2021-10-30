/*
swapchain.h ( Authored by p0lyh3dron )

Declares the swapchain, the physical render
target for drawing.  
*/
#include <vulkan/vulkan.hpp>

#pragma once

class SwapChain
{
	typedef std::vector< VkImage >		ImageSet;
protected:
	VkSwapchainKHR	aSwapChain;
	ImageSet		aSwapChainImages;
	VkFormat		aSwapChainImageFormat;
	VkExtent2D		aSwapChainExtent;
public:
	/* Returns the swap chain.  */
	VkSwapchainKHR		GetSwapChain(  ){ return aSwapChain; }
	/* Returns the swap chain images.  */
	ImageSet		GetImages(  ){ return aSwapChainImages; }
	/* Returns the format.  */
	VkFormat		GetFormat(  ){ return aSwapChainImageFormat; }
	/* Returns the extent.  */
	VkExtent2D		GetExtent(  ){ return aSwapChainExtent; }
	/* Sets variables.  */
        void			Init( VkSwapchainKHR sSwapChain, ImageSet &srSwapChainImages,
				      VkFormat sSwapChainImageFormat, VkExtent2D sSwapChainExtent )
		{
			aSwapChain 		= sSwapChain;
			aSwapChainImages 	= srSwapChainImages;
			aSwapChainImageFormat 	= sSwapChainImageFormat;
			aSwapChainExtent 	= sSwapChainExtent;
		}
};
