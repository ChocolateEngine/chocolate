#include "allocator.h"
#include "debug.h"
#include "core/filesystem.h"
#include "graphics/primvtx.hh"

#include "gutil.hh"

FunctionList 			gFreeQueue;
ShaderCache			gShaderCache;
DescriptorCache			gDescriptorCache;
PipelineBuilder			gPipelineBuilder;
LayoutBuilder			gLayoutBuilder;

Device 				*gpDevice 		= nullptr;
ImageSet 			*gpSwapChainImages 	= NULL;
VkRenderPass 			*gpRenderPass 		= new VkRenderPass;
VkDescriptorPool	        *gpPool 		= new VkDescriptorPool;

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

void TransitionImageLayout( VkImage sImage, VkImageLayout sOldLayout, VkImageLayout sNewLayout, uint32_t sMipLevels )
{
	VkImageMemoryBarrier 	barrier	= ImageMemoryBarrier( sImage, sOldLayout, sNewLayout, sMipLevels );
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

	CheckVKResult( vkCreateBuffer( DEVICE, &bufferInfo, NULL, &srBuffer ), "Failed to create buffer!" );

	VkMemoryRequirements memRequirements;
	vkGetBufferMemoryRequirements( DEVICE, srBuffer, &memRequirements );

	VkMemoryAllocateInfo allocInfo = MemoryAllocate( memRequirements.size, gpDevice->FindMemoryType( memRequirements.memoryTypeBits, sProperties ) );

	CheckVKResult( vkAllocateMemory( DEVICE, &allocInfo, nullptr, &srBufferMemory ), "Failed to allocate buffer memory!" );

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

void InitImageView( VkImageView& sImageView, VkImageViewCreateInfo &srInfo )
{
	if ( vkCreateImageView( DEVICE, &srInfo, NULL, &sImageView ) != VK_SUCCESS )
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

void InitTextureImageView( VkImageView &srTImageView, VkImage sTImage, uint32_t sMipLevels )
{
	auto i = ImageView( sTImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT, sMipLevels );
	InitImageView( srTImageView, i );
}

/* Creates the mip maps from a given VkImage.  */
void GenerateMipMaps( VkImage sImage, VkFormat sFormat, uint32_t sWidth, uint32_t sHeight, uint32_t sMipLevels )
{
	VkFormatProperties 	formatProperties;
	vkGetPhysicalDeviceFormatProperties( gpDevice->GetPhysicalDevice(  ), sFormat, &formatProperties );
	if ( !( formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT ) )
		throw std::runtime_error("texture image format does not support linear blitting!");
	
	auto 			barrier         = ImageMemoryBarrier( sImage, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_UNDEFINED, 1 );
	int32_t 		mipWidth 	= sWidth;
        int32_t 		mipHeight	= sHeight;
	
	for ( uint32_t i = 1; i < sMipLevels; ++i )
	{
		barrier.subresourceRange.baseMipLevel = i - 1;

		barrier.oldLayout 	= VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		barrier.newLayout 	= VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		barrier.srcAccessMask 	= VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask 	= VK_ACCESS_TRANSFER_READ_BIT;
		Submit( [ & ]( VkCommandBuffer c ){ vkCmdPipelineBarrier( c, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
									  0, 0, nullptr, 0, nullptr, 1, &barrier ); } );
		VkImageBlit blit{  };
		blit.srcOffsets[ 0 ] 			= { 0, 0, 0 };
		blit.srcOffsets[ 1 ] 			= { mipWidth, mipHeight, 1 };
		blit.srcSubresource.aspectMask 		= VK_IMAGE_ASPECT_COLOR_BIT;
		blit.srcSubresource.mipLevel 		= i - 1;
		blit.srcSubresource.baseArrayLayer 	= 0;
		blit.srcSubresource.layerCount 		= 1;
		blit.dstOffsets[ 0 ] 			= { 0, 0, 0 };
		blit.dstOffsets[ 1 ] 			= { mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, 1 };
		blit.dstSubresource.aspectMask 		= VK_IMAGE_ASPECT_COLOR_BIT;
		blit.dstSubresource.mipLevel 		= i;
		blit.dstSubresource.baseArrayLayer 	= 0;
		blit.dstSubresource.layerCount 		= 1;
		Submit( [ & ]( VkCommandBuffer c ){ vkCmdBlitImage( c, sImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, sImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
								    1, &blit, VK_FILTER_LINEAR ); } );
		
		barrier.oldLayout 	= VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		barrier.newLayout 	= VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		barrier.srcAccessMask 	= VK_ACCESS_TRANSFER_READ_BIT;
		barrier.dstAccessMask 	= VK_ACCESS_SHADER_READ_BIT;

		Submit( [ & ]( VkCommandBuffer c ){ vkCmdPipelineBarrier( c, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
									  0, 0, nullptr, 0, nullptr, 1, &barrier ); } );
		if ( mipWidth > 1 ) 	mipWidth 	/= 2;
		if ( mipHeight > 1 ) 	mipHeight 	/= 2;
	}
	barrier.subresourceRange.baseMipLevel 	= sMipLevels - 1;
	barrier.oldLayout 			= VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	barrier.newLayout 			= VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	barrier.srcAccessMask 			= VK_ACCESS_TRANSFER_WRITE_BIT;
	barrier.dstAccessMask 			= VK_ACCESS_SHADER_READ_BIT;

	Submit( [ & ]( VkCommandBuffer c ){ vkCmdPipelineBarrier( c, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
								  0, 0, nullptr, 0, nullptr, 1, &barrier ); } );
}

template< typename T >
void InitTexBuffer( const std::vector< T > &srData, VkBuffer &srBuffer, VkDeviceMemory &srBufferMem, VkBufferUsageFlags sUsage )
{
	VkBuffer 	stagingBuffer;
	VkDeviceMemory 	stagingBufferMemory;
	VkDeviceSize 	bufferSize = sizeof( srData[ 0 ] ) * srData.size(  );

	if ( !bufferSize ) {
		srBuffer    = 0;
		srBufferMem = 0;

	        IDPF( "Tried to create a vertex buffer / index buffer with no size!\n" );

		return;
	}
	
	InitBuffer( bufferSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory );
	
        MapMemory( stagingBufferMemory, bufferSize, srData.data(  ) );

        InitBuffer( bufferSize, sUsage, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		    srBuffer, srBufferMem );

        CopyBuffer( stagingBuffer, srBuffer, bufferSize );
	vkDestroyBuffer( DEVICE, stagingBuffer, NULL );
        vkFreeMemory( DEVICE, stagingBufferMemory, NULL );
}

void InitUniformBuffers( DataBuffer< VkBuffer > &srUBuffers, DataBuffer< VkDeviceMemory > &srUBuffersMem, size_t uboSize )
{
	VkDeviceSize bufferSize = uboSize;
		
	srUBuffers.Allocate( PSWAPCHAIN.GetImages(  ).size(  ) );
	srUBuffersMem.Allocate( PSWAPCHAIN.GetImages(  ).size(  ) );

	for ( int i = 0; i < PSWAPCHAIN.GetImages(  ).size(  ); i++ )
	        InitBuffer( bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			    srUBuffers.GetBuffer(  )[ i ], srUBuffersMem.GetBuffer(  )[ i ] );
}

// Still needed for uniform buffers for now
void InitDescriptorSets( DataBuffer< VkDescriptorSet > &srDescSets, VkDescriptorSetLayout &srDescSetLayout,
			 ImageInfoSets sDescImageInfos, BufferInfoSets sDescBufferInfos )
{
	std::vector< VkDescriptorSetLayout > 	layouts( PSWAPCHAIN.GetImages(  ).size(  ), srDescSetLayout );
	std::vector< VkWriteDescriptorSet > 	descriptorWrites{  };
	VkDescriptorSetAllocateInfo 		allocInfo{  };
	allocInfo.sType 		= VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool 	= *gpPool;							//	Pool where descriptors are allocated
	allocInfo.descriptorSetCount 	= ( uint32_t )PSWAPCHAIN.GetImages(  ).size(  );
	allocInfo.pSetLayouts 		= layouts.data(  );

	srDescSets.Allocate( PSWAPCHAIN.GetImages(  ).size(  ) );
	CheckVKResult( vkAllocateDescriptorSets( DEVICE, &allocInfo, srDescSets.GetBuffer(  ) ), "Failed to allocate descriptor sets!" );

	for ( int i = 0; i < PSWAPCHAIN.GetImages(  ).size(  ); i++ )					        //	For each of the rendered frames
	{
		int bindingCount = 0;
		for ( auto &&descBufferInfo : sDescBufferInfos )						//	Iterate through the descriptor buffer infos and make descriptor writes for them
		{
			auto 			buffer 		= DescriptorBuffer( descBufferInfo.apBuffer, descBufferInfo.range, 0, i );
			VkWriteDescriptorSet 	descriptorWrite = WriteDescriptor( srDescSets.GetBuffer(  )[ i ], bindingCount, descBufferInfo.type, NULL, &buffer );
			descriptorWrites.push_back( descriptorWrite );
			++bindingCount;
		}
		for ( auto &&descImageInfo : sDescImageInfos )					//	Do the same for the descriptor image infos
		{
			auto image 				= DescriptorImage( descImageInfo.imageLayout, descImageInfo.imageView, descImageInfo.textureSampler );
			VkWriteDescriptorSet descriptorWrite	= WriteDescriptor( srDescSets.GetBuffer(  )[ i ], bindingCount, descImageInfo.type, &image, NULL );
			descriptorWrites.push_back( descriptorWrite );
			++bindingCount;
		}
		vkUpdateDescriptorSets( DEVICE, ( uint32_t )descriptorWrites.size(  ), descriptorWrites.data(  ), 0, NULL );
	}
}
void UpdateImageSets( std::vector< VkDescriptorSet > &srSets, VkDescriptorSetLayout &srLayout, VkDescriptorPool &srPool, std::vector< TextureDescriptor* > &srImages, VkSampler &srSampler ) {
	if ( !srImages.size() )
		return;
	
	std::vector< VkDescriptorSetLayout > layouts( PSWAPCHAIN.GetImages().size(), srLayout );

	uint32_t counts[] = { 100, 100, 100 };
	VkDescriptorSetVariableDescriptorCountAllocateInfo dc{};
	dc.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_VARIABLE_DESCRIPTOR_COUNT_ALLOCATE_INFO;
	dc.descriptorSetCount = PSWAPCHAIN.GetImages().size();
	dc.pDescriptorCounts  = counts;
	
	VkDescriptorSetAllocateInfo a{};
	a.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	a.pNext              = &dc;
	a.descriptorPool     = srPool;
	a.descriptorSetCount = PSWAPCHAIN.GetImages().size();
	a.pSetLayouts        = layouts.data();


	srSets.resize( PSWAPCHAIN.GetImages().size() );
	if ( vkAllocateDescriptorSets( DEVICE, &a, srSets.data() ) != VK_SUCCESS )
		throw std::runtime_error( "Failed to allocate descriptor sets!" );

	for ( uint32_t i = 0; i < srSets.size(); ++i ) {
		std::vector< VkDescriptorImageInfo > infos;
		for ( uint32_t j = 0; j < srImages.size(); ++j ) {
			VkDescriptorImageInfo img{};
			img.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			img.imageView   = srImages[ j ]->aTextureImageView;
			img.sampler     = srSampler;
			
			infos.push_back( img );
		}
		
		VkWriteDescriptorSet w{};
		w.sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		w.dstBinding       = 0;
		w.dstArrayElement  = 0;
		w.descriptorType   = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		w.descriptorCount  = srImages.size();
		w.pBufferInfo      = 0;
		w.dstSet           = srSets[ i ];
		w.pImageInfo       = infos.data();
		
		vkUpdateDescriptorSets( DEVICE, 1, &w, 0, nullptr );
	}
}
/* Initializes a texture, view, and appropriate descriptors.  */
TextureDescriptor *InitTexture( const String &srImagePath, VkDescriptorSetLayout sLayout, VkDescriptorPool sPool, VkSampler sSampler, float *spWidth, float *spHeight )
{
	/*TextureDescriptor	*pTexture = new TextureDescriptor;
	VkImage 		textureImage;
	VkDeviceMemory 		textureMem;
	VkImageView		textureView;

	if ( spWidth && spHeight )
		InitTextureImage( srImagePath, textureImage, textureMem, pTexture->aMipLevels, spWidth, spHeight );
	else
		InitTextureImage( srImagePath, textureImage, textureMem, pTexture->aMipLevels );

	InitTextureImageView( textureView, textureImage, pTexture->aMipLevels );
	InitDescriptorSets( pTexture->aSets, sLayout, sPool,
			    { { { VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, textureView, sSampler, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER } } }, {  } );

	pTexture->aTextureImage 	= textureImage;
	pTexture->aTextureImageMem	= textureMem;
	pTexture->aTextureImageView     = textureView;*/

	return nullptr;
}

/* Initializes the uniform data, such as uniform buffers.  */
void InitUniformData( UniformDescriptor &srDescriptor, VkDescriptorSetLayout sLayout, unsigned int uboSize )
{
	InitUniformBuffers( srDescriptor.aData, srDescriptor.aMem, uboSize );

	InitDescriptorSets( srDescriptor.aSets, sLayout, {},
		{{{
			VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
			srDescriptor.aData.GetBuffer(),
			(unsigned int)uboSize
		}}}
	);
}

void InitImageViews( ImageViews &srSwapChainImageViews )
{
	srSwapChainImageViews.resize( PSWAPCHAIN.GetImages(  ).size(  ) );

	for ( int i = 0; i < PSWAPCHAIN.GetImages(  ).size(  ); i++ ) {
		auto info = ImageView( PSWAPCHAIN.GetImages(  ).at( i ), PSWAPCHAIN.GetFormat(  ), VK_IMAGE_ASPECT_COLOR_BIT, 1 );

	    InitImageView( srSwapChainImageViews[ i ], info );
	}
}

void InitRenderPass(  )
{
	VkAttachmentDescription colorAttachment = AttachmentDescription( PSWAPCHAIN.GetFormat(  ), VK_ATTACHMENT_LOAD_OP_CLEAR,
											 VK_ATTACHMENT_STORE_OP_STORE,
											 VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, gpDevice->GetSamples(  ) );
	
	VkAttachmentDescription depthAttachment = AttachmentDescription( FindDepthFormat(  ), VK_ATTACHMENT_LOAD_OP_CLEAR,
											 VK_ATTACHMENT_STORE_OP_DONT_CARE, 
											 VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, gpDevice->GetSamples(  ) );
	
	VkAttachmentDescription colorAttachmentResolve = AttachmentDescription( PSWAPCHAIN.GetFormat(  ), VK_ATTACHMENT_LOAD_OP_DONT_CARE,
											 VK_ATTACHMENT_STORE_OP_STORE,
											 VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, VK_SAMPLE_COUNT_1_BIT );

	VkAttachmentReference colorAttachmentRef{  };
	colorAttachmentRef.attachment 	= 0;	//	Referenced by index, so 0 means color is referenced first since type is ambiguous
	colorAttachmentRef.layout 	= VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkAttachmentReference depthAttachmentRef{  };
	depthAttachmentRef.attachment 	= 1;
	depthAttachmentRef.layout 	= VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkAttachmentReference colorAttachmentResolveRef{  };
	colorAttachmentResolveRef.attachment 	= 2;
	colorAttachmentResolveRef.layout 	= VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpass{  };	//	Smaller render operations, store them here
	subpass.pipelineBindPoint 	= VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount 	= 1;
	subpass.pColorAttachments 	= &colorAttachmentRef;
	subpass.pDepthStencilAttachment = &depthAttachmentRef;
	subpass.pResolveAttachments 	= &colorAttachmentResolveRef;

	VkSubpassDependency dependency{  };
	dependency.srcSubpass 		= VK_SUBPASS_EXTERNAL;
	dependency.dstSubpass 		= 0;
	dependency.srcStageMask 	= VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	dependency.srcAccessMask 	= 0;
	dependency.dstStageMask 	= VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	dependency.dstAccessMask 	= VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

	std::array< VkAttachmentDescription, 3 > attachments = { colorAttachment, depthAttachment, colorAttachmentResolve };
	VkRenderPassCreateInfo renderPassInfo{  };
	renderPassInfo.sType 		= VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassInfo.attachmentCount 	= ( uint32_t )attachments.size(  );
	renderPassInfo.pAttachments 	= attachments.data(  );
	renderPassInfo.subpassCount	= 1;
	renderPassInfo.pSubpasses 	= &subpass;
	renderPassInfo.dependencyCount 	= 1;
	renderPassInfo.pDependencies 	= &dependency;

	if ( vkCreateRenderPass( DEVICE, &renderPassInfo, NULL, gpRenderPass ) != VK_SUCCESS )
		throw std::runtime_error( "Failed to create render pass!" );
}
/* Create layouts of only one binding for future.  */
VkDescriptorSetLayout InitDescriptorSetLayout( DescSetLayouts sBindings )
{
	LayoutInfoList	info{  };
	for ( auto&& binding : sBindings )
		info.push_back( { binding.descriptorType, binding.stageFlags } );
	//if ( gDescriptorCache.Exists( info ) )
	//        return gDescriptorCache.GetLayout(  );
	
	uint32_t i = -1;
	VkDescriptorSetLayout layout;
	for ( auto&& binding : sBindings )
		binding.binding = ++i;

	VkDescriptorBindingFlagsEXT bindFlag = VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT_EXT | VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT_EXT;

	VkDescriptorSetLayoutBindingFlagsCreateInfoEXT extend{};
	extend.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO_EXT;
	extend.pNext         = nullptr;
	extend.bindingCount  = 1;
	extend.pBindingFlags = &bindFlag;

	VkDescriptorSetLayoutCreateInfo layoutInfo{  };
	layoutInfo.sType 			= VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.pNext                        = &extend;
	layoutInfo.bindingCount 		= ( uint32_t )sBindings.size(  );
	layoutInfo.pBindings 			= sBindings.data(  );
	if ( vkCreateDescriptorSetLayout( DEVICE, &layoutInfo, NULL, &layout ) != VK_SUCCESS )
		throw std::runtime_error( "Failed to create descriptor set layout!" );

	//gDescriptorCache.AddLayout( layout, info );
	return layout;
}
/* A.  */
VkDescriptorSetLayout InitDescriptorSetLayout( VkDescriptorSetLayoutBinding sBinding )
{
	VkDescriptorSetLayout layout;
	sBinding.binding = 0;

	VkDescriptorBindingFlagsEXT bindFlag = VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT_EXT | VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT_EXT;

	VkDescriptorSetLayoutBindingFlagsCreateInfoEXT extend{};
	extend.sType            = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO_EXT;
	extend.pNext            = nullptr;
	extend.bindingCount     = 1;
	extend.pBindingFlags    = &bindFlag;

	VkDescriptorSetLayoutCreateInfo layoutInfo{  };
	layoutInfo.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.pNext        = &extend;
	layoutInfo.bindingCount = 1;
	layoutInfo.pBindings    = &sBinding;

	if ( vkCreateDescriptorSetLayout( DEVICE, &layoutInfo, NULL, &layout ) != VK_SUCCESS )
		throw std::runtime_error( "Failed to create descriptor set layout!" );
	
	return layout;
}

/* Creates graphics pipeline layouts using the specified descriptor set layouts.  */
VkPipelineLayout InitPipelineLayouts( VkDescriptorSetLayout *spSetLayouts, uint32_t setLayoutsCount, uint32_t sPushExtent )
{
	VkPipelineLayoutCreateInfo pipelineInfo {
		.sType          = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
		.setLayoutCount = setLayoutsCount,
		.pSetLayouts    = spSetLayouts
	};

	VkPushConstantRange pushConstantRange {
		.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
		.offset     = 0,
		.size       = sPushExtent
	};

	VkPipelineLayout pipelineLayout;
	pipelineInfo.pushConstantRangeCount = 1;
	pipelineInfo.pPushConstantRanges = &pushConstantRange;
	
	CheckVKResult( vkCreatePipelineLayout( DEVICE, &pipelineInfo, NULL, &pipelineLayout ), "Failed to create pipeline layout" );
	return pipelineLayout;
}

void InitDepthResources( VkImage &srDepthImage, VkDeviceMemory &srDepthImageMemory, VkImageView &srDepthImageView )
{
	VkFormat depthFormat = gpDevice->FindDepthFormat(  );
    InitImage( Image( PSWAPCHAIN.GetExtent(  ).width, PSWAPCHAIN.GetExtent(  ).height, depthFormat,
			   VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, 1, gpDevice->GetSamples(  ) ),
		        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, srDepthImage, srDepthImageMemory );

	auto i = ImageView( srDepthImage, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT, 1 );
    InitImageView( srDepthImageView, i );
    TransitionImageLayout( srDepthImage, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, 1 );
}
/* Initializes the multisampling image.  */
void InitColorResources( VkImage &srColorImage, VkDeviceMemory &srColorImageMemory, VkImageView &srColorImageView )
{
	VkFormat colorFormat = PSWAPCHAIN.GetFormat(  );

    InitImage( Image( PSWAPCHAIN.GetExtent(  ).width, PSWAPCHAIN.GetExtent(  ).height, colorFormat, VK_IMAGE_TILING_OPTIMAL,
			   VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, 1, gpDevice->GetSamples(  ) ),
		       VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, srColorImage, srColorImageMemory );

	auto i = ImageView( srColorImage, colorFormat, VK_IMAGE_ASPECT_COLOR_BIT, 1 );
    InitImageView( srColorImageView, i );
}

void InitFrameBuffer( FrameBuffers &srSwapChainFramebuffers, ImageViews &srSwapChainImageViews, VkImageView &srDepthImageView, VkImageView &srColorImageView )
{
	srSwapChainFramebuffers.resize( srSwapChainImageViews.size(  ) );
	for ( int i = 0; i < srSwapChainImageViews.size(  ); i++ )
	{
		std::array< VkImageView, 3 > 	attachments = { srColorImageView, srDepthImageView, srSwapChainImageViews[ i ] };

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

void InitDescPool( std::vector< VkDescriptorPoolSize > sPoolSizes )
{
	VkDescriptorPoolCreateInfo 	poolInfo{  };
	poolInfo.sType 			= VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolInfo.poolSizeCount  	= ( uint32_t )sPoolSizes.size(  );
	poolInfo.pPoolSizes 		= sPoolSizes.data(  );
	// poolInfo.maxSets 		= 200000;//( uint32_t )swapChainImages.size(  );
	poolInfo.maxSets 		= 10000;

	if ( vkCreateDescriptorPool( DEVICE, &poolInfo, NULL, gpPool ) != VK_SUCCESS )
		throw std::runtime_error( "Failed to create descriptor pool!" );
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
	initInfo.MSAASamples 	= gpDevice->GetSamples(  );
	initInfo.CheckVkResultFn = CheckVKResult;

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

template void 		InitTexBuffer< uint32_t >( const std::vector< uint32_t >&, VkBuffer&, VkDeviceMemory&, VkBufferUsageFlags );
template void 		InitTexBuffer< vertex_2d_t >( const std::vector< vertex_2d_t >&, VkBuffer&, VkDeviceMemory&, VkBufferUsageFlags );
template void 		InitTexBuffer< vertex_3d_t >( const std::vector< vertex_3d_t >&, VkBuffer&, VkDeviceMemory&, VkBufferUsageFlags );
template void           InitTexBuffer< PrimVtx >( const std::vector< PrimVtx >&, VkBuffer&, VkDeviceMemory&, VkBufferUsageFlags );
