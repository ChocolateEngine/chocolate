#pragma once

#include <vulkan/vulkan.hpp>

#include "graphics/renderertypes.h"
#include "instance.h"
#include "core/log.h"

#include <SDL.h>

constexpr char const *VKString( VkResult sResult ) {
    switch ( sResult ) {
        case VK_SUCCESS:
            return "VK_SUCCESS";
        case VK_ERROR_OUT_OF_HOST_MEMORY:
            return "VK_ERROR_OUT_OF_HOST_MEMORY";
        case VK_ERROR_OUT_OF_DEVICE_MEMORY:
            return "VK_ERROR_OUT_OF_DEVICE_MEMORY";
        case VK_ERROR_INITIALIZATION_FAILED:
            return "VK_ERROR_INITIALIZATION_FAILED";
        case VK_ERROR_LAYER_NOT_PRESENT:
            return "VK_ERROR_LAYER_NOT_PRESENT";
        case VK_ERROR_EXTENSION_NOT_PRESENT:
            return "VK_ERROR_EXTENSION_NOT_PRESENT";
        case VK_ERROR_FEATURE_NOT_PRESENT:
            return "VK_ERROR_FEATURE_NOT_PRESENT";
        case VK_ERROR_INCOMPATIBLE_DRIVER:
            return "VK_ERROR_INCOMPATIBLE_DRIVER";
        case VK_ERROR_TOO_MANY_OBJECTS:
            return "VK_ERROR_TOO_MANY_OBJECTS";
        case VK_ERROR_FORMAT_NOT_SUPPORTED:
            return "VK_ERROR_FORMAT_NOT_SUPPORTED";
        case VK_ERROR_SURFACE_LOST_KHR:
            return "VK_ERROR_SURFACE_LOST_KHR";
        case VK_ERROR_NATIVE_WINDOW_IN_USE_KHR:
            return "VK_ERROR_NATIVE_WINDOW_IN_USE_KHR";
    }
}
#if 0
void CheckVKResult( VkResult sResult ) {
    if ( sResult == VK_SUCCESS )
        return;

    char pBuf[ 1024 ];
    snprintf( pBuf, sizeof( pBuf ), "Vulkan Error: %s", VKString( sResult ) );

    SDL_ShowSimpleMessageBox( SDL_MESSAGEBOX_ERROR, "Vulkan Error", pBuf, nullptr );
    throw std::runtime_error( pBuf );
}
#endif

constexpr void CheckVKResult( VkResult sResult, char const *spMsg ) {
    if ( sResult == VK_SUCCESS )
        return;

    char pBuf[ 1024 ];
    snprintf( pBuf, sizeof( pBuf ), "Vulkan Error %s: %s", spMsg, VKString( sResult ) );

    SDL_ShowSimpleMessageBox( SDL_MESSAGEBOX_ERROR, "Vulkan Error", pBuf, nullptr );
    LogFatal( pBuf );
}

static inline QueueFamilyIndices FindQueueFamilies( VkPhysicalDevice sDevice )
{
	QueueFamilyIndices indices;
	uint32_t queueFamilyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties( sDevice, &queueFamilyCount, nullptr );

	std::vector< VkQueueFamilyProperties > queueFamilies( queueFamilyCount );
	vkGetPhysicalDeviceQueueFamilyProperties( sDevice, &queueFamilyCount, queueFamilies.data(  ) );// Logic to find queue family indices to populate struct with
	int i = 0;
	for ( const auto& queueFamily : queueFamilies )
	{
		if ( queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT )
		{
			VkBool32 presentSupport = false;
			vkGetPhysicalDeviceSurfaceSupportKHR( sDevice, i, GetGInstance().GetSurface(), &presentSupport );
			if ( presentSupport )
			{
				indices.aPresentFamily = i;
			}
			indices.aGraphicsFamily = i;
		}
		if ( indices.Complete(  ) )
		{
			break;
		}
		i++;
	}
	return indices;
}

static inline SwapChainSupportInfo CheckSwapChainSupport( VkPhysicalDevice sDevice )
{
    auto surf = GetGInstance().GetSurface();
	SwapChainSupportInfo details;
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR( sDevice, surf, &details.aCapabilities );

	uint32_t formatCount;
	vkGetPhysicalDeviceSurfaceFormatsKHR( sDevice, surf, &formatCount, NULL );

	if ( formatCount != 0 )
	{
		details.aFormats.resize( formatCount );
		vkGetPhysicalDeviceSurfaceFormatsKHR( sDevice, surf, &formatCount, details.aFormats.data(  ) );
	}
	uint32_t presentModeCount;
	vkGetPhysicalDeviceSurfacePresentModesKHR( sDevice, surf, &presentModeCount, NULL );

	if ( presentModeCount != 0 )
	{
		details.aPresentModes.resize( presentModeCount );
		vkGetPhysicalDeviceSurfacePresentModesKHR( sDevice, surf, &presentModeCount, details.aPresentModes.data(  ) );
	}
	return details;
}

