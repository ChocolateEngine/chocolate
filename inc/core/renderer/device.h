/*
device.h ( Authored by p0lyh3dron )

Declares and defines many of the device
functions that utilize GPU functionality.
*/
#pragma once

#include "../../../inc/types/renderertypes.h"

#include <vulkan/vulkan.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_vulkan.h>
#include <vector>
#include <iostream>
#include <optional>

typedef std::vector< const char* > 	StringList;

#ifdef NDEBUG
    const bool 	gEnableValidationLayers = false;
#else
    const bool 	gEnableValidationLayers = true;
#endif

const 		StringList gValidationLayers = { "VK_LAYER_KHRONOS_validation" };
const 		StringList gDeviceExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

class Device
{
protected:
	typedef std::vector< VkSurfaceFormatKHR > 	SurfaceFormats;
	typedef std::vector< VkPresentModeKHR >		PresentModes;
	typedef std::vector< VkImage > 			ImageSet;
	typedef std::vector< VkFormat >			FormatSet;
	
	SDL_Window  			*apWindow;		        	//	Window to display stuff
	VkSurfaceKHR 			aSurface;				//	Allows window to display stuff	
	VkInstance 			aInstance;				//	Foundation for graphics API, stores application data
	VkDebugUtilsMessengerEXT 	aDebugMessenger;			//	Validation layers
	VkPhysicalDevice 		aPhysicalDevice;			//	GPU, we'll only be needing one of these
	VkDevice 			aDevice;				//	Stores features and is used to create other objects
	VkQueue 			aGraphicsQueue;
	VkQueue				aPresentQueue;
	VkCommandPool 			aCommandPool;
	/* Debug callback that displays the validation layer errors.  */
	static VKAPI_ATTR VkBool32 VKAPI_CALL   DebugCallback( VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType,
								const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData );
	/* Creates the main device which essentially preps the GPU.  */
	void 				        InitDevice(  );
	/* Create the SDL2 window.  */
	void 				        InitWindow(  );
	/* A.  */
	StringList 				InitRequiredExtensions(  );
	/* A.  */
	void 					InitInstance(  );
	/* A.  */
	void 					InitDebugMessengerInfo( VkDebugUtilsMessengerCreateInfoEXT &srCreateInfo );
	/* A.  */
	VkResult 				InitDebugMessenger( VkInstance sInstance, const VkDebugUtilsMessengerCreateInfoEXT *spCreateInfo,
								    const VkAllocationCallbacks *spAllocator, VkDebugUtilsMessengerEXT *spDebugMessenger );
	/* A.  */
	void 					InitValidationLayers(  );
	/* A.  */
	void 					InitSurface(  );
	/* A.  */
	void 					InitPhysicalDevice(  );
	/* A.  */
	void 					InitLogicalDevice(  );
	/* A.  */
	void 					InitCommandPool(  );
	/* A.  */
	bool 					CheckValidationLayerSupport(  );
	/* A.  */
        QueueFamilyIndices 			FindQueueFamilies( VkPhysicalDevice sDevice );
	/* A.  */
	bool 					CheckDeviceExtensionSupport( VkPhysicalDevice sDevice );
	/* A.  */
	SwapChainSupportInfo	 		CheckSwapChainSupport( VkPhysicalDevice sDevice );
	/* A.  */
	bool 					IsSuitableDevice( VkPhysicalDevice sDevice );
	/* A.  */
	VkSurfaceFormatKHR 			ChooseSwapSurfaceFormat( const SurfaceFormats &srAvailableFormats );
	/* A.  */
	VkPresentModeKHR 			ChooseSwapPresentMode( const PresentModes &srAvailablePresentModes );
	/* A.  */
	VkExtent2D 				ChooseSwapExtent( const VkSurfaceCapabilitiesKHR &srCapabilities );
	/* A.  */
	void 					DestroyDebugMessenger( const VkAllocationCallbacks *pAllocator );
	/* A.  */
	void 					Cleanup(  );
public:
	int 	aWidth 	= 1280;
	int 	aHeight = 720;
	/* A.  */
	void 					InitSwapChain( VkSwapchainKHR &srSwapChain, ImageSet &srSwapChainImages, VkFormat &srSwapChainImageFormat,
							       VkExtent2D &srSwapChainExtent );
	/* A.  */
	void 					InitTextureSampler( VkSampler& textureSampler, VkSamplerAddressMode mode );
	/* A.  */
	VkCommandBuffer 			BeginSingleTimeCommands(  );
	/* A.  */
	void 					EndSingleTimeCommands( VkCommandBuffer sCommandBuffer );
	/* A.  */
	VkFormat				FindSupportedFormat( const FormatSet &srCandidates, VkImageTiling sTiling, VkFormatFeatureFlags sFeatures );
	/* A.  */
	VkFormat 				FindDepthFormat(  );
	uint32_t 				FindMemoryType( uint32_t sTypeFilter, VkMemoryPropertyFlags sProperties );
	/* A.  */
	SDL_Window 				*GetWindow(  ){ return apWindow; }
	/* A.  */
	VkInstance 				GetInstance(  ){ return aInstance; }
	/* A.  */
	VkDevice 				GetDevice(  ){ return aDevice; }
	/* A.  */
	VkPhysicalDevice 			GetPhysicalDevice(  ){ return aPhysicalDevice; }
	/* A.  */
	VkCommandPool 				GetCommandPool(  ){ return aCommandPool; }
	/* A.  */
	VkQueue 				GetGraphicsQueue(  ){ return aGraphicsQueue; }
	/* A.  */
	VkQueue 				GetPresentQueue(  ){ return aPresentQueue; }
	/* A.  */
	void 					SetResolution( int sWidth, int sHeight ){ aWidth = sWidth; aHeight = sHeight; }
	/* A.  */
        					Device(  );
	/* A.  */
						~Device(  );
};
