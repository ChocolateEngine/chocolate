#pragma once

#include <vulkan/vulkan.hpp>

#include "instance.h"
#include "core/log.h"

#include <SDL.h>


constexpr char const *VKString( VkResult sResult )
{
    switch ( sResult )
    {
        default:
            return "Unknown";
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


constexpr void CheckVKResult( VkResult sResult )
{
    if ( sResult == VK_SUCCESS )
        return;

    char pBuf[ 1024 ];
    snprintf( pBuf, sizeof( pBuf ), "Vulkan Error: %s", VKString( sResult ) );

    SDL_ShowSimpleMessageBox( SDL_MESSAGEBOX_ERROR, "Vulkan Error", pBuf, nullptr );
    LogFatal( pBuf );
}


constexpr void CheckVKResult( VkResult sResult, char const *spMsg )
{
    if ( sResult == VK_SUCCESS )
        return;

    char pBuf[ 1024 ];
    snprintf( pBuf, sizeof( pBuf ), "Vulkan Error %s: %s", spMsg, VKString( sResult ) );

    SDL_ShowSimpleMessageBox( SDL_MESSAGEBOX_ERROR, "Vulkan Error", pBuf, nullptr );
    LogFatal( pBuf );
}

