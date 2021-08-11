#ifndef ALLOCATOR_H
#define ALLOCATOR_H

#define MAX_FRAMES_PROCESSING 2

#include "device.h"

#include <vector>
#include <functional>

class allocator_c
{
	private:

	std::vector< std::function< void(  ) > > freeQueue;

	VkFormat find_depth_format
		(  );
	std::vector< char > read_file
		( const std::string& filePath );
	VkShaderModule create_shader_module	//	Wraps shader bytecode into objects for pipeline to work
		( const std::vector< char >& code );
	void transition_image_layout
		( VkImage image, VkImageLayout oldLayout, VkImageLayout newLayout );

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

	void submit
		( const auto&& func );

	public:

	std::vector< VkImage >* swapChainImages = NULL;
	VkRenderPass* renderPass = NULL;

	void init_allocator
		( std::vector< VkImage >& swapChainImagesIn );

	void init_image_view
		( VkImageView& imageView, VkImage image, VkFormat format, VkImageAspectFlags aspectFlags );
	void init_image
		( uint32_t width,
		  uint32_t height,
		  VkFormat format,
		  VkImageTiling tiling,
		  VkImageUsageFlags usage,
		  VkMemoryPropertyFlags properties,
		  VkImage& image,
		  VkDeviceMemory& imageMemory );
	void init_texture_image
		( const std::string& imagePath, VkImage& tImage, VkDeviceMemory& tImageMem );
	void init_texture_image_view
		( VkImageView& tImageView, VkImage tImage );

	template< typename T >
	void init_vertex_buffer
		( const std::vector< T >& v, VkBuffer& vBuffer, VkDeviceMemory& vBufferMem );
	void init_index_buffer
		( std::vector< uint32_t >& i, VkBuffer& iBuffer, VkDeviceMemory& iBufferMem );
	template< typename T >
        void init_uniform_buffers
		( std::vector< VkBuffer >& uBuffers, std::vector< VkDeviceMemory >& uBuffersMem );
	void init_desc_sets
		( std::vector< VkDescriptorSet >& descSets,
		  VkDescriptorSetLayout& descSetLayout,
		  VkDescriptorPool& descPool,
		  const std::vector< combined_image_info_t >& descImageInfos   = std::vector< combined_image_info_t >(  ),
		  const std::vector< combined_buffer_info_t >& descBufferInfos = std::vector< combined_buffer_info_t >(  ) );
	
	void init_image_views
		( std::vector< VkImageView >& swapChainImageViews, VkFormat& swapChainImageFormat );
	void init_render_pass
		( VkRenderPass& renderPass, VkFormat& swapChainImageFormat );
	void init_desc_set_layout
		( VkDescriptorSetLayout& descSetLayout, const std::vector< desc_set_layout_t >& bindings = std::vector< desc_set_layout_t >(  ) );
	template< typename T >
	void init_graphics_pipeline
		( VkPipeline& pipeline,
	  	  VkPipelineLayout& layout,
	  	  VkExtent2D& swapChainExtent,
	  	  VkDescriptorSetLayout& descSetLayout,
	  	  const std::string& vertShader,
	  	  const std::string& fragShader );
	void init_depth_resources
		( VkImage& depthImage, VkDeviceMemory& depthImageMemory, VkImageView& depthImageView, VkExtent2D& swapChainExtent );
	void init_frame_buffer
		( std::vector<VkFramebuffer>& swapChainFramebuffers,
		  std::vector<VkImageView>& swapChainImageViews,
		  VkImageView& depthImageView,
		  VkExtent2D& swapChainExtent );
	void init_desc_pool	//	please for the love of god, change this
		( VkDescriptorPool& descPool, std::vector< VkDescriptorPoolSize > poolSizes );
	void init_imgui_pool
		( SDL_Window* window );
	void init_sync
		( std::vector< VkSemaphore >& imageAvailableSemaphores,
		  std::vector< VkSemaphore >& renderFinishedSemaphores,
		  std::vector< VkFence >& inFlightFences,
		  std::vector< VkFence >& imagesInFlight );

	void free_resources
		(  );

	~allocator_c
		(  );

	device_c* dev;
};

#endif
