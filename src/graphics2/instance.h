#pragma once

#include "types.h"
#include "graphics.h"

struct SwapChainSupportInfo;

#ifdef NDEBUG
    constexpr bool 	      gEnableValidationLayers = false;
    constexpr char const *gpValidationLayers[]    = { 0 };
#else
    constexpr bool 	      gEnableValidationLayers = true;
    constexpr char const *gpValidationLayers[]    = { "VK_LAYER_KHRONOS_validation" };
    // constexpr char const *gpValidationLayers[]    = { "VK_LAYER_KHRONOS_validation", "VK_LAYER_LUNARG_api_dump" };
#endif

struct QueueFamilyIndices
{
    int 	aPresentFamily 	= -1;
    int	    aGraphicsFamily = -1;
	
    // Function that returns true if there is a valid queue family available.
    bool    Complete() { return ( aPresentFamily > -1 ) && ( aGraphicsFamily > -1 ); }
};

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

    void                        CreateWindow();
    std::vector< const char* >  GetRequiredExtensions();
    void                        CreateInstance();
    VkResult                    CreateValidationLayers();
                                
    bool                        SuitableCard( VkPhysicalDevice sDevice );
    VkSampleCountFlagBits       FindMaxMSAASamples();
    void                        SetupPhysicalDevice();
    void                        CreateDevice();
public:
     GInstance();
    ~GInstance();
	                            
    void                        Init();
                                
    uint32_t                    GetMemoryType( uint32_t sTypeFilter, VkMemoryPropertyFlags sProperties );
    QueueFamilyIndices          FindQueueFamilies( VkPhysicalDevice sDevice );
    SwapChainSupportInfo        CheckSwapChainSupport( VkPhysicalDevice sDevice );
                                
    constexpr Window&           GetWindow()          { return aWindow;         }
                                
    constexpr VkInstance        GetInstance()        { return aInstance;       }
    constexpr VkSurfaceKHR      GetSurface()         { return aSurface;        }
                                
    constexpr VkQueue           GetGraphicsQueue()   { return aGraphicsQueue;  }
    constexpr VkQueue           GetPresentQueue()    { return aPresentQueue;   }
                                
    constexpr VkPhysicalDevice  GetPhysicalDevice()  { return aPhysicalDevice; }
    constexpr VkDevice          GetDevice()          { return aDevice;  }
};

GInstance        &GetGInstance();

VkInstance        GetInst();

VkDevice          GetDevice();

VkPhysicalDevice  GetPhysicalDevice();