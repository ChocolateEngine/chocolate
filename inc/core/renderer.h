#ifndef RENDERER_H
#define RENDERER_H

#include "system.h"

#include <vulkan/vulkan.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_vulkan.h>

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

typedef struct
{
	std::optional< int > presentFamily, graphicsFamily; // make this not use optional pls
	bool complete (  ) { return presentFamily.has_value(  ) && graphicsFamily.has_value(  ); }
}queue_family_indices_t;

typedef struct
{
	VkSurfaceCapabilitiesKHR 		c;	//	capabilities
	std::vector< VkSurfaceFormatKHR > 	f;	//	formats
	std::vector< VkPresentModeKHR > 	p;	//	present modes
}swap_chain_support_info_t;

class renderer_c : public system_c
{
	protected:

	int width = 1280, height = 720;

	SDL_Window* win;
	VkSurfaceKHR surf;
	VkSwapchainKHR swapChain;
	
	VkInstance inst;
	
	VkPhysicalDevice physicalDevice;
	VkDevice device;
	
	VkQueue graphicsQueue, presentQueue;
	
	std::vector<VkImageView> swapChainImageViews;
	std::vector<VkImage> swapChainImages;
	std::vector<VkFramebuffer> swapChainFramebuffers;
	std::vector<VkCommandBuffer> commandBuffers;
	
	VkFormat swapChainImageFormat;
	VkExtent2D swapChainExtent;
	VkRenderPass renderPass;
	
	VkPipelineLayout pipelineLayout;
	VkPipeline graphicsPipeline;
	VkCommandPool commandPool;
	
	VkSemaphore imageAvailableSemaphore, renderFinishedSemaphore;

	void init_vulkan
		(  );
	void init_window
		(  );
	void init_instance
		(  );
	void init_surface
		(  );
	void pick_physical_device
		(  );
	void init_logical_device
		(  );
	void init_swap_chain
		(  );
	void init_image_views
		(  );
	void init_render_pass
		(  );
	void init_graphics_pipeline
		(  );

	bool check_validation_layer_support
		(  );
	bool is_suitable_device
		( VkPhysicalDevice d );
	bool check_device_extension_support
		( VkPhysicalDevice d );

	VkSurfaceFormatKHR choose_swap_surface_format
		( const std::vector< VkSurfaceFormatKHR >& availableFormats );
	VkPresentModeKHR choose_swap_present_mode
		( const std::vector< VkPresentModeKHR >& availablePresentModes );
	VkExtent2D choose_swap_extent
		( const VkSurfaceCapabilitiesKHR& capabilities );
	VkShaderModule create_shader_module
		( const std::vector< char >& code );

	static std::vector< char > read_file
		( const std::string& filePath );

	queue_family_indices_t find_queue_families
		( VkPhysicalDevice d );
	swap_chain_support_info_t check_swap_chain_support
		( VkPhysicalDevice d );
	
	void cleanup
		(  );
	
	public:

	renderer_c
		(  );
	~renderer_c
		(  );
};

#endif
