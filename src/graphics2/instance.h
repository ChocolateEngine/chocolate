#pragma once

#include "types.h"
#include "graphics.h"

#ifdef NDEBUG
    constexpr bool 	      gEnableValidationLayers = false;
    constexpr char const *gpValidationLayers[]    = {};
#else
    constexpr bool 	      gEnableValidationLayers = true;
    constexpr char const *gpValidationLayers[]    = { "VK_LAYER_KHRONOS_validation" };
#endif

class GInstance
{
protected:
    Window                      aWindow;

    VkInstance                  aInstance;
    VkDebugUtilsMessengerEXT 	aLayers;
    VkSurfaceKHR                aSurface;

    void                       CreateWindow();
    std::vector< const char* > InitRequiredExtensions();
    void                       CreateInstance();
    VkResult                   CreateValidationLayers();
public:
     GInstance();
    ~GInstance();

    constexpr Window           GetWindow()   { return aWindow;   }

    constexpr VkInstance       GetInstance() { return aInstance; }
    constexpr VkSurfaceKHR     GetSurface()  { return aSurface;  }
};

GInstance &GetGInstance();