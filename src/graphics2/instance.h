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

    VkSampleCountFlagBits       aSampleCount;
    VkPhysicalDevice            aPhysicalDevice;
    VkDevice                    aDevice;
    VkQueue                     aGraphicsQueue;
    VkQueue                     aPresentQueue;

    void                       CreateWindow();
    std::vector< const char* > InitRequiredExtensions();
    void                       CreateInstance();
    VkResult                   CreateValidationLayers();

    VkSampleCountFlagBits      FindMaxMSAASamples();
    void                       SetupPhysicalDevice();
    void                       CreateDevice();
public:
     GInstance();
    ~GInstance();

    uint32_t                   GetMemoryType( uint32_t sTypeFilter, VkMemoryPropertyFlags sProperties );

    constexpr Window           GetWindow()          { return aWindow;         }

    constexpr VkInstance       GetInstance()        { return aInstance;       }
    constexpr VkSurfaceKHR     GetSurface()         { return aSurface;        }

    constexpr VkQueue          GetGraphicsQueue()   { return aGraphicsQueue;  }
    constexpr VkQueue          GetPresentQueue()    { return aPresentQueue;   }

    constexpr VkPhysicalDevice GetPhysicalDevice()  { return aPhysicalDevice; }
    constexpr VkDevice         GetDevice()          { return aDevice;         }
};

GInstance        &GetGInstance();

VkInstance        GetInst();

VkDevice          GetLogicDevice();

VkPhysicalDevice  GetPhysicalDevice();