#ifndef ALLOCATOR_H
#define ALLOCATOR_H

#define MAX_FRAMES_PROCESSING 2

#include "device.h"

#include <vector>

class allocator_c
{
	private:

	VkFormat find_depth_format
		(  );
	std::vector< char > read_file
		( const std::string& filePath );
	VkShaderModule create_shader_module	//	Wraps shader bytecode into objects for pipeline to work
		( const std::vector< char >& code );
	void transition_image_layout
		( VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout );

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

	public:

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
	
	void init_image_views
		( std::vector< VkImage >& swapChainImages, std::vector< VkImageView >& swapChainImageViews, VkFormat& swapChainImageFormat );
	void init_render_pass
		( VkRenderPass& renderPass, VkFormat& swapChainImageFormat );
	void init_desc_set_layout
		( VkDescriptorSetLayout& descSetLayout );
	template< typename T >
	void init_graphics_pipeline
		( VkPipeline& pipeline,
	  	  VkPipelineLayout& layout,
	  	  VkExtent2D& swapChainExtent,
	  	  VkDescriptorSetLayout& descSetLayout,
	  	  VkRenderPass& renderPass,
	  	  const std::string& vertShader,
	  	  const std::string& fragShader);
	void init_depth_resources
		( VkImage& depthImage, VkDeviceMemory& depthImageMemory, VkImageView& depthImageView, VkExtent2D& swapChainExtent );
	void init_frame_buffer
		( std::vector<VkFramebuffer>& swapChainFramebuffers,
		  std::vector<VkImageView>& swapChainImageViews,
		  VkImageView& depthImageView,
		  VkRenderPass& renderPass,
		  VkExtent2D& swapChainExtent );
	void init_desc_pool	//	please for the love of god, change this
		( VkDescriptorPool& descPool );
	void init_sync
		( std::vector< VkSemaphore >& imageAvailableSemaphores,
		  std::vector< VkSemaphore >& renderFinishedSemaphores,
		  std::vector< VkFence >& inFlightFences,
		  std::vector< VkFence >& imagesInFlight,
		  std::vector<VkImage>& swapChainImages );

	device_c dev;
};

#endif
