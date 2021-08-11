#ifndef RENDERER_H
#define RENDERER_H

#define GLM_ENABLE_EXPERIMENTAL

#include "../../shared/system.h"
#include "../../types/enums.h"
#include "../../types/renderertypes.h"
#include "../gui.h"

#include "allocator.h"

#include <vulkan/vulkan.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_vulkan.h>
#include <glm/glm.hpp>
#include <glm/gtx/hash.hpp>
#include <array>

#include <optional>

class renderer_c : public system_c	//	Most of these objects are used to make new objects by initializing them with certain parameters, then creating and storing them
{
	protected:

	bool imGuiInitialized = false;

	device_c device;
	allocator_c allocator;
	
	int currentFrame = 0;

	VkSwapchainKHR swapChain;					//	Queue for stuff to be rendered, does some processing before drawn to screen
	
	std::vector<VkImageView> swapChainImageViews;			//	View into an image, describing which part to access, one needed for each image
	std::vector<VkImage> swapChainImages;				//	Stores images to be rendered, can have many
	std::vector<VkFramebuffer> swapChainFramebuffers;		//	
	std::vector<VkCommandBuffer> commandBuffers;			//	Send commands to these to be executed later, better for concurrency, so many are nice
	
	VkFormat swapChainImageFormat;
	VkExtent2D swapChainExtent;
	VkRenderPass renderPass;					//	Stores framebuffer attachments that will be used for rendering

	VkDescriptorSetLayout modelSetLayout;
	VkDescriptorSetLayout spriteSetLayout;
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
	
	std::vector< VkSemaphore > imageAvailableSemaphores, renderFinishedSemaphores;
	std::vector< VkFence > inFlightFences, imagesInFlight;

	void init_commands
		(  );
	
	void init_command_buffers
		(  );
	void init_texture_image
		( const std::string& imagePath, VkImage& tImage, VkDeviceMemory& tImageMem );
	void init_texture_image_view
		( VkImageView& tImageView, VkImage tImage );
	void load_obj
		( const std::string& objPath, model_data_t& model );
	void load_gltf
		( const std::string& gltfPath, model_data_t& model );
	void init_model_vertices
		( const std::string& modelPath, model_data_t& model );
	void init_sprite_vertices
		( const std::string& spritePath, sprite_data_t& sprite );
	
	void reinit_swap_chain
		(  );
	void destroy_swap_chain
		(  );

	bool has_stencil_component
		( VkFormat fmt );
	
	template< typename T >
	void destroy_renderable
		( T& renderable );
	
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

	std::vector< model_data_t* >* models;
	std::vector< sprite_data_t* >* sprites;
	
	renderer_c
		(  );
	void send_messages
		(  );
	~renderer_c
		(  );
};

#endif
