#ifndef RENDERER_H
#define RENDERER_H

#define GLM_ENABLE_EXPERIMENTAL

#include "system.h"
#include "../types/enums.h"

#include <vulkan/vulkan.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_vulkan.h>
#include <glm/glm.hpp>
#include <glm/gtx/hash.hpp>
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
	glm::vec3 pos;
	glm::vec3 color;
	glm::vec2 texCoord;

	static VkVertexInputBindingDescription get_binding_desc
		(  )
	{
		VkVertexInputBindingDescription bindingDescription{};
		bindingDescription.binding = 0;
		bindingDescription.stride = sizeof( vertex_s );
		bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

		return bindingDescription;
	}

	static std::array< VkVertexInputAttributeDescription, 3 > get_attribute_desc
		(  )
	{
		std::array< VkVertexInputAttributeDescription, 3 >attributeDescriptions{  };
		attributeDescriptions[ 0 ].binding  = 0;
		attributeDescriptions[ 0 ].location = 0;
		attributeDescriptions[ 0 ].format   = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[ 0 ].offset   = offsetof( vertex_s, pos );

		attributeDescriptions[ 1 ].binding  = 0;
		attributeDescriptions[ 1 ].location = 1;
		attributeDescriptions[ 1 ].format   = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[ 1 ].offset   = offsetof( vertex_s, color );

		attributeDescriptions[ 2 ].binding  = 0;
		attributeDescriptions[ 2 ].location = 2;
		attributeDescriptions[ 2 ].format   = VK_FORMAT_R32G32_SFLOAT;
		attributeDescriptions[ 2 ].offset   = offsetof( vertex_s, texCoord );

		return attributeDescriptions;
	}
	bool operator==( const vertex_s& other ) const
	{
		return pos == other.pos && color == other.color && texCoord == other.texCoord;
	}
}vertex_t;

namespace std
{
	template<  > struct hash< vertex_t >
	{
		size_t operator(  )( vertex_t const& vertex ) const
		{
			return  ( ( hash< glm::vec3 >(  )( vertex.pos ) ^
                   		( hash< glm::vec3 >(  )( vertex.color ) << 1 ) ) >> 1 ) ^
				( hash< glm::vec2 >(  )( vertex.texCoord ) << 1 );
		}
	};
}

typedef struct
{
	glm::mat4 model, view, proj;
}ubo_t;

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

const std::string MODELPATH = "materials/models/protogen_wip_5_plus_protodal.obj";
const std::string TEXTUREPATH = "materials/textures/blank_mat.png";

class renderer_c : public system_c	//	Most of these objects are used to make new objects by initializing them with certain parameters, then creating and storing them
{
	protected:
	
	int width = 1280, height = 720;
	int currentFrame = 0;

	std::vector< vertex_t > vertices;
	std::vector< uint32_t > indicesVector;

	SDL_Window* win;						//	Window to display stuff
	VkSurfaceKHR surf;						//	Allows window to display stuff
	VkSwapchainKHR swapChain;					//	Queue for stuff to be rendered, does some processing before drawn to screen
	
	VkInstance inst;						//	Foundation for graphics API, stores application data
	VkDebugUtilsMessengerEXT debugMessenger;			//	Validation layers
	
	VkPhysicalDevice physicalDevice;				//	GPU, we'll only be needing one of these
	VkDevice device;						//	Stores features and is used to create other objects
	
	VkQueue graphicsQueue, presentQueue;				//	
	
	std::vector<VkImageView> swapChainImageViews;			//	View into an image, describing which part to access, one needed for each image
	std::vector<VkImage> swapChainImages;				//	Stores images to be rendered, can have many
	std::vector<VkFramebuffer> swapChainFramebuffers;		//	
	std::vector<VkCommandBuffer> commandBuffers;			//	Send commands to these to be executed later, better for concurrency, so many are nice
	
	VkFormat swapChainImageFormat;
	VkExtent2D swapChainExtent;
	VkRenderPass renderPass;					//	Stores framebuffer attachments that will be used for rendering

	VkDescriptorSetLayout descSetLayout;
	VkDescriptorPool descPool;
	std::vector< VkDescriptorSet > descSets;

	VkImage textureImage;
	VkDeviceMemory textureImageMemory;
	VkImageView textureImageView;
	VkSampler textureSampler;

	VkImage depthImage;
	VkDeviceMemory depthImageMemory;
	VkImageView depthImageView;
	
	VkPipelineLayout pipelineLayout;
	VkPipeline graphicsPipeline;					//	Colossal object that handles most stages of rendering. Seems to need multiple to render multiple things.
	VkCommandPool commandPool;					//	Manage memory related to command buffers

	VkBuffer vertexBuffer, indexBuffer;
	VkDeviceMemory vertexBufferMemory, indexBufferMemory;
	std::vector< VkBuffer > uniformBuffers;
	std::vector< VkDeviceMemory > uniformBuffersMemory;
	
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
	void init_render_pass				//	
		(  );
	void init_desc_set_layout
		(  );
	void init_graphics_pipeline			//	Initialize the graphics pipeline to load a shader and various other tools for rendering
		(  );					//	TODO: create parameters to specify a shader, as the rest done in this function doesn't need to be changed for multiple pipelines
	void init_frame_buffer
		(  );
	void init_command_pool
		(  );
	void init_depth_resources
		(  );
	void init_command_buffers
		(  );
	void init_sync
		(  );
	void init_texture_image
		(  );
	void init_texture_image_view
		(  );
	void init_texture_sampler
		(  );
	void init_model
	(  );
	void init_image
		( uint32_t width,
		  uint32_t height,
		  VkFormat format,
		  VkImageTiling tiling,
		  VkImageUsageFlags usage,
		  VkMemoryPropertyFlags properties,
		  VkImage& image,
		  VkDeviceMemory& imageMemory );
	void update_vertex_buffer			//	Initialize device memory with vertex data, may need to be called each time new vertices are loaded
		(  );
	void update_index_buffer			//	Initialize device memory with index data, may need to be called each time new vertices are loaded
		(  );
	void init_uniform_buffers			//	Pass arbitrary attributes to vertex shader for each vertex, allows vertex positioning without remapping memory
		(  );
	void init_desc_pool
		(  );
	void init_desc_sets
		(  );
	void reinit_swap_chain
		(  );
	void destroy_swap_chain
		(  );

	VkImageView init_image_view
		( VkImage image, VkFormat format, VkImageAspectFlags aspectFlags );
	void transition_image_layout
		( VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout );
	VkCommandBuffer begin_single_time_commands
		(  );
	void end_single_time_commands
		( VkCommandBuffer c );
	void init_buffer
		( VkDeviceSize size,
		  VkBufferUsageFlags usage,
		  VkMemoryPropertyFlags properties,
		  VkBuffer& buffer,
		  VkDeviceMemory& bufferMemory );
	void map_memory
		( VkDeviceMemory bufMem, VkDeviceSize size, const void* in );
	void buf_copy
		( VkBuffer src, VkBuffer dst, VkDeviceSize size );
	void copy_buffer_to_img
		( VkBuffer buffer, VkImage image, uint32_t width, uint32_t height );

	bool check_validation_layer_support
		(  );
	bool is_suitable_device
		( VkPhysicalDevice d );
	bool check_device_extension_support
		( VkPhysicalDevice d );
	VkFormat find_supported_fmt
		( const std::vector< VkFormat >& candidates,
		  VkImageTiling tiling,
		  VkFormatFeatureFlags features );
	VkFormat find_depth_format
		(  );
	bool has_stencil_component
		( VkFormat fmt );

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

	void update_uniform_buffers
		( uint32_t currentImage );
	
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
