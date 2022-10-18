#include "core/platform.h"
#include "core/log.h"
#include "util.h"

#include "render/irender.h"
#include "render_vk.h"


VkSampler                                          gSampler;

std::vector< TextureVK* >                          gTextures;
static std::vector< RenderTarget* >                gRenderTargets;
static RenderTarget*                               gpBackBuffer = nullptr;

static ResourceList< VkFramebuffer >               gFramebuffers;

// kind of a hack
static std::unordered_map< VkFramebuffer, Handle > gFramebufferHandles;

// std::unordered_map< ImageInfo*, TextureVK* > gImageMap;


void VK_SetImageLayout( VkImage sImage, VkImageLayout sOldLayout, VkImageLayout sNewLayout, VkImageSubresourceRange& sSubresourceRange )
{
	VkImageMemoryBarrier barrier{};
	barrier.sType               = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.oldLayout           = sOldLayout;
	barrier.newLayout           = sNewLayout;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.image               = sImage;
	barrier.subresourceRange    = sSubresourceRange;

	VkPipelineStageFlags sourceStage;
	VkPipelineStageFlags destinationStage;

#if 1
	switch ( sOldLayout )
	{
		case VK_IMAGE_LAYOUT_UNDEFINED:
			barrier.srcAccessMask = 0;
			sourceStage           = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
			break;

		case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			sourceStage           = VK_PIPELINE_STAGE_TRANSFER_BIT;
			break;

		default:
			Log_Fatal( "Unsupported old layout transition!\n" );
			break;
	}

	switch ( sNewLayout )
	{
		case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
			barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			destinationStage      = VK_PIPELINE_STAGE_TRANSFER_BIT;
			break;

		case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
			barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
			destinationStage      = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
			break;

		case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
			barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
			destinationStage      = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
			break;

		default:
			Log_Fatal( "Unsupported new layout transition!\n" );
			break;
	}

#else

	if ( sOldLayout == VK_IMAGE_LAYOUT_UNDEFINED && sNewLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL )
	{
		barrier.srcAccessMask = VK_ACCESS_NONE_KHR;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

		sourceStage           = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		destinationStage      = VK_PIPELINE_STAGE_TRANSFER_BIT;
	}
	else if ( sOldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && sNewLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL )
	{
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		sourceStage           = VK_PIPELINE_STAGE_TRANSFER_BIT;
		destinationStage      = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	}
	else if ( sOldLayout == VK_IMAGE_LAYOUT_UNDEFINED && sNewLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL )
	{
		barrier.srcAccessMask = VK_ACCESS_NONE_KHR;
		barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

		sourceStage           = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		destinationStage      = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	}
	else
		LogFatal( "Unsupported layout transition!\n" );
#endif

	/* Submit to the GPU.  */
	VK_SingleCommand( [ & ]( VkCommandBuffer c )
	                  { vkCmdPipelineBarrier( c, sourceStage, destinationStage, 0, 0, NULL, 0, NULL, 1, &barrier ); } );
}


void VK_SetImageLayout( VkImage sImage, VkImageLayout sOldLayout, VkImageLayout sNewLayout, u32 sMipLevels )
{
	VkImageSubresourceRange subresourceRange{};
	subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
	subresourceRange.baseMipLevel   = 0;
	subresourceRange.levelCount     = sMipLevels;
	subresourceRange.baseArrayLayer = 0;
	subresourceRange.layerCount     = 1;

	VK_SetImageLayout( sImage, sOldLayout, sNewLayout, subresourceRange );
}


// NOTE: may need to change this for texture filtering later, oh boy
VkSampler VK_GetSampler()
{
	if ( gSampler )
		return gSampler;

	VkFilter vkFilter = VK_FILTER_NEAREST;

	VkSamplerCreateInfo samplerInfo{};
	samplerInfo.sType        = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	samplerInfo.magFilter    = vkFilter;
	samplerInfo.minFilter    = vkFilter;
	samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;

	const auto& deviceProps  = VK_GetPhysicalDeviceProperties();

	samplerInfo.anisotropyEnable        = VK_TRUE;
	samplerInfo.maxAnisotropy           = deviceProps.limits.maxSamplerAnisotropy;
	samplerInfo.borderColor             = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	samplerInfo.unnormalizedCoordinates = VK_FALSE; 
	samplerInfo.compareEnable           = VK_FALSE;
	samplerInfo.compareOp               = VK_COMPARE_OP_ALWAYS;
	samplerInfo.mipmapMode              = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	samplerInfo.mipLodBias              = 0.0f;
	samplerInfo.minLod                  = 0.0f;
	samplerInfo.maxLod                  = 1000.0f;

	VK_CheckResult( vkCreateSampler( VK_GetDevice(), &samplerInfo, NULL, &gSampler ), "Failed to create sampler!" );

	return gSampler;
}


TextureVK* VK_NewTexture()
{
	TextureVK* tex = gTextures.emplace_back( new TextureVK );
	tex->aIndex    = gTextures.size() - 1;
	return tex;
}


TextureVK* VK_LoadTexture( const std::string& srPath )
{
	TextureVK* tex = KTX_LoadTexture( srPath );

	if ( tex == nullptr )
	{
		Log_ErrorF( gLC_Render, "Failed to load texture: \"%s\"\n", srPath.c_str() );
		return nullptr;
	}

	VK_UpdateImageSets();
	return tex;
}


TextureVK* VK_CreateTexture( const TextureCreateInfo_t& srCreate )
{
	TextureVK* tex = gTextures.emplace_back( new TextureVK );
	tex->aIndex    = gTextures.size() - 1;
	tex->aSize     = srCreate.aSize;
	tex->aFormat   = VK_ToVkFormat( srCreate.aFormat );
	tex->aUsage    = VK_ToVkImageUsage( srCreate.aUsage );

	VkImageCreateInfo createInfo{ VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
	createInfo.imageType     = VK_IMAGE_TYPE_2D;
	createInfo.extent.width  = srCreate.aSize.x;
	createInfo.extent.height = srCreate.aSize.y;
	createInfo.extent.depth  = 1;
	createInfo.mipLevels     = 1;
	createInfo.arrayLayers   = 1;
	createInfo.format        = tex->aFormat;
	createInfo.tiling        = VK_IMAGE_TILING_OPTIMAL;
	createInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	createInfo.usage         = tex->aUsage;
	createInfo.samples       = srCreate.aUseMSAA ? VK_GetMSAASamples() : VK_SAMPLE_COUNT_1_BIT;
	createInfo.sharingMode   = VK_SHARING_MODE_EXCLUSIVE;

	VK_CheckResult( vkCreateImage( VK_GetDevice(), &createInfo, NULL, &tex->aImage ), "Failed to create image" );

	// Allocate and Bind Image Memory
	{
		VkMemoryRequirements memRequirements;
		vkGetImageMemoryRequirements( VK_GetDevice(), tex->aImage, &memRequirements );

		VkMemoryAllocateInfo allocInfo{ VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
		allocInfo.allocationSize  = memRequirements.size;
		allocInfo.memoryTypeIndex = VK_GetMemoryType( memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT );

		VK_CheckResult( vkAllocateMemory( VK_GetDevice(), &allocInfo, NULL, &tex->aMemory ), "Failed to allocate image memory" );

		VK_CheckResult( vkBindImageMemory( VK_GetDevice(), tex->aImage, tex->aMemory, 0 ), "Failed to bind image memory" );
	}

	// if ( createInfo.usage & VK_IMAGE_USAGE_SAMPLED_BIT )
	// 	VK_SetImageLayout( tex->aImage, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, createInfo.mipLevels );

	if ( createInfo.usage & VK_IMAGE_USAGE_SAMPLED_BIT && !( createInfo.usage & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT || createInfo.usage & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT ) )
		VK_SetImageLayout( tex->aImage, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, createInfo.mipLevels );

	if ( createInfo.usage & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT || createInfo.usage & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT )
		tex->aRenderTarget = true;

	VkImageViewType viewType = VK_IMAGE_VIEW_TYPE_2D;

	switch ( srCreate.aViewType )
	{
		default:
		case EImageView_1D:
			viewType = VK_IMAGE_VIEW_TYPE_1D;
			break;

		case EImageView_2D:
			viewType = VK_IMAGE_VIEW_TYPE_2D;
			break;

		case EImageView_3D:
			viewType = VK_IMAGE_VIEW_TYPE_3D;
			break;

		case EImageView_Cube:
			viewType = VK_IMAGE_VIEW_TYPE_CUBE;
			break;

		case EImageView_1D_Array:
			viewType = VK_IMAGE_VIEW_TYPE_1D_ARRAY;
			break;

		case EImageView_2D_Array:
			viewType = VK_IMAGE_VIEW_TYPE_2D_ARRAY;
			break;

		case EImageView_CubeArray:
			viewType = VK_IMAGE_VIEW_TYPE_CUBE_ARRAY;
			break;
	}

	// Create Image View
	VkImageViewCreateInfo viewInfo{};
	viewInfo.sType                           = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	viewInfo.image                           = tex->aImage;
	viewInfo.viewType                        = viewType;
	viewInfo.format                          = tex->aFormat;

	if ( tex->aUsage & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT )
		viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
	else
		viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;

	viewInfo.subresourceRange.baseMipLevel   = 0;
	viewInfo.subresourceRange.levelCount     = createInfo.mipLevels;
	viewInfo.subresourceRange.baseArrayLayer = 0;
	viewInfo.subresourceRange.layerCount     = createInfo.arrayLayers;

	VK_CheckResult( vkCreateImageView( VK_GetDevice(), &viewInfo, nullptr, &tex->aImageView ), "Failed to create Image View" );

	VK_UpdateImageSets();

	return tex;
}


void VK_DestroyTexture( TextureVK* srTexture )
{
	vkDestroyImageView( VK_GetDevice(), srTexture->aImageView, nullptr );
	vkDestroyImage( VK_GetDevice(), srTexture->aImage, nullptr );

	// TODO: probably change this so i don't call delete on the texture, and maybe i can reuse this image memory somehow
	vkFreeMemory( VK_GetDevice(), srTexture->aMemory, nullptr );

	vec_remove( gTextures, srTexture );
	delete srTexture;
}


void VK_DestroyAllTextures()
{
	for ( auto& tex : gTextures )
	{
		if ( !tex )
			continue;

		vkDestroyImageView( VK_GetDevice(), tex->aImageView, nullptr );
		vkDestroyImage( VK_GetDevice(), tex->aImage, nullptr );

		// TODO: probably change this so i don't call delete on the texture, and maybe i can reuse this image memory somehow
		vkFreeMemory( VK_GetDevice(), tex->aMemory, nullptr );
		delete tex;
	}

	if ( gSampler )
		vkDestroySampler( VK_GetDevice(), gSampler, nullptr );

	gTextures.clear();
	// gImageMap.clear();
}


TextureVK* VK_GetTexture( Handle texture )
{
	// TextureVK* find = gTextureHandles.Get( texture );
	// 
	// // Image is loaded
	// if ( find != gTextureHandles.end() )
	// 	return find->second;

	return nullptr;
}


Handle VK_CreateFramebuffer( VkRenderPass sRenderPass, u16 sWidth, u16 sHeight, const VkImageView* spAttachments, u32 sCount )
{
	VkFramebufferCreateInfo framebufferInfo{ VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO };
	framebufferInfo.renderPass      = sRenderPass;
	framebufferInfo.attachmentCount = sCount;
	framebufferInfo.pAttachments    = spAttachments;
	framebufferInfo.width           = sWidth;
	framebufferInfo.height          = sHeight;
	framebufferInfo.layers          = 1;

	VkFramebuffer buffer = nullptr;
	VK_CheckResult( vkCreateFramebuffer( VK_GetDevice(), &framebufferInfo, nullptr, &buffer ), "Failed to create framebuffer" );

	return gFramebuffers.Add( buffer );
}


Handle VK_CreateFramebuffer( const VkFramebufferCreateInfo& sCreateInfo )
{
	VkFramebuffer buffer = nullptr;
	VK_CheckResult( vkCreateFramebuffer( VK_GetDevice(), &sCreateInfo, nullptr, &buffer ), "Failed to create framebuffer" );
	return gFramebuffers.Add( buffer );
}


void VK_DestroyFramebuffer( Handle shHandle )
{
	VkFramebuffer buffer = nullptr;
	if ( !gFramebuffers.Get( shHandle, &buffer ) )
	{
		Log_Error( gLC_Render, "Failed to Get Framebuffer to destroy\n" );
		return;
	}

	vkDestroyFramebuffer( VK_GetDevice(), buffer, nullptr );
	gFramebuffers.Remove( shHandle );
}


VkFramebuffer VK_GetFramebuffer( Handle shHandle )
{
	VkFramebuffer buffer = nullptr;
	if ( !gFramebuffers.Get( shHandle, &buffer ) )
	{
		Log_Error( gLC_Render, "Failed to Get Framebuffer\n" );
		return VK_NULL_HANDLE;
	}

	return buffer;
}


void VK_CreateRenderTargetInt( RenderTarget* target, const std::vector< TextureVK* >& srImages, u16 sWidth, u16 sHeight, const std::vector< VkImageView >& srSwapImages )
{
	target->aImages.resize( srImages.size() );
	target->aFrameBuffers.resize( srSwapImages.size() );

	for ( size_t i = 0; i < srImages.size(); ++i )
		target->aImages[ i ] = srImages[ i ];

	for ( u32 i = 0; i < VK_GetSwapImageCount(); ++i )
	{
		std::vector< VkImageView > attachments;

		// TODO: this probably is wrong lol
		if ( VK_UseMSAA() )
		{
			for ( auto image : srImages )
				attachments.push_back( image->aImageView );

			// color resolve images?
			if ( srSwapImages.size() )
				attachments.push_back( srSwapImages[ i ] );
		}
		else
		{
			if( srSwapImages.size() )
				attachments.push_back( srSwapImages[ i ] );

			attachments.push_back( srImages[ 1 ]->aImageView );
		}

		VkFramebufferCreateInfo framebufferInfo{ VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO };
		framebufferInfo.renderPass      = VK_GetRenderPass();
		framebufferInfo.attachmentCount = attachments.size();
		framebufferInfo.pAttachments    = attachments.data();
		framebufferInfo.width           = sWidth;
		framebufferInfo.height          = sHeight;
		framebufferInfo.layers          = 1;

		VK_CheckResult( vkCreateFramebuffer( VK_GetDevice(), &framebufferInfo, nullptr, &target->aFrameBuffers[ i ] ), "Failed to create framebuffer" );

		gFramebufferHandles[ target->aFrameBuffers[ i ] ] = gFramebuffers.Add( target->aFrameBuffers[ i ] );
	}
}


RenderTarget* VK_CreateRenderTarget( const std::vector< TextureVK* >& srImages, u16 sWidth, u16 sHeight, const std::vector< VkImageView >& srSwapImages )
{
	auto target = gRenderTargets.emplace_back( new RenderTarget );
	VK_CreateRenderTargetInt( target, srImages, sWidth, sHeight, srSwapImages );
	return target;
}


void VK_DestroyRenderTarget( RenderTarget* spTarget )
{
	if ( !spTarget )
		return;

	for ( auto frameBuffer : spTarget->aFrameBuffers )
	{
		vkDestroyFramebuffer( VK_GetDevice(), frameBuffer, nullptr );
		gFramebuffers.Remove( gFramebufferHandles[ frameBuffer ] );
		gFramebufferHandles.erase( frameBuffer );
	}

	for ( auto& texture : spTarget->aImages )
		VK_DestroyTexture( texture );

	vec_remove( gRenderTargets, spTarget );
	delete spTarget;
}


void VK_DestroyRenderTargets()
{
	for ( auto& target : gRenderTargets )
	{
		VK_DestroyRenderTarget( target );
	}

	gRenderTargets.clear();
	gpBackBuffer = nullptr;
}


RenderTarget* CreateBackBuffer()
{
	/*
     *    Our backbuffer contains 3 render operations: Color, Depth, and Resolve,
     *    so we'll make those now.
     */
	TextureVK* colorTex = VK_NewTexture();
	colorTex->aRenderTarget = true;

	VkImageCreateInfo color;
	color.sType                 = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	color.pNext                 = nullptr;
	color.flags                 = 0;
	color.imageType             = VK_IMAGE_TYPE_2D;
	color.format                = VK_GetSwapFormat();
	color.extent.width          = VK_GetSwapExtent().width;
	color.extent.height         = VK_GetSwapExtent().height;
	color.extent.depth          = 1;
	color.mipLevels             = 1;
	color.arrayLayers           = 1;
	color.samples               = VK_GetMSAASamples();
	color.tiling                = VK_IMAGE_TILING_OPTIMAL;
	// DEMEZ TEST
	// color.usage                 = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	// color.usage                 = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
	color.usage                 = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
	// color.usage                 = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT;
	color.sharingMode           = VK_SHARING_MODE_EXCLUSIVE;
	color.queueFamilyIndexCount = 0;
	color.pQueueFamilyIndices   = nullptr;
	color.initialLayout         = VK_IMAGE_LAYOUT_UNDEFINED;

	VK_CheckResult( vkCreateImage( VK_GetDevice(), &color, nullptr, &colorTex->aImage ), "Failed to create color image!" );

	VkMemoryRequirements memReqs;
	vkGetImageMemoryRequirements( VK_GetDevice(), colorTex->aImage, &memReqs );

	VkMemoryAllocateInfo allocInfo = {};
	allocInfo.sType                = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.pNext                = nullptr;
	allocInfo.allocationSize       = memReqs.size;
	allocInfo.memoryTypeIndex      = VK_GetMemoryType( memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT );

	VK_CheckResult( vkAllocateMemory( VK_GetDevice(), &allocInfo, nullptr, &colorTex->aMemory ), "Failed to allocate color image memory!" );

	vkBindImageMemory( VK_GetDevice(), colorTex->aImage, colorTex->aMemory, 0 );

	VkImageViewCreateInfo colorView;
	colorView.sType                           = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	colorView.pNext                           = nullptr;
	colorView.flags                           = 0;
	colorView.image                           = colorTex->aImage;
	colorView.viewType                        = VK_IMAGE_VIEW_TYPE_2D;
	colorView.format                          = color.format;
	colorView.components                      = { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A };
	colorView.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
	colorView.subresourceRange.baseMipLevel   = 0;
	colorView.subresourceRange.levelCount     = 1;
	colorView.subresourceRange.baseArrayLayer = 0;
	colorView.subresourceRange.layerCount     = 1;

	VK_CheckResult( vkCreateImageView( VK_GetDevice(), &colorView, nullptr, &colorTex->aImageView ), "Failed to create color image view!" );

	// ------------------------------------------------------
	// Create Depth Texture

	TextureVK* depthTex     = VK_NewTexture();
	depthTex->aRenderTarget = true;

	VkImageCreateInfo depth;
	depth.sType                 = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	depth.pNext                 = nullptr;
	depth.flags                 = 0;
	depth.imageType             = VK_IMAGE_TYPE_2D;
	depth.format                = VK_FORMAT_D32_SFLOAT;
	depth.extent.width          = VK_GetSwapExtent().width;
	depth.extent.height         = VK_GetSwapExtent().height;
	depth.extent.depth          = 1;
	depth.mipLevels             = 1;
	depth.arrayLayers           = 1;
	depth.samples               = VK_GetMSAASamples();
	depth.tiling                = VK_IMAGE_TILING_OPTIMAL;
	depth.usage                 = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
	depth.sharingMode           = VK_SHARING_MODE_EXCLUSIVE;
	depth.queueFamilyIndexCount = 0;
	depth.pQueueFamilyIndices   = nullptr;
	depth.initialLayout         = VK_IMAGE_LAYOUT_UNDEFINED;

	VK_CheckResult( vkCreateImage( VK_GetDevice(), &depth, nullptr, &depthTex->aImage ), "Failed to create depth image!" );

	vkGetImageMemoryRequirements( VK_GetDevice(), depthTex->aImage, &memReqs );

	allocInfo.allocationSize  = memReqs.size;
	allocInfo.memoryTypeIndex = VK_GetMemoryType( memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT );

	VK_CheckResult( vkAllocateMemory( VK_GetDevice(), &allocInfo, nullptr, &depthTex->aMemory ), "Failed to allocate depth image memory!" );

	vkBindImageMemory( VK_GetDevice(), depthTex->aImage, depthTex->aMemory, 0 );

	VkImageViewCreateInfo depthView;
	depthView.sType                           = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	depthView.pNext                           = nullptr;
	depthView.flags                           = 0;
	depthView.image                           = depthTex->aImage;
	depthView.viewType                        = VK_IMAGE_VIEW_TYPE_2D;
	depthView.format                          = depth.format;
	depthView.components                      = { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A };
	depthView.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_DEPTH_BIT;
	depthView.subresourceRange.baseMipLevel   = 0;
	depthView.subresourceRange.levelCount     = 1;
	depthView.subresourceRange.baseArrayLayer = 0;
	depthView.subresourceRange.layerCount     = 1;

	VK_CheckResult( vkCreateImageView( VK_GetDevice(), &depthView, nullptr, &depthTex->aImageView ), "Failed to create depth image view!" );

	// TODO: wtf use colorView and depthView, i love memory leaks lol
	Log_Warn( gLC_Render, "why am i not using the colorView and depthView we created???!?!?!!?\n" );

	RenderTarget* rt = VK_CreateRenderTarget( { colorTex, depthTex }, VK_GetSwapExtent().width, VK_GetSwapExtent().height, VK_GetSwapImageViews() );
	// RenderTarget* rt = VK_CreateRenderTarget( { colorTex, depthTex }, VK_GetSwapExtent().width, VK_GetSwapExtent().height, { colorTex->aImageView, depthTex->aImageView } );
	
	return rt;
}


RenderTarget* VK_CreateRenderTarget( const CreateRenderTarget_t& srCreate )
{
	/*
     *    Our backbuffer contains 3 render operations: Color, Depth, and Resolve,
     *    so we'll make those now.
     */
	TextureVK* colorTex = VK_NewTexture();
	colorTex->aRenderTarget = true;

	VkImageCreateInfo color;
	color.sType                 = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	color.pNext                 = nullptr;
	color.flags                 = 0;
	color.imageType             = VK_IMAGE_TYPE_2D;
	color.format                = VK_GetSwapFormat();
	color.extent.width          = VK_GetSwapExtent().width;
	color.extent.height         = VK_GetSwapExtent().height;
	color.extent.depth          = 1;
	color.mipLevels             = 1;
	color.arrayLayers           = 1;
	color.samples               = VK_GetMSAASamples();
	color.tiling                = VK_IMAGE_TILING_OPTIMAL;
	// DEMEZ TEST
	// color.usage                 = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	// color.usage                 = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
	color.usage                 = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
	// color.usage                 = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT;
	color.sharingMode           = VK_SHARING_MODE_EXCLUSIVE;
	color.queueFamilyIndexCount = 0;
	color.pQueueFamilyIndices   = nullptr;
	color.initialLayout         = VK_IMAGE_LAYOUT_UNDEFINED;

	VK_CheckResult( vkCreateImage( VK_GetDevice(), &color, nullptr, &colorTex->aImage ), "Failed to create color image!" );

	VkMemoryRequirements memReqs;
	vkGetImageMemoryRequirements( VK_GetDevice(), colorTex->aImage, &memReqs );

	VkMemoryAllocateInfo allocInfo = {};
	allocInfo.sType                = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.pNext                = nullptr;
	allocInfo.allocationSize       = memReqs.size;
	allocInfo.memoryTypeIndex      = VK_GetMemoryType( memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT );

	VK_CheckResult( vkAllocateMemory( VK_GetDevice(), &allocInfo, nullptr, &colorTex->aMemory ), "Failed to allocate color image memory!" );

	vkBindImageMemory( VK_GetDevice(), colorTex->aImage, colorTex->aMemory, 0 );

	VkImageViewCreateInfo colorView;
	colorView.sType                           = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	colorView.pNext                           = nullptr;
	colorView.flags                           = 0;
	colorView.image                           = colorTex->aImage;
	colorView.viewType                        = VK_IMAGE_VIEW_TYPE_2D;
	colorView.format                          = color.format;
	colorView.components                      = { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A };
	colorView.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
	colorView.subresourceRange.baseMipLevel   = 0;
	colorView.subresourceRange.levelCount     = 1;
	colorView.subresourceRange.baseArrayLayer = 0;
	colorView.subresourceRange.layerCount     = 1;

	VK_CheckResult( vkCreateImageView( VK_GetDevice(), &colorView, nullptr, &colorTex->aImageView ), "Failed to create color image view!" );

	// ------------------------------------------------------
	// Create Depth Texture

	TextureVK* depthTex     = VK_NewTexture();
	depthTex->aRenderTarget = true;

	VkImageCreateInfo depth;
	depth.sType                 = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	depth.pNext                 = nullptr;
	depth.flags                 = 0;
	depth.imageType             = VK_IMAGE_TYPE_2D;
	depth.format                = VK_FORMAT_D32_SFLOAT;
	depth.extent.width          = VK_GetSwapExtent().width;
	depth.extent.height         = VK_GetSwapExtent().height;
	depth.extent.depth          = 1;
	depth.mipLevels             = 1;
	depth.arrayLayers           = 1;
	depth.samples               = VK_GetMSAASamples();
	depth.tiling                = VK_IMAGE_TILING_OPTIMAL;
	depth.usage                 = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
	depth.sharingMode           = VK_SHARING_MODE_EXCLUSIVE;
	depth.queueFamilyIndexCount = 0;
	depth.pQueueFamilyIndices   = nullptr;
	depth.initialLayout         = VK_IMAGE_LAYOUT_UNDEFINED;

	VK_CheckResult( vkCreateImage( VK_GetDevice(), &depth, nullptr, &depthTex->aImage ), "Failed to create depth image!" );

	vkGetImageMemoryRequirements( VK_GetDevice(), depthTex->aImage, &memReqs );

	allocInfo.allocationSize  = memReqs.size;
	allocInfo.memoryTypeIndex = VK_GetMemoryType( memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT );

	VK_CheckResult( vkAllocateMemory( VK_GetDevice(), &allocInfo, nullptr, &depthTex->aMemory ), "Failed to allocate depth image memory!" );

	vkBindImageMemory( VK_GetDevice(), depthTex->aImage, depthTex->aMemory, 0 );

	VkImageViewCreateInfo depthView;
	depthView.sType                           = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	depthView.pNext                           = nullptr;
	depthView.flags                           = 0;
	depthView.image                           = depthTex->aImage;
	depthView.viewType                        = VK_IMAGE_VIEW_TYPE_2D;
	depthView.format                          = depth.format;
	depthView.components                      = { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A };
	depthView.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_DEPTH_BIT;
	depthView.subresourceRange.baseMipLevel   = 0;
	depthView.subresourceRange.levelCount     = 1;
	depthView.subresourceRange.baseArrayLayer = 0;
	depthView.subresourceRange.layerCount     = 1;

	VK_CheckResult( vkCreateImageView( VK_GetDevice(), &depthView, nullptr, &depthTex->aImageView ), "Failed to create depth image view!" );

	RenderTarget* rt = VK_CreateRenderTarget( { colorTex, depthTex }, VK_GetSwapExtent().width, VK_GetSwapExtent().height, VK_GetSwapImageViews() );

	return rt;
}

/*
 *    Returns the backbuffer.^
 *    The returned backbuffer contains framebuffers which
 *    are to be drawn to during command buffer recording.
 *    Previously, these were wrongly assumed to be the
 *    same as the swapchain images, but it turns out that
 *    this doesn't matter, it just needs something to draw to.
 * 
 *    @return RenderTarget *    The backbuffer.
 */
RenderTarget* VK_GetBackBuffer()
{
	if ( !gpBackBuffer )
	{
		gpBackBuffer = CreateBackBuffer();
	}
	return gpBackBuffer;
}


Handle VK_GetFrameBufferHandle( VkFramebuffer sFrameBuffer )
{
	auto it = gFramebufferHandles.find( sFrameBuffer );
	if ( it != gFramebufferHandles.end() )
	{
		return it->second;
	}

	Log_Error( gLC_Render, "Framebuffer Handle not found!\n" );
	return InvalidHandle;
}


VkFramebuffer VK_GetFrameBuffer( Handle shFrameBuffer )
{
	VkFramebuffer* frameBuffer = gFramebuffers.Get( shFrameBuffer );

	if ( frameBuffer == nullptr )
	{
		Log_Error( gLC_Render, "Framebuffer not found!\n" );
		return nullptr;
	}

	return *frameBuffer;
}

