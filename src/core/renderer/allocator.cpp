#include "../../../inc/core/renderer/allocator.h"
#include "../../../lib/io/stb_image.h"

FunctionList 			gFreeQueue;
ShaderCache			gShaderCache;
DescriptorCache			gDescriptorCache;

Device 				*gpDevice 		= new Device;
ImageSet 			*gpSwapChainImages 	= NULL;
VkRenderPass 			*gpRenderPass 		= NULL;
VkDescriptorPool	        *gpPool 		= NULL;

VkFormat FindDepthFormat(  )
{
	return gpDevice->FindSupportedFormat(
		{ VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
		VK_IMAGE_TILING_OPTIMAL,
        	VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT );
}
/* Immediately submits a command to a command buffer.  */
void Submit( const auto&& sFunction )
{
	/* Open command buffer.  */
	VkCommandBuffer c = gpDevice->BeginSingleTimeCommands(  );
	sFunction( c );
	/* Complete command recording.  */
	gpDevice->EndSingleTimeCommands( c );
}
/* Reads a file into a byte array.  */
ByteArray ReadFile( const String &srFilePath )
{
	/* Open file.  */
	std::ifstream file( srFilePath, std::ios::ate | std::ios::binary );
	if ( !file.is_open(  ) )
		throw std::runtime_error( "Failed to open file!" );
	int fileSize = ( int )file.tellg(  );
        ByteArray buffer( fileSize );
	file.seekg( 0 );
	/* Read contents.  */
	file.read( buffer.data(  ), fileSize );
	file.close(  );

	return buffer;
}
/* Wraps bytecode into objects for the pipeline to use.  */
VkShaderModule CreateShaderModule( const ByteArray &srCode )
{
	VkShaderModuleCreateInfo 	createInfo{  };
	VkShaderModule 			shaderModule;
	createInfo.sType 	= VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	createInfo.codeSize 	= srCode.size(  );
	createInfo.pCode 	= ( const uint32_t* )srCode.data(  );
	if ( vkCreateShaderModule( DEVICE, &createInfo, NULL, &shaderModule ) != VK_SUCCESS )
		throw std::runtime_error( "Failed to create shader module!" );
	return shaderModule;
}

void TransitionImageLayout( VkImage sImage, VkImageLayout sOldLayout, VkImageLayout sNewLayout )
{
	VkImageMemoryBarrier 	barrier	= ImageMemoryBarrier( sImage, sOldLayout, sNewLayout );
	VkPipelineStageFlags 	sourceStage;
	VkPipelineStageFlags 	destinationStage;

	if ( sOldLayout == VK_IMAGE_LAYOUT_UNDEFINED && sNewLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL )
	{
		barrier.srcAccessMask 	= 0;
		barrier.dstAccessMask 	= VK_ACCESS_TRANSFER_WRITE_BIT;

		sourceStage 		= VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		destinationStage	= VK_PIPELINE_STAGE_TRANSFER_BIT;
	}
	else if ( sOldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && sNewLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL )
	{
		barrier.srcAccessMask 	= VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask 	= VK_ACCESS_SHADER_READ_BIT;

		sourceStage 		= VK_PIPELINE_STAGE_TRANSFER_BIT;
		destinationStage 	= VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	}
	else if ( sOldLayout == VK_IMAGE_LAYOUT_UNDEFINED && sNewLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL )
	{
		barrier.srcAccessMask 	= 0;
		barrier.dstAccessMask 	= VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

		sourceStage 		= VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		destinationStage 	= VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	}
	else
		throw std::invalid_argument( "Unsupported layout transition!" );
	/* Submit to the GPU.  */
	Submit( [ & ]( VkCommandBuffer c ){ vkCmdPipelineBarrier( c, sourceStage, destinationStage, 0, 0, NULL, 0, NULL, 1, &barrier ); } );
}
/* Creates a buffer and maps the memory.  */
void InitBuffer( VkDeviceSize sSize, VkBufferUsageFlags sUsage, VkMemoryPropertyFlags sProperties, VkBuffer &srBuffer, VkDeviceMemory &srBufferMemory )
{
	VkBufferCreateInfo bufferInfo = BufferCreate( sSize, sUsage );

	if ( vkCreateBuffer( DEVICE, &bufferInfo, NULL, &srBuffer ) != VK_SUCCESS )
		throw std::runtime_error( "Failed to create buffer!" );

	VkMemoryRequirements memRequirements;
	vkGetBufferMemoryRequirements( DEVICE, srBuffer, &memRequirements );

	VkMemoryAllocateInfo allocInfo = MemoryAllocate( memRequirements.size, gpDevice->FindMemoryType( memRequirements.memoryTypeBits, sProperties ) );

	if ( vkAllocateMemory( DEVICE, &allocInfo, nullptr, &srBufferMemory ) != VK_SUCCESS )
		throw std::runtime_error( "Failed to allocate buffer memory!" );

	vkBindBufferMemory( DEVICE, srBuffer, srBufferMemory, 0 );
}
/* Copies the memory into the GPU.  */
void MapMemory( VkDeviceMemory sBufferMemory, VkDeviceSize sSize, const void *spData )
{
	void 	*pData;
	vkMapMemory( DEVICE, sBufferMemory, 0, sSize, 0, &pData );
	memcpy( pData, spData, ( size_t )sSize );
	vkUnmapMemory( DEVICE, sBufferMemory );
}
/* Copies the contents of a buffer into another buffer.  */
void CopyBuffer( VkBuffer sSrc, VkBuffer sDst, VkDeviceSize sSize )
{
	VkBufferCopy copyRegion{  };
	copyRegion.srcOffset 	= 0; // Optional
	copyRegion.dstOffset 	= 0; // Optional
	copyRegion.size 	= sSize;
	/* Submit to the GPU.  */
	Submit( [ & ]( VkCommandBuffer c ){ vkCmdCopyBuffer( c, sSrc, sDst, 1, &copyRegion ); } );
}

void CopyBufferToImage( VkBuffer sBuffer, VkImage sImage, uint32_t sWidth, uint32_t sHeight )
{
	VkBufferImageCopy region = BufferImageCopy( sWidth, sHeight );
	/* Submit to the GPU.  */
	Submit( [ & ]( VkCommandBuffer c ){ vkCmdCopyBufferToImage( c, sBuffer, sImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region ); } );
}

void InitImageView( VkImageView& sImageView, VkImage sImage, VkFormat sFormat, VkImageAspectFlags sAspectFlags )
{
	VkImageViewCreateInfo viewInfo = ImageView( sImage, sFormat, sAspectFlags );

	if ( vkCreateImageView( DEVICE, &viewInfo, NULL, &sImageView ) != VK_SUCCESS )
		throw std::runtime_error( "Failed to create texture image view!" );
}

void InitImage( VkImageCreateInfo sImageInfo, VkMemoryPropertyFlags sProperties, VkImage &srImage, VkDeviceMemory &srImageMemory )
{
	if ( vkCreateImage( DEVICE, &sImageInfo, NULL, &srImage ) != VK_SUCCESS )
		throw std::runtime_error( "Failed to create image!" );

	VkMemoryRequirements memRequirements;
	vkGetImageMemoryRequirements( DEVICE, srImage, &memRequirements );

	VkMemoryAllocateInfo allocInfo = MemoryAllocate( memRequirements.size, gpDevice->FindMemoryType( memRequirements.memoryTypeBits, sProperties ) );

	if ( vkAllocateMemory( DEVICE, &allocInfo, NULL, &srImageMemory ) != VK_SUCCESS )
		throw std::runtime_error( "Failed to allocate image memory!" );

	vkBindImageMemory( DEVICE, srImage, srImageMemory, 0 );
}

void InitTextureImage( const String &srImagePath, VkImage &srTImage, VkDeviceMemory &srTImageMem, float *spWidth, float *spHeight )
{
	int 		texWidth;
	int		texHeight;
	int		texChannels;
	VkBuffer 	stagingBuffer;
	VkDeviceMemory 	stagingBufferMemory;
	stbi_uc 	*pPixels = stbi_load( srImagePath.c_str(  ), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha );
	VkDeviceSize 	imageSize = texWidth * texHeight * 4;

	if ( !pPixels )
		throw std::runtime_error( "Failed to load texture image!" );
	
        InitBuffer( imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		    stagingBuffer, stagingBufferMemory );
	
	MapMemory( stagingBufferMemory, imageSize, pPixels );
	stbi_image_free( pPixels );
	
	InitImage( Image( texWidth, texHeight, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT ),
		   VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,srTImage,srTImageMem );
	
        TransitionImageLayout( srTImage, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL );
	
        CopyBufferToImage( stagingBuffer, srTImage, ( uint32_t )texWidth, ( uint32_t )texHeight );
        TransitionImageLayout( srTImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL );
	/* Store the width and height so sprite on screen is the same resolution as it is as a file.  */
	if ( spWidth && spHeight )
	{
		*spWidth 	= ( float )texWidth  / ( float )gpDevice->aWidth  * 2.0f;	//	maintain size and aspect ratio
		*spHeight  	= ( float )texHeight / ( float )gpDevice->aHeight * 2.0f;
	}
	/* Clean memory  */
	vkDestroyBuffer( DEVICE, stagingBuffer, NULL );
	vkFreeMemory( DEVICE, stagingBufferMemory, NULL );
}

void InitTextureImageView( VkImageView &srTImageView, VkImage sTImage )
{
    InitImageView( srTImageView, sTImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT );
}

template< typename T >
void InitTexBuffer( const std::vector< T > &srData, VkBuffer &srBuffer, VkDeviceMemory &srBufferMem, VkBufferUsageFlags sUsage )
{
	VkBuffer 	stagingBuffer;
	VkDeviceMemory 	stagingBufferMemory;
	VkDeviceSize 	bufferSize = sizeof( srData[ 0 ] ) * srData.size(  );
	
	InitBuffer( bufferSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory );
	
        MapMemory( stagingBufferMemory, bufferSize, srData.data(  ) );

        InitBuffer( bufferSize, sUsage, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		    srBuffer, srBufferMem );

        CopyBuffer( stagingBuffer, srBuffer, bufferSize );
	vkDestroyBuffer( DEVICE, stagingBuffer, NULL );
        vkFreeMemory( DEVICE, stagingBufferMemory, NULL );
}

void InitUniformBuffers( VkBuffer *&sprUBuffers, VkDeviceMemory *&sprUBuffersMem )
{
	VkDeviceSize bufferSize = sizeof( ubo_3d_t );
		
	sprUBuffers 	= new VkBuffer[ PSWAPCHAIN.GetImages(  ).size(  ) ];
	sprUBuffersMem	= new VkDeviceMemory[ PSWAPCHAIN.GetImages(  ).size(  ) ];

	for ( int i = 0; i < PSWAPCHAIN.GetImages(  ).size(  ); i++ )
	        InitBuffer( bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			     sprUBuffers[ i ], sprUBuffersMem[ i ] );
}

void InitDescriptorSets( VkDescriptorSet *&sprDescSets, VkDescriptorSetLayout &srDescSetLayout, VkDescriptorPool &srDescPool,
				    ImageInfoSets sDescImageInfos, BufferInfoSets sDescBufferInfos )
{
	std::vector< VkDescriptorSetLayout > 	layouts( PSWAPCHAIN.GetImages(  ).size(  ), srDescSetLayout );
	std::vector< VkWriteDescriptorSet > 	descriptorWrites{  };
	VkDescriptorSetAllocateInfo 		allocInfo{  };
	allocInfo.sType 		= VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool 	= *gpPool;							//	Pool where descriptors are allocated
	allocInfo.descriptorSetCount 	= ( uint32_t )PSWAPCHAIN.GetImages(  ).size(  );
	allocInfo.pSetLayouts 		= layouts.data(  );

	sprDescSets = new VkDescriptorSet [ PSWAPCHAIN.GetImages(  ).size(  ) ];
	if ( vkAllocateDescriptorSets( DEVICE, &allocInfo, sprDescSets ) != VK_SUCCESS )	//	Allocate descriptor sets
		throw std::runtime_error( "Failed to allocate descriptor sets!" );

	for ( int i = 0; i < PSWAPCHAIN.GetImages(  ).size(  ); i++ )					        //	For each of the rendered frames
	{
		int bindingCount = 0;
		for ( auto &&descBufferInfo : sDescBufferInfos )						//	Iterate through the descriptor buffer infos and make descriptor writes for them
		{
			auto 			buffer 		= DescriptorBuffer( descBufferInfo.apBuffer, descBufferInfo.range, 0, i );
			VkWriteDescriptorSet 	descriptorWrite = WriteDescriptor( sprDescSets[ i ], bindingCount, descBufferInfo.type, NULL, &buffer );
			descriptorWrites.push_back( descriptorWrite );
			++bindingCount;
		}
		for ( auto &&descImageInfo : sDescImageInfos )					//	Do the same for the descriptor image infos
		{
			auto image 				= DescriptorImage( descImageInfo.imageLayout, descImageInfo.imageView, descImageInfo.textureSampler );
			VkWriteDescriptorSet descriptorWrite	= WriteDescriptor( sprDescSets[ i ], bindingCount, descImageInfo.type, &image, NULL );
			descriptorWrites.push_back( descriptorWrite );
			++bindingCount;
		}
		vkUpdateDescriptorSets( DEVICE, ( uint32_t )descriptorWrites.size(  ), descriptorWrites.data(  ), 0, NULL );
	}
}
/* Initializes a texture, view, and appropriate descriptors.  */
TextureDescriptor *InitTexture( const String &srImagePath, VkDescriptorSetLayout sLayout, VkDescriptorPool sPool, VkSampler sSampler )
{
	TextureDescriptor	*pTexture = new TextureDescriptor;
	VkImage 		textureImage;
	VkDeviceMemory 		textureMem;
	VkImageView		textureView;
	InitTextureImage( srImagePath, textureImage, textureMem, NULL, NULL );
	InitTextureImageView( textureView, textureImage );
	InitDescriptorSets( pTexture->aSets.GetBuffer(  ), sLayout, sPool,
			    { { { VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, textureView, sSampler, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER } } }, {  } );

	pTexture->aTextureImage 	= textureImage;
	pTexture->aTextureImageMem	= textureMem;
	pTexture->aTextureImageView     = textureView;

	return pTexture;
}
/* Initializes the uniform data, such as uniform buffers.  */
void InitUniformData( UniformDescriptor &srDescriptor, VkDescriptorSetLayout sLayout )
{
	InitUniformBuffers( srDescriptor.aData.GetBuffer(  ), srDescriptor.aMem.GetBuffer(  ) );
	InitDescriptorSets( srDescriptor.aSets.GetBuffer(  ), sLayout, *gpPool, {  },
			    { { { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, srDescriptor.aData.GetBuffer(  ), sizeof( ubo_3d_t ) } } } );
}

void InitImageViews( ImageViews &srSwapChainImageViews )
{
	srSwapChainImageViews.resize( PSWAPCHAIN.GetImages(  ).size(  ) );
	for ( int i = 0; i < PSWAPCHAIN.GetImages(  ).size(  ); i++ )
	        InitImageView( srSwapChainImageViews[ i ], PSWAPCHAIN.GetImages(  ).at( i ), PSWAPCHAIN.GetFormat(  ), VK_IMAGE_ASPECT_COLOR_BIT );
}

void InitRenderPass( VkRenderPass &srRenderPass )
{
	VkAttachmentDescription 	colorAttachment = AttachmentDescription( PSWAPCHAIN.GetFormat(  ), VK_ATTACHMENT_STORE_OP_STORE, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR );
	VkAttachmentDescription 	depthAttachment = AttachmentDescription( FindDepthFormat(  ), VK_ATTACHMENT_STORE_OP_DONT_CARE, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL );

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

	if ( vkCreateRenderPass( DEVICE, &renderPassInfo, NULL, &srRenderPass ) != VK_SUCCESS )
		throw std::runtime_error( "Failed to create render pass!" );
}
/* Create layouts of only one binding for future.  */
VkDescriptorSetLayout InitDescriptorSetLayout( DescSetLayouts sBindings )
{
	LayoutInfoList	info{  };
	for ( auto&& binding : sBindings )
		info.push_back( { binding.descriptorType, binding.stageFlags } );
	if ( gDescriptorCache.Exists( info ) )
	        return gDescriptorCache.GetLayout(  );
	
        uint32_t 	        i = -1;
	VkDescriptorSetLayout   layout;
	for ( auto&& binding : sBindings )
	        binding.binding = ++i;

	VkDescriptorSetLayoutCreateInfo layoutInfo{  };
	layoutInfo.sType 			= VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.bindingCount 		= ( uint32_t )sBindings.size(  );
	layoutInfo.pBindings 			= sBindings.data(  );
	if ( vkCreateDescriptorSetLayout( DEVICE, &layoutInfo, NULL, &layout ) != VK_SUCCESS )
		throw std::runtime_error( "Failed to create descriptor set layout!" );

	gDescriptorCache.AddLayout( layout, info );
	return layout;
}
/* Creates graphics pipeline layouts using the specified descriptor set layouts.  */
VkPipelineLayout InitPipelineLayouts( VkDescriptorSetLayout *spSetLayouts, uint32_t setLayoutsCount )
{
	VkPipelineLayout		pipelineLayout;
	VkPipelineLayoutCreateInfo 	pipelineInfo = PipelineLayout( spSetLayouts, setLayoutsCount );
	VkPushConstantRange		pushConstantRange = PushConstantRange( VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
									       sizeof( push_constant_t ), 0 );

        pipelineInfo.pushConstantRangeCount 	= 1;
	pipelineInfo.pPushConstantRanges 	= &pushConstantRange;
        if ( vkCreatePipelineLayout( DEVICE, &pipelineInfo, NULL, &pipelineLayout ) != VK_SUCCESS )
		throw std::runtime_error( "Failed to create pipeline layout!" );
	return pipelineLayout;
}
/* Refactor to create multiple pipelines for materials, Called whenever a new model is loaded.  */
template< typename T >
void InitGraphicsPipeline( VkPipeline &srPipeline, VkPipelineLayout &srLayout, VkDescriptorSetLayout &srDescSetLayout,
				      const String &srVertShader, const String &srFragShader, int sFlags )
{
	if ( gShaderCache.Exists( srVertShader, srFragShader, srLayout ) )
	{
		srPipeline 		= gShaderCache.GetPipeline(  );
		return;
	}
		
	auto 		vertShaderCode  	= ReadFile( srVertShader );
	auto 		fragShaderCode  	= ReadFile( srFragShader );
		
	VkShaderModule 	vertShaderModule 	= CreateShaderModule( vertShaderCode );	//	Processes incoming verticies, taking world position, color, and texture coordinates as an input
	VkShaderModule 	fragShaderModule 	= CreateShaderModule( fragShaderCode );	//	Fills verticies with fragments to produce color, and depth

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

	/* Region of frambuffer to be rendered to, likely will always use 0, 0 and width, height  */
	VkViewport viewport = Viewport( 0.f, 0.f, ( float )PSWAPCHAIN.GetExtent(  ).width, ( float )PSWAPCHAIN.GetExtent(  ).height, 0.f, 1.0f );

	VkRect2D scissor{  };		//	More agressive cropping than viewport, defining which regions pixels are to be stored
	scissor.offset = { 0, 0 };
	scissor.extent = PSWAPCHAIN.GetExtent(  );

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
	rasterizer.cullMode 			= ( sFlags & NO_CULLING ) ? VK_CULL_MODE_NONE : VK_CULL_MODE_BACK_BIT;	//	FIX FOR MAKING 2D SPRITES WORK!!! WOOOOO!!!!
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
	depthStencil.depthTestEnable 		= ( sFlags & NO_DEPTH ) ? VK_FALSE : VK_TRUE;
	depthStencil.depthWriteEnable		= ( sFlags & NO_DEPTH ) ? VK_FALSE : VK_TRUE;
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
	pipelineInfo.layout 			= srLayout;
	pipelineInfo.renderPass 		= *gpRenderPass;
	pipelineInfo.subpass 			= 0;
	pipelineInfo.basePipelineHandle 	= VK_NULL_HANDLE; // Optional, very important for later when making new pipelines. It is less expensive to reference an existing similar pipeline
	pipelineInfo.basePipelineIndex 		= -1; // Optional

	if ( vkCreateGraphicsPipelines( DEVICE, VK_NULL_HANDLE, 1, &pipelineInfo, NULL, &srPipeline ) != VK_SUCCESS )
		throw std::runtime_error( "Failed to create graphics pipeline!" );

	gShaderCache.AddPipeline( srVertShader, srFragShader, srLayout, srPipeline );

	vkDestroyShaderModule( DEVICE, fragShaderModule, NULL );
	vkDestroyShaderModule( DEVICE, vertShaderModule, NULL );
}

void InitDepthResources( VkImage &srDepthImage, VkDeviceMemory &srDepthImageMemory, VkImageView &srDepthImageView )
{
	VkFormat depthFormat = gpDevice->FindDepthFormat(  );
        InitImage( Image( PSWAPCHAIN.GetExtent(  ).width, PSWAPCHAIN.GetExtent(  ).height, depthFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT ),
		   VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, srDepthImage, srDepthImageMemory );
	
        InitImageView( srDepthImageView, srDepthImage, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT );
        TransitionImageLayout( srDepthImage, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL );
}

void InitFrameBuffer( FrameBuffers &srSwapChainFramebuffers, ImageViews &srSwapChainImageViews,
				 VkImageView &srDepthImageView )
{
	srSwapChainFramebuffers.resize( srSwapChainImageViews.size(  ) );
	for ( int i = 0; i < srSwapChainImageViews.size(  ); i++ )
	{
		std::array< VkImageView, 2 > 	attachments = { srSwapChainImageViews[ i ], srDepthImageView };

		VkFramebufferCreateInfo 	framebufferInfo{  };
		framebufferInfo.sType 		= VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferInfo.renderPass 	= *gpRenderPass;
		framebufferInfo.attachmentCount = ( uint32_t )attachments.size(  );
		framebufferInfo.pAttachments 	= attachments.data(  );
		framebufferInfo.width 		= PSWAPCHAIN.GetExtent(  ).width;
		framebufferInfo.height 		= PSWAPCHAIN.GetExtent(  ).height;
		framebufferInfo.layers 		= 1;

		if ( vkCreateFramebuffer( DEVICE, &framebufferInfo, NULL, &srSwapChainFramebuffers[ i ] ) != VK_SUCCESS )
			throw std::runtime_error( "Failed to create framebuffer!" );
	}
}

void InitDescPool( VkDescriptorPool &srDescPool, std::vector< VkDescriptorPoolSize > sPoolSizes )
{
	VkDescriptorPoolCreateInfo 	poolInfo{  };
	poolInfo.sType 			= VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolInfo.poolSizeCount  	= ( uint32_t )sPoolSizes.size(  );
	poolInfo.pPoolSizes 		= sPoolSizes.data(  );
	poolInfo.maxSets 		= 200000;//( uint32_t )swapChainImages.size(  );

	if ( vkCreateDescriptorPool( DEVICE, &poolInfo, NULL, &srDescPool ) != VK_SUCCESS )
		throw std::runtime_error( "Failed to create descriptor pool!" );
	
	gpPool = &srDescPool;
}

void InitImguiPool( SDL_Window *spWindow )
{
	std::array< VkDescriptorPoolSize, 11 > poolSizes
	{
		{
			{ VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
			{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
			{ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
			{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
			{ VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
			{ VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
			{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
			{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
			{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
			{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
			{ VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 },
		}
	};
	
	VkDescriptorPoolCreateInfo 	poolInfo{  };
        VkDescriptorPool 		imguiPool;
	poolInfo.sType 		= VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolInfo.flags 		= VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
	poolInfo.maxSets 	= 1000;
	poolInfo.poolSizeCount 	= ( uint32_t )poolSizes.size(  );;
	poolInfo.pPoolSizes 	= poolSizes.data(  );

	if ( vkCreateDescriptorPool( DEVICE, &poolInfo, NULL, &imguiPool ) != VK_SUCCESS )
		throw std::runtime_error( "Failed to create imgui pool!" );
	
	ImGui::CreateContext(  );
	ImGui_ImplSDL2_InitForVulkan( spWindow );

	ImGui_ImplVulkan_InitInfo initInfo{  };
	initInfo.Instance 	= gpDevice->GetInstance(  );
	initInfo.PhysicalDevice = gpDevice->GetPhysicalDevice(  );
	initInfo.Device 	= DEVICE;
	initInfo.Queue 		= gpDevice->GetGraphicsQueue(  );
	initInfo.DescriptorPool = imguiPool;
	initInfo.MinImageCount 	= 3;
	initInfo.ImageCount 	= 3;
	initInfo.MSAASamples 	= VK_SAMPLE_COUNT_1_BIT;

	ImGui_ImplVulkan_Init( &initInfo, *gpRenderPass );
	Submit( [ & ]( VkCommandBuffer c ){ ImGui_ImplVulkan_CreateFontsTexture( c ); } );
	ImGui_ImplVulkan_DestroyFontUploadObjects(  );
	gFreeQueue.push_back( [ = ](  ){ vkDestroyDescriptorPool( DEVICE, imguiPool, NULL ); ImGui_ImplVulkan_Shutdown(  ); } );
}

void InitSync( SemaphoreList &srImageAvailableSemaphores, SemaphoreList &srRenderFinishedSemaphores,
			  FenceList &srInFlightFences, FenceList &srImagesInFlight )
{
	srImageAvailableSemaphores.resize( MAX_FRAMES_PROCESSING );
	srRenderFinishedSemaphores.resize( MAX_FRAMES_PROCESSING );
	srInFlightFences.resize( MAX_FRAMES_PROCESSING );
	srImagesInFlight.resize( PSWAPCHAIN.GetImages(  ).size(  ), VK_NULL_HANDLE );
	
	VkSemaphoreCreateInfo semaphoreInfo{  };
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	VkFenceCreateInfo fenceInfo{  };
	fenceInfo.sType	    = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceInfo.flags	    = VK_FENCE_CREATE_SIGNALED_BIT;

	for ( int i = 0 ; i < MAX_FRAMES_PROCESSING; ++i )
		if ( vkCreateSemaphore( DEVICE, &semaphoreInfo, NULL, &srImageAvailableSemaphores[ i ] ) != VK_SUCCESS ||
		     vkCreateSemaphore( DEVICE, &semaphoreInfo, NULL, &srRenderFinishedSemaphores[ i ] ) != VK_SUCCESS ||
		     vkCreateFence( DEVICE, &fenceInfo, NULL, &srInFlightFences[ i ] ) != VK_SUCCESS )
			throw std::runtime_error( "Failed to create sync objects!" );
}

void FreeResources(  )
{
	for ( const auto& rFunc : gFreeQueue )
		rFunc(  );
}

template void InitTexBuffer< uint32_t >( const std::vector< uint32_t >&, VkBuffer&, VkDeviceMemory&, VkBufferUsageFlags );
template void InitTexBuffer< vertex_2d_t >( const std::vector< vertex_2d_t >&, VkBuffer&, VkDeviceMemory&, VkBufferUsageFlags );
template void InitTexBuffer< vertex_3d_t >( const std::vector< vertex_3d_t >&, VkBuffer&, VkDeviceMemory&, VkBufferUsageFlags );
template void InitGraphicsPipeline< vertex_2d_t >( VkPipeline&, VkPipelineLayout&, VkDescriptorSetLayout&, const std::string&, const std::string&, int );
template void InitGraphicsPipeline< vertex_3d_t >( VkPipeline&, VkPipelineLayout&, VkDescriptorSetLayout&, const std::string&, const std::string&, int );
