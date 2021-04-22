#ifndef RENDERER_H
#define RENDERER_H

#include "system.h"
#include "../types/enums.h"

#include <vulkan/vulkan.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_vulkan.h>
#include <glm/glm.hpp>
#include <array>

#include <optional>

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

typedef struct vertex_s
{
	glm::vec2 pos;
	glm::vec3 color;

	static VkVertexInputBindingDescription get_binding_desc
		(  )
	{
		VkVertexInputBindingDescription bindingDescription{};
		bindingDescription.binding = 0;
		bindingDescription.stride = sizeof( vertex_s );
		bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

		return bindingDescription;
	}

	static std::array< VkVertexInputAttributeDescription, 2 > get_attribute_desc
		(  )
	{
		std::array< VkVertexInputAttributeDescription, 2 >attributeDescriptions{  };
		attributeDescriptions[ 0 ].binding = 0;
		attributeDescriptions[ 0 ].location = 0;
		attributeDescriptions[ 0 ].format = VK_FORMAT_R32G32_SFLOAT;
		attributeDescriptions[ 0 ].offset = offsetof( vertex_s, pos );

		attributeDescriptions[ 1 ].binding = 0;
		attributeDescriptions[ 1 ].location = 1;
		attributeDescriptions[ 1 ].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[ 1 ].offset = offsetof( vertex_s, color );

		return attributeDescriptions;
	}
}vertex_t;


#ifdef NDEBUG
    const bool enableValidationLayers = false;
#else
    const bool enableValidationLayers = true;
#endif

const int MAX_FRAMES_PROCESSING = 2;

const std::vector< const char* > validationLayers = {
    "VK_LAYER_KHRONOS_validation"
};

const std::vector< const char* > deviceExtensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

const std::vector< vertex_t > vertices = {
	{ { -0.5f, -0.5f }, { 1.0f, 1.0f, 0.5f } },
	{ { 0.5f, -0.5f }, { 0.0f, 1.0f, 0.0f } },
	{ { 0.5f, 0.5f }, { 0.0f, 0.0f, 1.0f } },
	{ { -0.5f, 0.5f }, { 1.0f, 1.0f, 1.0f } }
};

const std::vector< uint16_t > indicesVector = {
	0, 1, 2, 2, 3, 0
};

class renderer_c : public system_c
{
	protected:
	
	int width = 1280, height = 720;
	int currentFrame = 0;

	SDL_Window* win;
	VkSurfaceKHR surf;
	VkSwapchainKHR swapChain;
	
	VkInstance inst;
	VkDebugUtilsMessengerEXT debugMessenger;
	
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

	VkBuffer vertexBuffer, indexBuffer;
	VkDeviceMemory vertexBufferMemory, indexBufferMemory;
	
	std::vector< VkSemaphore > imageAvailableSemaphores, renderFinishedSemaphores;
	std::vector< VkFence > inFlightFences, imagesInFlight;

	void init_commands
		(  );
	
	void init_vulkan
		(  );
	void init_window
		(  );
	std::vector< const char* > init_required_extensions
		(  );
	void init_instance
		(  );
	void init_debug_messenger_info
		( VkDebugUtilsMessengerCreateInfoEXT& c );
	void init_validation_layers
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
	void init_frame_buffer
		(  );
	void init_command_pool
		(  );
	void init_command_buffers
		(  );
	void init_sync
		(  );
	void init_vertex_buffer
		(  );
	void init_index_buffer
		(  );
	void reinit_swap_chain
		(  );
	void destroy_swap_chain
		(  );
	
	void init_buffer
		( VkDeviceSize size,
		  VkBufferUsageFlags usage,
		  VkMemoryPropertyFlags properties,
		  VkBuffer& buffer,
		  VkDeviceMemory& bufferMemory );
	void buf_copy
		( VkBuffer src, VkBuffer dst, VkDeviceSize size );

	bool check_validation_layer_support
		(  );
	bool is_suitable_device
		( VkPhysicalDevice d );
	bool check_device_extension_support
		( VkPhysicalDevice d );

	uint32_t find_memory_type
		( uint32_t typeFilter, VkMemoryPropertyFlags properties );

	VkResult init_debug_messenger
		( VkInstance instance,
		  const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
		  const VkAllocationCallbacks* pAllocator,
		  VkDebugUtilsMessengerEXT* pDebugMessenger );
	void destroy_debug_messenger
		( VkInstance instance,
		  VkDebugUtilsMessengerEXT debugMessenger,
		  const VkAllocationCallbacks* pAllocator );

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
	static VKAPI_ATTR VkBool32 VKAPI_CALL debug_callback
		( VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
		  VkDebugUtilsMessageTypeFlagsEXT messageType,
		  const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
		  void* pUserData );

	queue_family_indices_t find_queue_families
		( VkPhysicalDevice d );
	swap_chain_support_info_t check_swap_chain_support
		( VkPhysicalDevice d );
	
	void cleanup
		(  );
	
	public:

	void draw_frame
		(  );
	
	renderer_c
		(  );
	~renderer_c
		(  );
};

#endif
