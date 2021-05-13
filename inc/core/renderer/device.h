#ifndef DEVICE_H
#define DEVICE_H

#include <vulkan/vulkan.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_vulkan.h>
#include <vector>
#include <iostream>
#include <optional>

#ifdef NDEBUG
    const bool enableValidationLayers = false;
#else
    const bool enableValidationLayers = true;
#endif

typedef struct
{
	bool hasPresent, hasGraphics;
        int presentFamily, graphicsFamily; // make this not use optional pls
	bool complete (  ) { return hasPresent && hasGraphics; }
}queue_family_indices_t2;

typedef struct
{
	VkSurfaceCapabilitiesKHR 		c;	//	capabilities
	std::vector< VkSurfaceFormatKHR > 	f;	//	formats
	std::vector< VkPresentModeKHR > 	p;	//	present modes
}swap_chain_support_info_t;

const std::vector< const char* > validationLayers = {
    "VK_LAYER_KHRONOS_validation"
};

const std::vector< const char* > deviceExtensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

class device_c
{
	protected:

	int width = 1280, height = 720;
	SDL_Window* win;						//	Window to display stuff
	VkSurfaceKHR surf;						//	Allows window to display stuff	
	VkInstance inst;						//	Foundation for graphics API, stores application data
	VkDebugUtilsMessengerEXT debugMessenger;			//	Validation layers
	VkPhysicalDevice physicalDevice;				//	GPU, we'll only be needing one of these
	VkDevice device;						//	Stores features and is used to create other objects
	VkQueue graphicsQueue, presentQueue;

	static VKAPI_ATTR VkBool32 VKAPI_CALL debug_callback
		( VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
	  	  VkDebugUtilsMessageTypeFlagsEXT messageType,
	  	  const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
	  	  void* pUserData );
	
	void init_device
		(  );
	void init_window
		(  );
	std::vector< const char* > init_required_extensions
		(  );
	void init_instance
		(  );
	void init_debug_messenger_info
		( VkDebugUtilsMessengerCreateInfoEXT& c );
	VkResult init_debug_messenger
		( VkInstance instance,
	  	  const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
	  	  const VkAllocationCallbacks* pAllocator,
	  	  VkDebugUtilsMessengerEXT* pDebugMessenger );
	void init_validation_layers
		(  );
	void init_surface
		(  );
	void init_physical_device
		(  );
	void init_logical_device
		(  );

	bool check_validation_layer_support
		(  );
	queue_family_indices_t2 find_queue_families
		( VkPhysicalDevice d );
	bool check_device_extension_support
		( VkPhysicalDevice d );
	swap_chain_support_info_t check_swap_chain_support
		( VkPhysicalDevice d );
	bool is_suitable_device
		( VkPhysicalDevice d );

	VkSurfaceFormatKHR choose_swap_surface_format
		( const std::vector< VkSurfaceFormatKHR >& availableFormats );
	VkPresentModeKHR choose_swap_present_mode
		( const std::vector< VkPresentModeKHR >& availablePresentModes );
	VkExtent2D choose_swap_extent
		( const VkSurfaceCapabilitiesKHR& capabilities );
	
	public:

	void init_swap_chain
		( VkSwapchainKHR& swapChain, std::vector< VkImage >& swapChainImages, VkFormat& swapChainImageFormat, VkExtent2D& swapChainExtent );

	VkFormat find_supported_fmt
		( const std::vector< VkFormat >& candidates,
	  	  VkImageTiling tiling,
	  	  VkFormatFeatureFlags features );
	
	VkDevice get
		(  )
	{
		return device;
	}

	device_c
		(  );
	~device_c
		(  );
};

#endif
