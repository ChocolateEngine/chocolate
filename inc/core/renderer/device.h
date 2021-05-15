#ifndef DEVICE_H
#define DEVICE_H

#include "../../../inc/types/renderertypes.h"

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
	VkCommandPool commandPool;

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
	void init_command_pool
		(  );

	bool check_validation_layer_support
		(  );
	queue_family_indices_t find_queue_families
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

	void destroy_debug_messenger
		( const VkAllocationCallbacks* pAllocator );
	void cleanup
		(  );
	
	public:

	void init_swap_chain
		( VkSwapchainKHR& swapChain, std::vector< VkImage >& swapChainImages, VkFormat& swapChainImageFormat, VkExtent2D& swapChainExtent );
	void init_texture_sampler
		( VkSampler& textureSampler );

	VkCommandBuffer begin_single_time_commands
		(  );
	void end_single_time_commands
		( VkCommandBuffer c );

	VkFormat find_supported_fmt
		( const std::vector< VkFormat >& candidates,
	  	  VkImageTiling tiling,
	  	  VkFormatFeatureFlags features );
	VkFormat find_depth_format
		(  );
	uint32_t find_memory_type
		( uint32_t typeFilter, VkMemoryPropertyFlags properties );
	
	VkDevice dev
		(  )
	{
		return device;
	}
	VkCommandPool c_pool
		(  )
	{
		return commandPool;
	}
	VkQueue g_queue
		(  )
	{
		return graphicsQueue;
	}
	VkQueue p_queue
		(  )
	{
		return presentQueue;
	}

	device_c
		(  );
	~device_c
		(  );
};

#endif
