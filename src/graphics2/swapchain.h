#pragma once

#include "types.h"

class Swapchain
{
protected:
    VkSurfaceFormatKHR 	       aSurfaceFormat;
	VkPresentModeKHR 	       aPresentMode;
	VkExtent2D 		           aExtent;
	VkSwapchainKHR		       aSwapChain;
    std::vector< VkImage >     aImages;
    std::vector< VkImageView > aImageViews;
public:
     Swapchain();
    ~Swapchain();

    uint32_t           GetImageCount()        { return aImages.size(); }
    VkSurfaceFormatKHR GetSurfaceFormat()     { return aSurfaceFormat; }
    VkFormat           GetFormat()            { return aSurfaceFormat.format; }
    VkColorSpaceKHR    GetColorSpace()        { return aSurfaceFormat.colorSpace; }
};

Swapchain &GetSwapchain();