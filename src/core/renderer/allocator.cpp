#include "../../../inc/core/renderer/allocator.h"

#define STB_IMAGE_IMPLEMENTATION

#include <array>
#include <string>
#include <fstream>

#include "../../../lib/io/stb_image.h"

VkFormat allocator_c::find_depth_format
	(  )
{
	return dev->find_supported_fmt(
		{ VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
		VK_IMAGE_TILING_OPTIMAL,
        	VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT );
}

std::vector< char > allocator_c::read_file
	( const std::string& filePath )
{
	std::ifstream file( filePath, std::ios::ate | std::ios::binary );
	if ( !file.is_open(  ) ) {
		throw std::runtime_error( "Failed to open file!" );
	}
	int fileSize = ( int ) file.tellg(  );
	std::vector< char > buffer( fileSize );
	file.seekg( 0 );
	file.read( buffer.data(  ), fileSize );
	file.close(  );

	return buffer;
}

VkShaderModule allocator_c::create_shader_module	//	Wraps shader bytecode into objects for pipeline to work
	( const std::vector< char >& code )
{
	VkShaderModuleCreateInfo createInfo{  };
	VkShaderModule shaderModule;
	createInfo.sType 	= VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	createInfo.codeSize 	= code.size(  );
	createInfo.pCode 	= ( const uint32_t* )code.data(  );
	if ( vkCreateShaderModule( dev->dev(  ), &createInfo, NULL, &shaderModule ) != VK_SUCCESS )
	{
		throw std::runtime_error( "Failed to create shader module!" );
	}
	return shaderModule;
}

void allocator_c::transition_image_layout
	( VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout )
{
	VkCommandBuffer c = dev->begin_single_time_commands(  );

	VkImageMemoryBarrier barrier{  };
        barrier.sType 				= VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.oldLayout 			= oldLayout;
        barrier.newLayout 			= newLayout;
        barrier.srcQueueFamilyIndex 		= VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex 		= VK_QUEUE_FAMILY_IGNORED;
        barrier.image 				= image;
        barrier.subresourceRange.aspectMask 	= VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.baseMipLevel 	= 0;
        barrier.subresourceRange.levelCount 	= 1;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount 	= 1;

	VkPipelineStageFlags sourceStage;
	VkPipelineStageFlags destinationStage;

	if ( oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL )
	{
		barrier.srcAccessMask 	= 0;
		barrier.dstAccessMask 	= VK_ACCESS_TRANSFER_WRITE_BIT;

		sourceStage 		= VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		destinationStage	= VK_PIPELINE_STAGE_TRANSFER_BIT;
	}
	else if ( oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL )
	{
		barrier.srcAccessMask 	= VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask 	= VK_ACCESS_SHADER_READ_BIT;

		sourceStage 		= VK_PIPELINE_STAGE_TRANSFER_BIT;
		destinationStage 	= VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	}
	else if ( oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL )
	{
		barrier.srcAccessMask 	= 0;
		barrier.dstAccessMask 	= VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

		sourceStage 		= VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		destinationStage 	= VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	}
	else
	{
		throw std::invalid_argument( "Unsupported layout transition!" );
	}

	vkCmdPipelineBarrier(
		c,
		sourceStage, destinationStage,
		0,
		0,
		NULL,
		0,
		NULL,
		1, &barrier );

	dev->end_single_time_commands( c );
}

void allocator_c::init_buffer
		( VkDeviceSize size,
		  VkBufferUsageFlags usage,
		  VkMemoryPropertyFlags properties,
		  VkBuffer& buffer,
		  VkDeviceMemory& bufferMemory )
{
	VkBufferCreateInfo bufferInfo{  };
	bufferInfo.sType 	= VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.size 	= size;
	bufferInfo.usage 	= usage;
	bufferInfo.sharingMode  = VK_SHARING_MODE_EXCLUSIVE;

	if ( vkCreateBuffer( dev->dev(  ), &bufferInfo, NULL, &buffer ) != VK_SUCCESS )
	{
		throw std::runtime_error( "Failed to create buffer!" );
	}

	VkMemoryRequirements memRequirements;
	vkGetBufferMemoryRequirements( dev->dev(  ), buffer, &memRequirements );

	VkMemoryAllocateInfo allocInfo{  };
	allocInfo.sType 		= VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize 	= memRequirements.size;
	allocInfo.memoryTypeIndex 	= dev->find_memory_type( memRequirements.memoryTypeBits, properties );

	if ( vkAllocateMemory( dev->dev(  ), &allocInfo, nullptr, &bufferMemory ) != VK_SUCCESS )
	{
		throw std::runtime_error( "Failed to allocate buffer memory!" );
	}

	vkBindBufferMemory( dev->dev(  ), buffer, bufferMemory, 0 );
}

void allocator_c::map_memory
	( VkDeviceMemory bufMem, VkDeviceSize size, const void* in )
{
	void* data;
	vkMapMemory( dev->dev(  ), bufMem, 0, size, 0, &data );
	memcpy( data, in, ( size_t )size );
	vkUnmapMemory( dev->dev(  ), bufMem );
}

void allocator_c::buf_copy
	( VkBuffer src, VkBuffer dst, VkDeviceSize size )
{
	VkCommandBuffer commandBuffer = dev->begin_single_time_commands(  );
	
	VkBufferCopy copyRegion{  };
	copyRegion.srcOffset 	= 0; // Optional
	copyRegion.dstOffset 	= 0; // Optional
	copyRegion.size 	= size;
	vkCmdCopyBuffer( commandBuffer, src, dst, 1, &copyRegion );
	dev->end_single_time_commands( commandBuffer );
}

void allocator_c::copy_buffer_to_img
	( VkBuffer buffer, VkImage image, uint32_t width, uint32_t height )
{
	VkCommandBuffer c = dev->begin_single_time_commands(  );

	VkBufferImageCopy region{  };
	region.bufferOffset 		= 0;
	region.bufferRowLength 		= 0;
	region.bufferImageHeight 	= 0;

	region.imageSubresource.aspectMask 	= VK_IMAGE_ASPECT_COLOR_BIT;
	region.imageSubresource.mipLevel 	= 0;
	region.imageSubresource.baseArrayLayer  = 0;
	region.imageSubresource.layerCount 	= 1;

	region.imageOffset = { 0, 0, 0 };
	region.imageExtent = {
		width,
		height,
		1
	};

	vkCmdCopyBufferToImage(
		c,
		buffer,
		image,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		1,
		&region
		);

	dev->end_single_time_commands( c );
}

void allocator_c::init_image_view
	( VkImageView& imageView, VkImage image, VkFormat format, VkImageAspectFlags aspectFlags )
{
	VkImageViewCreateInfo viewInfo{  };
	viewInfo.sType 				 = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	viewInfo.image 				 = image;
	viewInfo.viewType 			 = VK_IMAGE_VIEW_TYPE_2D;
	viewInfo.format 			 = format;
	viewInfo.subresourceRange.aspectMask 	 = aspectFlags;
	viewInfo.subresourceRange.baseMipLevel 	 = 0;
	viewInfo.subresourceRange.levelCount 	 = 1;
	viewInfo.subresourceRange.baseArrayLayer = 0;
	viewInfo.subresourceRange.layerCount 	 = 1;

	if ( vkCreateImageView( dev->dev(  ), &viewInfo, NULL, &imageView ) != VK_SUCCESS )
	{
		throw std::runtime_error( "Failed to create texture image view!" );
	}
}

void allocator_c::init_image
	( uint32_t width,
	  uint32_t height,
	  VkFormat format,
	  VkImageTiling tiling,
	  VkImageUsageFlags usage,
	  VkMemoryPropertyFlags properties,
	  VkImage& image,
	  VkDeviceMemory& imageMemory )
{
	VkImageCreateInfo imageInfo{  };
	imageInfo.sType 	= VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageInfo.imageType 	= VK_IMAGE_TYPE_2D;
	imageInfo.extent.width 	= width;
	imageInfo.extent.height = height;
	imageInfo.extent.depth 	= 1;
	imageInfo.mipLevels 	= 1;
	imageInfo.arrayLayers 	= 1;
	imageInfo.format 	= format;
	imageInfo.tiling 	= tiling;
	imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	imageInfo.usage	 	= usage;
	imageInfo.samples 	= VK_SAMPLE_COUNT_1_BIT;
	imageInfo.sharingMode 	= VK_SHARING_MODE_EXCLUSIVE;

	if ( vkCreateImage( dev->dev(  ), &imageInfo, NULL, &image ) != VK_SUCCESS )
	{
		throw std::runtime_error( "Failed to create image!" );
	}

	VkMemoryRequirements memRequirements;
	vkGetImageMemoryRequirements( dev->dev(  ), image, &memRequirements );

	VkMemoryAllocateInfo allocInfo{  };
	allocInfo.sType 	  = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize  = memRequirements.size;
	allocInfo.memoryTypeIndex = dev->find_memory_type( memRequirements.memoryTypeBits, properties );

	if ( vkAllocateMemory( dev->dev(  ), &allocInfo, NULL, &imageMemory ) != VK_SUCCESS )
	{
		throw std::runtime_error( "Failed to allocate image memory!" );
	}

	vkBindImageMemory( dev->dev(  ), image, imageMemory, 0 );
}

void allocator_c::init_texture_image
	( const std::string& imagePath, VkImage& tImage, VkDeviceMemory& tImageMem )
{
	int texWidth, texHeight, texChannels;
	stbi_uc* pixels = stbi_load( imagePath.c_str(  ), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha );
	VkDeviceSize imageSize = texWidth * texHeight * 4;

	if ( !pixels ) {
		throw std::runtime_error( "Failed to load texture image!" );
	}

	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;
	init_buffer( imageSize,
		     VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		     stagingBuffer,
		     stagingBufferMemory );

	void* data;
	vkMapMemory( dev->dev(  ), stagingBufferMemory, 0, imageSize, 0, &data );
	memcpy( data, pixels, ( size_t )imageSize );
	vkUnmapMemory( dev->dev(  ), stagingBufferMemory );

	stbi_image_free( pixels );

	init_image( texWidth,
		    texHeight,
		    VK_FORMAT_R8G8B8A8_SRGB,
		    VK_IMAGE_TILING_OPTIMAL,
		    VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
		    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		    tImage,
		    tImageMem );
	transition_image_layout( tImage,
				 VK_FORMAT_R8G8B8A8_SRGB,
				 VK_IMAGE_LAYOUT_UNDEFINED,
				 VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL );
	copy_buffer_to_img( stagingBuffer,
			    tImage,
			    ( uint32_t )texWidth,
			    ( uint32_t )texHeight );
	transition_image_layout( tImage,
				 VK_FORMAT_R8G8B8A8_SRGB,
				 VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				 VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL );

	vkDestroyBuffer( dev->dev(  ), stagingBuffer, NULL );
	vkFreeMemory( dev->dev(  ), stagingBufferMemory, NULL );
}

void allocator_c::init_texture_image_view
	( VkImageView& tImageView, VkImage tImage )
{
	init_image_view( tImageView, tImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT );
}

template< typename T >
void allocator_c::init_vertex_buffer
	( const std::vector< T >& v, VkBuffer& vBuffer, VkDeviceMemory& vBufferMem )
{
	VkDeviceSize bufferSize = sizeof( v[ 0 ] ) * v.size(  );
	
	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;
	init_buffer( bufferSize,
		     VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
		     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		     stagingBuffer,
		     stagingBufferMemory );
	
	map_memory( stagingBufferMemory, bufferSize, v.data(  ) );

	init_buffer( bufferSize,
		     VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
		     VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		     vBuffer,
		     vBufferMem );

	buf_copy( stagingBuffer, vBuffer, bufferSize );

	vkDestroyBuffer( dev->dev(  ), stagingBuffer, NULL );
        vkFreeMemory( dev->dev(  ), stagingBufferMemory, NULL );
}

void allocator_c::init_index_buffer
	( std::vector< uint32_t >& i, VkBuffer& iBuffer, VkDeviceMemory& iBufferMem )	//	pls nuke this in the future, it is a copy of the above function
{
	VkDeviceSize bufferSize = sizeof( i[ 0 ] ) * i.size(  );

	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;
	init_buffer( bufferSize,
		     VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		     stagingBuffer, stagingBufferMemory );

	map_memory( stagingBufferMemory, bufferSize, i.data(  ) );

	init_buffer( bufferSize,
		     VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
		     VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		     iBuffer,
		     iBufferMem );

	buf_copy( stagingBuffer, iBuffer, bufferSize );

	vkDestroyBuffer( dev->dev(  ), stagingBuffer, NULL );
	vkFreeMemory( dev->dev(  ), stagingBufferMemory, NULL );
}

template< typename T >
void allocator_c::init_uniform_buffers
	( std::vector< VkBuffer >& uBuffers, std::vector< VkDeviceMemory >& uBuffersMem, std::vector< VkImage >& swapChainImages )
{
	VkDeviceSize bufferSize = sizeof( T );

	uBuffers.resize( swapChainImages.size(  ) );
	uBuffersMem.resize( swapChainImages.size(  ) );

	for ( int i = 0; i < swapChainImages.size(  ); i++ ) {
		init_buffer( bufferSize,
			     VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			     uBuffers[ i ],
			     uBuffersMem[ i ] );
	}
}

template< typename T >
void allocator_c::init_desc_sets
	( std::vector< VkDescriptorSet >& descSets,
	  std::vector< VkBuffer >& uBuffers,
	  VkImageView tImageView,
	  std::vector< VkImage >& swapChainImages,
	  VkDescriptorSetLayout& descSetLayout,
	  VkDescriptorPool descPool,
	  VkSampler textureSampler )
{
	std::vector< VkDescriptorSetLayout > layouts( swapChainImages.size(  ), descSetLayout );
	VkDescriptorSetAllocateInfo allocInfo{  };
	allocInfo.sType 		= VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool 	= descPool;
	allocInfo.descriptorSetCount 	= ( uint32_t )swapChainImages.size(  );
	allocInfo.pSetLayouts 		= layouts.data(  );

	descSets.resize( swapChainImages.size(  ) );
	if ( vkAllocateDescriptorSets( dev->dev(  ), &allocInfo, descSets.data(  ) ) != VK_SUCCESS )
	{
		throw std::runtime_error( "Failed to allocate descriptor sets!" );
	}

	for ( int i = 0; i < swapChainImages.size(  ); i++ )
	{
		VkDescriptorBufferInfo bufferInfo{  };
		bufferInfo.buffer = uBuffers[ i ];
		bufferInfo.offset = 0;
	        bufferInfo.range  = sizeof( T );

		VkDescriptorImageInfo imageInfo{  };
		imageInfo.imageLayout   = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		imageInfo.imageView 	= tImageView;
		imageInfo.sampler 	= textureSampler;

		std::array< VkWriteDescriptorSet, 2 > descriptorWrites{  };
		descriptorWrites[ 0 ].sType 		 = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[ 0 ].dstSet 		 = descSets[ i ];
		descriptorWrites[ 0 ].dstBinding 	 = 0;
		descriptorWrites[ 0 ].dstArrayElement  	 = 0;
		descriptorWrites[ 0 ].descriptorType   	 = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		descriptorWrites[ 0 ].descriptorCount  	 = 1;
		descriptorWrites[ 0 ].pBufferInfo 	 = &bufferInfo;

		descriptorWrites[ 1 ].sType 		= VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[ 1 ].dstSet 		= descSets[ i ];
		descriptorWrites[ 1 ].dstBinding 	= 1;
		descriptorWrites[ 1 ].dstArrayElement 	= 0;
		descriptorWrites[ 1 ].descriptorType 	= VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		descriptorWrites[ 1 ].descriptorCount	= 1;
		descriptorWrites[ 1 ].pImageInfo 	= &imageInfo;

		vkUpdateDescriptorSets( dev->dev(  ), ( uint32_t )descriptorWrites.size(  ), descriptorWrites.data(  ), 0, NULL );
	}
}

void allocator_c::init_image_views
	( std::vector< VkImage >& swapChainImages, std::vector< VkImageView >& swapChainImageViews, VkFormat& swapChainImageFormat )
{
	swapChainImageViews.resize( swapChainImages.size(  ) );
	for ( int i = 0; i < swapChainImages.size(  ); i++ )
	{
		init_image_view( swapChainImageViews[ i ], swapChainImages[ i ], swapChainImageFormat, VK_IMAGE_ASPECT_COLOR_BIT );
	}
}

void allocator_c::init_render_pass
	( VkRenderPass& renderPass, VkFormat& swapChainImageFormat )
{
	VkAttachmentDescription colorAttachment{  };
	colorAttachment.format 		= swapChainImageFormat;
	colorAttachment.samples 	= VK_SAMPLE_COUNT_1_BIT;
	colorAttachment.loadOp 		= VK_ATTACHMENT_LOAD_OP_CLEAR;
	colorAttachment.storeOp 	= VK_ATTACHMENT_STORE_OP_STORE;
	colorAttachment.stencilLoadOp 	= VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachment.stencilStoreOp 	= VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colorAttachment.initialLayout 	= VK_IMAGE_LAYOUT_UNDEFINED;
	colorAttachment.finalLayout 	= VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	VkAttachmentDescription depthAttachment{  };
	depthAttachment.format 		= find_depth_format(  );
	depthAttachment.samples 	= VK_SAMPLE_COUNT_1_BIT;
	depthAttachment.loadOp 		= VK_ATTACHMENT_LOAD_OP_CLEAR;
	depthAttachment.storeOp 	= VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttachment.stencilLoadOp 	= VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	depthAttachment.stencilStoreOp 	= VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttachment.initialLayout 	= VK_IMAGE_LAYOUT_UNDEFINED;
	depthAttachment.finalLayout 	= VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkAttachmentReference colorAttachmentRef{  };
	colorAttachmentRef.attachment 	= 0;	//	Referenced by index, so 0 means color is referenced first since type is ambiguous
	colorAttachmentRef.layout 	= VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkAttachmentReference depthAttachmentRef{  };
	depthAttachmentRef.attachment 	= 1;
	depthAttachmentRef.layout 	= VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpass{  };	//	Smaller render operations, store them here
	subpass.pipelineBindPoint 	= VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount 	= 1;
	subpass.pColorAttachments 	= &colorAttachmentRef;
	subpass.pDepthStencilAttachment = &depthAttachmentRef;

	VkSubpassDependency dependency{  };
	dependency.srcSubpass 		= VK_SUBPASS_EXTERNAL;
	dependency.dstSubpass 		= 0;
	dependency.srcStageMask 	= VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	dependency.srcAccessMask 	= 0;
	dependency.dstStageMask 	= VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	dependency.dstAccessMask 	= VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

	std::array< VkAttachmentDescription, 2 > attachments = { colorAttachment, depthAttachment };
	VkRenderPassCreateInfo renderPassInfo{  };
	renderPassInfo.sType 		= VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassInfo.attachmentCount 	= ( uint32_t )attachments.size(  );
	renderPassInfo.pAttachments 	= attachments.data(  );
	renderPassInfo.subpassCount	= 1;
	renderPassInfo.pSubpasses 	= &subpass;
	renderPassInfo.dependencyCount 	= 1;
	renderPassInfo.pDependencies 	= &dependency;

	if ( vkCreateRenderPass( dev->dev(  ), &renderPassInfo, NULL, &renderPass ) != VK_SUCCESS )
	{
		throw std::runtime_error( "Failed to create render pass!" );
	}
}

void allocator_c::init_desc_set_layout
	( VkDescriptorSetLayout& descSetLayout )
{
	VkDescriptorSetLayoutBinding uboLayoutBinding{  };
	uboLayoutBinding.binding 		= 0;
	uboLayoutBinding.descriptorType 	= VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	uboLayoutBinding.descriptorCount 	= 1;
	uboLayoutBinding.stageFlags 		= VK_SHADER_STAGE_VERTEX_BIT;
	uboLayoutBinding.pImmutableSamplers 	= NULL; // Optional

	VkDescriptorSetLayoutBinding samplerLayoutBinding{  };
	samplerLayoutBinding.binding 		= 1;
	samplerLayoutBinding.descriptorCount 	= 1;
	samplerLayoutBinding.descriptorType 	= VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	samplerLayoutBinding.pImmutableSamplers = NULL;
	samplerLayoutBinding.stageFlags 	= VK_SHADER_STAGE_FRAGMENT_BIT;

	std::array< VkDescriptorSetLayoutBinding, 2 > bindings = { uboLayoutBinding, samplerLayoutBinding };
	VkDescriptorSetLayoutCreateInfo layoutInfo{  };
	layoutInfo.sType 			= VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.bindingCount 		= ( uint32_t )bindings.size(  );
	layoutInfo.pBindings 			= bindings.data(  );

	if ( vkCreateDescriptorSetLayout( dev->dev(  ), &layoutInfo, NULL, &descSetLayout ) != VK_SUCCESS )
	{
		throw std::runtime_error( "Failed to create descriptor set layout!" );
	}
}

template< typename T >
void allocator_c::init_graphics_pipeline
	( VkPipeline& pipeline,
	  VkPipelineLayout& layout,
	  VkExtent2D& swapChainExtent,
	  VkDescriptorSetLayout& descSetLayout,
	  VkRenderPass& renderPass,
	  const std::string& vertShader,
	  const std::string& fragShader )
{
	auto vertShaderCode = read_file( vertShader );
	auto fragShaderCode = read_file( fragShader );
		
	VkShaderModule vertShaderModule = create_shader_module( vertShaderCode );	//	Processes incoming verticies, taking world position, color, and texture coordinates as an input
	VkShaderModule fragShaderModule = create_shader_module( fragShaderCode );	//	Fills verticies with fragments to produce color, and depth

	VkPipelineShaderStageCreateInfo vertShaderStageInfo{  };
	vertShaderStageInfo.sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;	//	Tells Vulkan which stage the shader is going to be used
	vertShaderStageInfo.stage  = VK_SHADER_STAGE_VERTEX_BIT;
	vertShaderStageInfo.module = vertShaderModule;
	vertShaderStageInfo.pName  = "main";							//	void main() in the shader to be executed

	VkPipelineShaderStageCreateInfo fragShaderStageInfo{  };
	fragShaderStageInfo.sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	fragShaderStageInfo.stage  = VK_SHADER_STAGE_FRAGMENT_BIT;
	fragShaderStageInfo.module = fragShaderModule;
	fragShaderStageInfo.pName  = "main";

	VkPipelineShaderStageCreateInfo shaderStages[  ] = { vertShaderStageInfo, fragShaderStageInfo };

	auto attributeDescriptions 	= T::get_attribute_desc(  );
	auto bindingDescription 	= T::get_binding_desc(  );      

	VkPipelineVertexInputStateCreateInfo vertexInputInfo{  };	//	Format of vertex data
	vertexInputInfo.sType 				= VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertexInputInfo.vertexBindingDescriptionCount 	= 1;
	vertexInputInfo.pVertexBindingDescriptions 	= &bindingDescription;			//	Contains details for loading vertex data
	vertexInputInfo.vertexAttributeDescriptionCount = ( uint32_t )( attributeDescriptions.size(  ) );
	vertexInputInfo.pVertexAttributeDescriptions 	= attributeDescriptions.data(  );	//	Same as above

	VkPipelineInputAssemblyStateCreateInfo inputAssembly{  };	//	Collects raw vertex data from buffers
	inputAssembly.sType 			= VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssembly.topology 			= VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	inputAssembly.primitiveRestartEnable 	= VK_FALSE;

	VkViewport viewport{  };	//	Region of frambuffer to be rendered to, likely will always use 0, 0 and width, height
	viewport.x		= 0.0f;
	viewport.y 		= 0.0f;
	viewport.width 		= ( float )swapChainExtent.width;
	viewport.height 	= ( float )swapChainExtent.height;
	viewport.minDepth 	= 0.0f;
	viewport.maxDepth 	= 1.0f;

	VkRect2D scissor{  };		//	More agressive cropping than viewport, defining which regions pixels are to be stored
	scissor.offset = { 0, 0 };
	scissor.extent = swapChainExtent;

	VkPipelineViewportStateCreateInfo viewportState{  };	//	Combines viewport and scissor
	viewportState.sType 		= VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportState.viewportCount 	= 1;
	viewportState.pViewports 	= &viewport;
	viewportState.scissorCount 	= 1;
	viewportState.pScissors 	= &scissor;

	VkPipelineRasterizationStateCreateInfo rasterizer{  };		//	Turns  primitives into fragments, aka, pixels for the framebuffer
	rasterizer.sType 			= VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizer.depthClampEnable 		= VK_FALSE;
	rasterizer.rasterizerDiscardEnable 	= VK_FALSE;
	rasterizer.polygonMode 			= VK_POLYGON_MODE_FILL;		//	Fill with fragments, can optionally use VK_POLYGON_MODE_LINE for a wireframe
	rasterizer.lineWidth 			= 1.0f;
	rasterizer.cullMode 			= VK_CULL_MODE_BACK_BIT;
	rasterizer.frontFace 			= VK_FRONT_FACE_COUNTER_CLOCKWISE;
	rasterizer.depthBiasEnable 		= VK_FALSE;
	rasterizer.depthBiasConstantFactor 	= 0.0f; // Optional
	rasterizer.depthBiasClamp 		= 0.0f; // Optional
	rasterizer.depthBiasSlopeFactor 	= 0.0f; // Optional

	VkPipelineMultisampleStateCreateInfo multisampling{  };		//	Performs anti-aliasing
	multisampling.sType 		    = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampling.sampleShadingEnable   = VK_FALSE;
	multisampling.rasterizationSamples  = VK_SAMPLE_COUNT_1_BIT;
	multisampling.minSampleShading 	    = 1.0f; // Optional
	multisampling.pSampleMask 	    = NULL; // Optional
	multisampling.alphaToCoverageEnable = VK_FALSE; // Optional
	multisampling.alphaToOneEnable 	    = VK_FALSE; // Optional

	VkPipelineColorBlendAttachmentState colorBlendAttachment{  };
	colorBlendAttachment.colorWriteMask 	 = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	colorBlendAttachment.blendEnable 	 = VK_TRUE;
	colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
	colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	colorBlendAttachment.colorBlendOp	 = VK_BLEND_OP_ADD;
	colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	colorBlendAttachment.alphaBlendOp 	 = VK_BLEND_OP_ADD;

	VkPipelineDepthStencilStateCreateInfo depthStencil{  };
	depthStencil.sType 			= VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	depthStencil.depthTestEnable 		= VK_TRUE;
	depthStencil.depthWriteEnable		= VK_TRUE;
	depthStencil.depthCompareOp 		= VK_COMPARE_OP_LESS;
	depthStencil.depthBoundsTestEnable 	= VK_FALSE;
	depthStencil.minDepthBounds 		= 0.0f; // Optional
	depthStencil.maxDepthBounds 		= 1.0f; // Optional
	depthStencil.stencilTestEnable 		= VK_FALSE;
	depthStencil.front 			= {  }; // Optional
	depthStencil.back 			= {  }; // Optional

	VkPipelineColorBlendStateCreateInfo colorBlending{  };
	colorBlending.sType 		  = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlending.logicOpEnable 	  = VK_FALSE;
	colorBlending.logicOp 	          = VK_LOGIC_OP_COPY; // Optional
	colorBlending.attachmentCount 	  = 1;
	colorBlending.pAttachments 	  = &colorBlendAttachment;
	colorBlending.blendConstants[ 0 ] = 0.0f; // Optional
	colorBlending.blendConstants[ 1 ] = 0.0f; // Optional
	colorBlending.blendConstants[ 2 ] = 0.0f; // Optional
	colorBlending.blendConstants[ 3 ] = 0.0f; // Optional

	VkDynamicState dynamicStates[  ] = {	//	Allows for some pipeline configuration values to change during runtime
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_LINE_WIDTH
	};

	VkPipelineDynamicStateCreateInfo dynamicState{  };
	dynamicState.sType 		= VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamicState.dynamicStateCount  = 2;
	dynamicState.pDynamicStates 	= dynamicStates;

	VkPipelineLayoutCreateInfo pipelineLayoutInfo{  };	//	Allows for use of uniform values
	pipelineLayoutInfo.sType 			= VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount 		= 1; // Optional
	pipelineLayoutInfo.pSetLayouts 			= &descSetLayout; // Optional

	if ( vkCreatePipelineLayout( dev->dev(  ), &pipelineLayoutInfo, NULL, &layout ) != VK_SUCCESS )
	{
		throw std::runtime_error( "Failed to create pipeline layout!" );
	}
	VkGraphicsPipelineCreateInfo pipelineInfo{  };	//	Combine all the objects above into one parameter for graphics pipeline creation
	pipelineInfo.sType 			= VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineInfo.stageCount 		= 2;
	pipelineInfo.pStages 			= shaderStages;
	pipelineInfo.pVertexInputState 		= &vertexInputInfo;
	pipelineInfo.pInputAssemblyState 	= &inputAssembly;
	pipelineInfo.pViewportState 		= &viewportState;
	pipelineInfo.pRasterizationState 	= &rasterizer;
	pipelineInfo.pMultisampleState 		= &multisampling;
	pipelineInfo.pDepthStencilState 	= &depthStencil;
	pipelineInfo.pColorBlendState 		= &colorBlending;
	pipelineInfo.pDynamicState 		= NULL; // Optional
	pipelineInfo.layout 			= layout;
	pipelineInfo.renderPass 		= renderPass;
	pipelineInfo.subpass 			= 0;
	pipelineInfo.basePipelineHandle 	= VK_NULL_HANDLE; // Optional, very important for later when making new pipelines. It is less expensive to reference an existing similar pipeline
	pipelineInfo.basePipelineIndex 		= -1; // Optional

	if ( vkCreateGraphicsPipelines( dev->dev(  ), VK_NULL_HANDLE, 1, &pipelineInfo, NULL, &pipeline ) != VK_SUCCESS )
	{
		throw std::runtime_error( "Failed to create graphics pipeline!" );
	}

	vkDestroyShaderModule( dev->dev(  ), fragShaderModule, NULL );
	vkDestroyShaderModule( dev->dev(  ), vertShaderModule, NULL );
}

void allocator_c::init_depth_resources
	( VkImage& depthImage, VkDeviceMemory& depthImageMemory, VkImageView& depthImageView, VkExtent2D& swapChainExtent )
{
	VkFormat depthFormat = dev->find_depth_format(  );
	init_image( swapChainExtent.width,
		    swapChainExtent.height,
		    depthFormat,
		    VK_IMAGE_TILING_OPTIMAL,
		    VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
		    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		    depthImage,
		    depthImageMemory );
	init_image_view( depthImageView, depthImage, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT );
	transition_image_layout( depthImage,
				 depthFormat,
				 VK_IMAGE_LAYOUT_UNDEFINED,
				 VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL );
}

void allocator_c::init_frame_buffer
	( std::vector<VkFramebuffer>& swapChainFramebuffers,
	  std::vector<VkImageView>& swapChainImageViews,
	  VkImageView& depthImageView,
	  VkRenderPass& renderPass,
	  VkExtent2D& swapChainExtent )
{
	swapChainFramebuffers.resize( swapChainImageViews.size(  ) );
	for ( int i = 0; i < swapChainImageViews.size(  ); i++ )
	{
		std::array< VkImageView, 2 > attachments = {
			swapChainImageViews[ i ],
			depthImageView
		};

		VkFramebufferCreateInfo framebufferInfo{  };
		framebufferInfo.sType 		= VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferInfo.renderPass 	= renderPass;
		framebufferInfo.attachmentCount = ( uint32_t )attachments.size(  );
		framebufferInfo.pAttachments 	= attachments.data(  );
		framebufferInfo.width 		= swapChainExtent.width;
		framebufferInfo.height 		= swapChainExtent.height;
		framebufferInfo.layers 		= 1;

		if ( vkCreateFramebuffer( dev->dev(  ), &framebufferInfo, NULL, &swapChainFramebuffers[ i ] ) != VK_SUCCESS )
		{
			throw std::runtime_error( "Failed to create framebuffer!" );
		}
	}
}

void allocator_c::init_desc_pool	//	please for the love of god, change this
	( VkDescriptorPool& descPool )
{
	std::array< VkDescriptorPoolSize, 2 > poolSizes{  };
	poolSizes[ 0 ].type 		 = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	poolSizes[ 0 ].descriptorCount 	 = 2000;//( uint32_t )swapChainImages.size(  );
	poolSizes[ 1 ].type 		 = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	poolSizes[ 1 ].descriptorCount 	 = 2000;//( uint32_t )swapChainImages.size(  );

	VkDescriptorPoolCreateInfo poolInfo{  };
	poolInfo.sType 		= VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolInfo.poolSizeCount  = ( uint32_t )poolSizes.size(  );
	poolInfo.pPoolSizes 	= poolSizes.data(  );
	poolInfo.maxSets 	= 2000;//( uint32_t )swapChainImages.size(  );

	if ( vkCreateDescriptorPool( dev->dev(  ), &poolInfo, NULL, &descPool ) != VK_SUCCESS )
	{
		throw std::runtime_error( "Failed to create descriptor pool!" );
	}
}

void allocator_c::init_sync
	( std::vector< VkSemaphore >& imageAvailableSemaphores,
	  std::vector< VkSemaphore >& renderFinishedSemaphores,
	  std::vector< VkFence >& inFlightFences,
	  std::vector< VkFence >& imagesInFlight,
	  std::vector<VkImage>& swapChainImages  )
{
	imageAvailableSemaphores.resize( MAX_FRAMES_PROCESSING );
	renderFinishedSemaphores.resize( MAX_FRAMES_PROCESSING );
	inFlightFences.resize( MAX_FRAMES_PROCESSING );
	imagesInFlight.resize( swapChainImages.size(  ), VK_NULL_HANDLE );
	
	VkSemaphoreCreateInfo semaphoreInfo{  };
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	VkFenceCreateInfo fenceInfo{  };
	fenceInfo.sType	    = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceInfo.flags	    = VK_FENCE_CREATE_SIGNALED_BIT;

	for ( int i = 0 ; i < MAX_FRAMES_PROCESSING; ++i )
	{
		if ( vkCreateSemaphore( dev->dev(  ), &semaphoreInfo, NULL, &imageAvailableSemaphores[ i ] ) != VK_SUCCESS ||
		     vkCreateSemaphore( dev->dev(  ), &semaphoreInfo, NULL, &renderFinishedSemaphores[ i ] ) != VK_SUCCESS ||
		     vkCreateFence( dev->dev(  ), &fenceInfo, NULL, &inFlightFences[ i ] ) != VK_SUCCESS )
		{
			throw std::runtime_error( "Failed to create sync objects!" );
		}
	}
}

template void allocator_c::init_vertex_buffer< vertex_2d_t >( const std::vector< vertex_2d_t >&, VkBuffer&, VkDeviceMemory& );
template void allocator_c::init_vertex_buffer< vertex_3d_t >( const std::vector< vertex_3d_t >&, VkBuffer&, VkDeviceMemory& );
template void allocator_c::init_uniform_buffers< ubo_2d_t >( std::vector< VkBuffer >&, std::vector< VkDeviceMemory >&, std::vector< VkImage >& );
template void allocator_c::init_uniform_buffers< ubo_3d_t >( std::vector< VkBuffer >&, std::vector< VkDeviceMemory >&, std::vector< VkImage >& );
template void allocator_c::init_desc_sets< ubo_2d_t >( std::vector< VkDescriptorSet >&,
						       std::vector< VkBuffer >&,
						       VkImageView tImageView,
						       std::vector< VkImage >&,
						       VkDescriptorSetLayout&,
						       VkDescriptorPool,
						       VkSampler );
template void allocator_c::init_desc_sets< ubo_3d_t >( std::vector< VkDescriptorSet >&,
						       std::vector< VkBuffer >&,
						       VkImageView tImageView,
						       std::vector< VkImage >&,
						       VkDescriptorSetLayout&,
						       VkDescriptorPool,
						       VkSampler );
template void allocator_c::init_graphics_pipeline< vertex_2d_t >( VkPipeline&,
								  VkPipelineLayout&,
								  VkExtent2D&,
								  VkDescriptorSetLayout&,
								  VkRenderPass&,
								  const std::string&,
								  const std::string&  );
template void allocator_c::init_graphics_pipeline< vertex_3d_t >( VkPipeline&,
								  VkPipelineLayout&,
								  VkExtent2D&,
								  VkDescriptorSetLayout&,
								  VkRenderPass&,
								  const std::string&,
								  const std::string&  );
