#ifndef RENDERER_H
#define RENDERER_H

#define GLM_ENABLE_EXPERIMENTAL

#include "../system.h"
#include "../../types/enums.h"
#include "../../types/renderertypes.h"

#include "device.h"

#include <vulkan/vulkan.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_vulkan.h>
#include <glm/glm.hpp>
#include <glm/gtx/hash.hpp>
#include <array>

#include <optional>

#define USE_3D_VERTEX 1 << 0
#define USE_2D_VERTEX 1 << 1

/*
#ifdef NDEBUG
    const bool enableValidationLayers = false;
#else
    const bool enableValidationLayers = true;
#endif
*/
const int MAX_FRAMES_PROCESSING = 2;
/*
const std::vector< const char* > validationLayers = {
    "VK_LAYER_KHRONOS_validation"
};

const std::vector< const char* > deviceExtensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME
};
*/
const std::string MODELPATH = "materials/models/protogen_wip_5_plus_protodal.obj";
const std::string TEXTUREPATH = "materials/textures/blank_mat.png";

class renderer_c : public system_c	//	Most of these objects are used to make new objects by initializing them with certain parameters, then creating and storing them
{
	protected:

	//allocator_c allocator;
	
	int width = 1280, height = 720;
	int currentFrame = 0;

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

	VkImageView textureImageView;
	VkSampler textureSampler;

	VkImage depthImage;
	VkDeviceMemory depthImageMemory;
	VkImageView depthImageView;
	
	VkPipelineLayout modelLayout;
	VkPipelineLayout spriteLayout;
	VkPipeline modelPipeline;					//	Colossal object that handles most stages of rendering. Seems to need multiple to render multiple things.
	VkPipeline spritePipeline;
	VkCommandPool commandPool;					//	Manage memory related to command buffers
	
	std::vector< VkSemaphore > imageAvailableSemaphores, renderFinishedSemaphores;
	std::vector< VkFence > inFlightFences, imagesInFlight;

	void init_commands
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
	template< typename T >
	void init_graphics_pipeline			//	Initialize the graphics pipeline to load a shader and various other tools for rendering
		( VkPipeline& pipeline, VkPipelineLayout& layout, const std::string& vertShader, const std::string& fragShader );					//	TODO: create parameters to specify a shader, as the rest done in this function doesn't need to be changed for multiple pipelines
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
		( const std::string& imagePath, VkImage& tImage, VkDeviceMemory& tImageMem );
	void init_texture_image_view
		( VkImageView& tImageView, VkImage tImage );
	void init_texture_sampler
		(  );
	void init_model_vertices
		( const std::string& modelPath, model_data_t& model );
	void init_sprite_vertices
		( const std::string& spritePath, sprite_data_t& sprite );
	void init_image
		( uint32_t width,
		  uint32_t height,
		  VkFormat format,
		  VkImageTiling tiling,
		  VkImageUsageFlags usage,
		  VkMemoryPropertyFlags properties,
		  VkImage& image,
		  VkDeviceMemory& imageMemory );
	template< typename T >
	void update_vertex_buffer			//	Initialize device memory with vertex data, may need to be called each time new vertices are loaded
		( const std::vector< T >& v, VkBuffer& vBuffer, VkDeviceMemory& vBufferMem );
	void update_index_buffer			//	Initialize device memory with index data, may need to be called each time new vertices are loaded
		( std::vector< uint32_t >&i, VkBuffer& iBuffer, VkDeviceMemory& iBufferMem );
	template< typename T >
	void init_uniform_buffers			//	Pass arbitrary attributes to vertex shader for each vertex, allows vertex positioning without remapping memory
		( std::vector< VkBuffer >& uBuffers, std::vector< VkDeviceMemory >& uBuffersMem );
	void init_desc_pool
		(  );
	template< typename T >
	void init_desc_sets
		( std::vector< VkDescriptorSet >& descSets, std::vector< VkBuffer >& uBuffers, VkImageView tImageView );
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
		( uint32_t currentImage, model_data_t& modelData );
	void update_sprite_uniform_buffers
		( uint32_t currentImage, sprite_data_t& spriteData );
	
	void cleanup
		(  );
	
	public:

	void init_vulkan
		(  );
	void init_model
		( model_data_t& modelData, const std::string& modelPath, const std::string& texturePath );
	void init_sprite
		( sprite_data_t& spriteData, const std::string& spritePath );

	void draw_frame
		(  );

	std::vector< model_data_t >* models;
	std::vector< sprite_data_t >* sprites;
	
	renderer_c
		(  );
	~renderer_c
		(  );
};

#endif
