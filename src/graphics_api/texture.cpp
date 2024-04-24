#include "core/platform.h"
#include "core/log.h"
#include "util.h"

#include "render/irender.h"
#include "render_vk.h"
#include "missingtexture.h"


// VkSampler                                          gSamplers[ 2 ];
//VkSampler                                          gSamplers[ 2 ][ 5 ];  // [texture filter][sampler address]
VkSampler                                          gSamplers[ 2 ][ 5 ][ 2 ];

static std::vector< RenderTarget* >                gRenderTargets;

static ResourceList< VkFramebuffer >               gFramebuffers;
static std::unordered_map< Handle, glm::uvec2 >    gFramebufferSize;

extern ResourceList< TextureVK* >                  gTextureHandles;
extern ResourceList< BufferVK >                    gBufferHandles;
// extern std::vector< Handle >                       gSwapImageHandles;

// kind of a hack
static std::unordered_map< VkFramebuffer, Handle > gFramebufferHandles;
// std::unordered_map< TextureVK*, Handle >           gBackbufferHandles;

// the true handle of the missing texture, but handle of 0 will also give the missing texture
ChHandle_t                                         gMissingTexHandle = CH_INVALID_HANDLE;

extern bool                                        gNeedTextureUpdate;

static int                                         gTextureSamplers = 0;

// std::unordered_map< ImageInfo*, TextureVK* > gImageMap;

void VK_RecreateTextureSamplers();


 // a few gpu's out there have a max of 255 mipmap levels
// https://vulkan.gpuinfo.org/displaydevicelimit.php?name=maxSamplerLodBias
#define MAX_LOD_BIAS 255 


CONVAR_RANGE_INT_CMD( r_miplod_min, 0, 0, MAX_LOD_BIAS, 0, "Minimum Mipmap Level" )
{
	VK_RecreateTextureSamplers();
}


CONVAR_RANGE_INT_CMD( r_miplod_max, MAX_LOD_BIAS, 0, MAX_LOD_BIAS, 0, "Maximum Mipmap Level" )
{
	VK_RecreateTextureSamplers();
}


CONVAR_RANGE_INT_CMD( r_anisotropy, 16, 0, 16, CVARF_ARCHIVE, "Ansiotropic Filtering Setting" )
{
	VK_RecreateTextureSamplers();
}


CONVAR_RANGE_INT_CMD( r_texture_filtering, 1, 0, 2, CVARF_ARCHIVE, "Enable or Disable Linear Texture Filtering, 0 for off, 1 for default settings, 2 for forced on" )
{
	VK_RecreateTextureSamplers();
}


CONVAR_BOOL_CMD( r_linear_mipmaps, 1, CVARF_ARCHIVE, "Enable or Disable Linear Mipmaps" )
{
	VK_RecreateTextureSamplers();
}


void VK_SetImageLayout( VkCommandBuffer c, VkImage sImage, VkImageLayout sOldLayout, VkImageLayout sNewLayout, VkImageSubresourceRange& sSubresourceRange )
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
			return;
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
			return;
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

	vkCmdPipelineBarrier( c, sourceStage, destinationStage, 0, 0, NULL, 0, NULL, 1, &barrier );
}


void VK_SetImageLayout( VkImage sImage, VkImageLayout sOldLayout, VkImageLayout sNewLayout, VkImageSubresourceRange& sSubresourceRange )
{
	VkCommandBuffer c = VK_BeginOneTimeCommand();

	VK_SetImageLayout( c, sImage, sOldLayout, sNewLayout, sSubresourceRange );

	VK_EndOneTimeCommand( c );
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


void VK_SetImageLayout( VkCommandBuffer c, VkImage sImage, VkImageLayout sOldLayout, VkImageLayout sNewLayout, u32 sMipLevels )
{
	VkImageSubresourceRange subresourceRange{};
	subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
	subresourceRange.baseMipLevel   = 0;
	subresourceRange.levelCount     = sMipLevels;
	subresourceRange.baseArrayLayer = 0;
	subresourceRange.layerCount     = 1;

	VK_SetImageLayout( c, sImage, sOldLayout, sNewLayout, subresourceRange );
}


void VK_CreateTextureSamplers()
{
	return;

#if 0
	for ( int i = 0; i < 2; i++ )
	{
		for ( int address = 0; address < 5; address++ )
		{
			VK_GetSampler( ( VkFilter )i, ( VkSamplerAddressMode )address, VK_FALSE );
			VK_GetSampler( ( VkFilter )i, ( VkSamplerAddressMode )address, VK_TRUE );
		}
	}
#endif
}


void VK_DestroyTextureSamplers()
{
	for ( int i = 0; i < 2; i++ )
	{
		for ( int address = 0; address < 5; address++ )
		{
			if ( gSamplers[ i ][ address ][ 0 ] )
				vkDestroySampler( VK_GetDevice(), gSamplers[ i ][ address ][ 0 ], nullptr );
			if ( gSamplers[ i ][ address ][ 1 ] )
				vkDestroySampler( VK_GetDevice(), gSamplers[ i ][ address ][ 1 ], nullptr );

			gSamplers[ i ][ address ][ 0 ] = VK_NULL_HANDLE;
			gSamplers[ i ][ address ][ 1 ] = VK_NULL_HANDLE;
		}
	}

	gTextureSamplers = 0;

	Log_DevF( 1, "Cleared Texture Samplers\n" );
}


int VK_GetAnisotropicFilteringLevel()
{
	const auto& deviceProps = VK_GetPhysicalDeviceProperties();
	return glm::min( (float)r_anisotropy, deviceProps.limits.maxSamplerAnisotropy );
}


// TODO: use a hash map for caches
VkSampler VK_GetSampler( VkFilter sFilter, VkSamplerAddressMode addressMode, VkBool32 sDepthCompare )
{
	auto& deviceLimits = VK_GetPhysicalDeviceLimits();

	int index = 0;
	int addressIndex = 0;

	if ( r_texture_filtering == 2 )
	{
		// Force Linear Filtering
		index = 1;
	}
	else if ( r_texture_filtering )
	{
		switch ( sFilter )
		{
			default:
			case VK_FILTER_NEAREST:
				break;

			case VK_FILTER_LINEAR:
				index = 1;
				break;
		}
	}

	switch ( addressMode )
	{
		default:
		case VK_SAMPLER_ADDRESS_MODE_REPEAT:
			break;

		case VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT:
			addressIndex = 1;
			break;

		case VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE:
			addressIndex = 2;
			break;

		case VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER:
			addressIndex = 3;
			break;

		case VK_SAMPLER_ADDRESS_MODE_MIRROR_CLAMP_TO_EDGE:
			addressIndex = 4;
			break;
	}

	if ( gSamplers[ index ][ addressIndex ][ sDepthCompare ] )
		return gSamplers[ index ][ addressIndex ][ sDepthCompare ];

	// This Sampler has not been created yet, so create a new texture sampler

	if ( ++gTextureSamplers == deviceLimits.maxSamplerAllocationCount )
	{
		Log_ErrorF( "Out of Texture Samplers!\n" );

		// just look for like any texture sampler
		for ( int i = 0; i < 2; i++ )
		{
			for ( int address = 0; address < 5; address++ )
			{
				if ( gSamplers[ i ][ address ][ 0 ] )
					return gSamplers[ i ][ address ][ 0 ];

				if ( gSamplers[ i ][ address ][ 1 ] )
					return gSamplers[ i ][ address ][ 1 ];
			}
		}

		Log_FatalF( "No Texture Samplers found, yet we are at the maxSamplerAllocationCount of %d?\n", deviceLimits.maxSamplerAllocationCount );
		return VK_NULL_HANDLE;
	}

	const auto&         deviceProps = VK_GetPhysicalDeviceProperties();

	VkSamplerCreateInfo samplerInfo{ VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO };

	samplerInfo.addressModeU = addressMode;
	samplerInfo.addressModeV = addressMode;
	samplerInfo.addressModeW = addressMode;

	// make sure this device has anisotropic filtering and the user requests it
	if ( r_anisotropy > 1 && deviceProps.limits.maxSamplerAnisotropy > 1 )
	{
		samplerInfo.anisotropyEnable = VK_TRUE;
		samplerInfo.maxAnisotropy    = VK_GetAnisotropicFilteringLevel();
	}
	else
	{
		samplerInfo.anisotropyEnable = VK_FALSE;
		samplerInfo.maxAnisotropy    = 0;
	}

	if ( r_texture_filtering == 2 )
	{
		// Force Linear Filtering
		samplerInfo.magFilter = VK_FILTER_LINEAR;
		samplerInfo.minFilter = VK_FILTER_LINEAR;
	}
	else if ( r_texture_filtering )
	{
		samplerInfo.magFilter = sFilter;
		samplerInfo.minFilter = sFilter;
	}
	else
	{
		samplerInfo.magFilter = VK_FILTER_NEAREST;
		samplerInfo.minFilter = VK_FILTER_NEAREST;
	}

	if ( r_linear_mipmaps )
	{
		samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	}
	else
	{
		samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
	}

	samplerInfo.borderColor             = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	samplerInfo.unnormalizedCoordinates = VK_FALSE;
	samplerInfo.compareEnable           = sDepthCompare;
	samplerInfo.compareOp               = VK_COMPARE_OP_LESS;
	samplerInfo.mipLodBias              = 0.f;
	samplerInfo.maxLod                  = std::max( 0, r_miplod_max );
	samplerInfo.minLod                  = std::min( (float)std::max( 0, r_miplod_min ), samplerInfo.maxLod );

	VK_CheckResult( vkCreateSampler( VK_GetDevice(), &samplerInfo, NULL, &gSamplers[ index ][ addressIndex ][ sDepthCompare ] ), "Failed to create sampler!" );

	Log_DevF( 1, "Created New Texture Sampler (%d / %d Max Samplers)\n", gTextureSamplers, deviceLimits.maxSamplerAllocationCount );

	return gSamplers[ index ][ addressIndex ][ sDepthCompare ];
}


void VK_RecreateTextureSamplers()
{
	VK_WaitForGraphicsQueue();
	VK_WaitForTransferQueue();

	VK_DestroyTextureSamplers();
	VK_CreateTextureSamplers();

	gNeedTextureUpdate = true;
}


bool VK_IsCompressedFormat( VkFormat sFormat )
{
	switch ( sFormat )
	{
		default:
			return false;

		case VK_FORMAT_BC7_UNORM_BLOCK:
		case VK_FORMAT_BC7_SRGB_BLOCK:
		case VK_FORMAT_BC6H_UFLOAT_BLOCK:
		case VK_FORMAT_BC6H_SFLOAT_BLOCK:
		case VK_FORMAT_BC3_UNORM_BLOCK:
		case VK_FORMAT_BC3_SRGB_BLOCK:
		case VK_FORMAT_BC1_RGB_UNORM_BLOCK:
		case VK_FORMAT_BC1_RGB_SRGB_BLOCK:
		case VK_FORMAT_BC1_RGBA_UNORM_BLOCK:
		case VK_FORMAT_BC1_RGBA_SRGB_BLOCK:
		case VK_FORMAT_BC4_UNORM_BLOCK:
		case VK_FORMAT_BC4_SNORM_BLOCK:
		case VK_FORMAT_BC5_UNORM_BLOCK:
		case VK_FORMAT_BC5_SNORM_BLOCK:
			return true;
	}
}


// Returns a Float because of ASTC compression
float VK_GetFormatSize( VkFormat sFormat )
{
	switch ( sFormat )
	{
		default:
			return 0;

		case VK_FORMAT_R8_UINT:
		case VK_FORMAT_R8_SINT:
		case VK_FORMAT_R8_SRGB:
			return 8;

		case VK_FORMAT_R8G8_UINT:
		case VK_FORMAT_R8G8_SINT:
		case VK_FORMAT_R8G8_SRGB:
			return 16;

		case VK_FORMAT_R8G8B8_UINT:
		case VK_FORMAT_R8G8B8_SINT:
		case VK_FORMAT_R8G8B8_SRGB:
			return 24;

		case VK_FORMAT_R8G8B8A8_UINT:
		case VK_FORMAT_R8G8B8A8_SINT:
		case VK_FORMAT_R8G8B8A8_SRGB:
			return 32;

		// -------------------------------------------------------
		// Compressed Formats

		case VK_FORMAT_BC7_UNORM_BLOCK:
		case VK_FORMAT_BC7_SRGB_BLOCK:
		case VK_FORMAT_BC6H_UFLOAT_BLOCK:
		case VK_FORMAT_BC6H_SFLOAT_BLOCK:
		case VK_FORMAT_BC3_UNORM_BLOCK:
		case VK_FORMAT_BC3_SRGB_BLOCK:
			return 4;  // 8

		case VK_FORMAT_BC1_RGB_UNORM_BLOCK:
		case VK_FORMAT_BC1_RGB_SRGB_BLOCK:
		case VK_FORMAT_BC1_RGBA_UNORM_BLOCK:  // ???
		case VK_FORMAT_BC1_RGBA_SRGB_BLOCK:   // ???
		case VK_FORMAT_BC4_UNORM_BLOCK:
		case VK_FORMAT_BC4_SNORM_BLOCK:
		case VK_FORMAT_BC5_UNORM_BLOCK:
		case VK_FORMAT_BC5_SNORM_BLOCK:
			return 2;  // 4
	}

	return 0;
}


void VK_CalcTextureMemoryUsage( TextureVK* spTexture )
{
	float formatSize       = VK_GetFormatSize( spTexture->aFormat );

	if ( VK_IsCompressedFormat( spTexture->aFormat ) )
	{
		// treat this as 4x4 blocks?
		//spTexture->aMemorySize = ((spTexture->aSize.x * spTexture->aSize.y / 4.f) * formatSize);
	}
	else
	{
		//spTexture->aMemorySize = spTexture->aSize.x * spTexture->aSize.y * formatSize;
	}

}


TextureVK* VK_NewTexture( ChHandle_t& srHandle )
{
	TextureVK* tex = new TextureVK;
	tex->aIndex    = -1;
	srHandle       = gTextureHandles.Add( tex );
	return tex;
}


bool VK_LoadTexture( ChHandle_t& srHandle, TextureVK* tex, const std::string& srPath, const TextureCreateData_t& srCreateData )
{
	if ( !KTX_LoadTexture( tex, srPath.c_str() ) )
	{
		Log_ErrorF( gLC_Render, "Failed to load texture: \"%s\"\n", srPath.c_str() );
		return false;
	}

	tex->aFilter         = VK_ToVkFilter( srCreateData.aFilter );
	tex->aSamplerAddress = VK_ToVkSamplerAddress( srCreateData.aSamplerAddress );
	tex->aDepthCompare   = srCreateData.aDepthCompare;	// Does this need a dedicated function?
	tex->apName          = VK_AllocString( srPath.data(), srPath.size() );

	// textures loaded through KTX are always sampled currently
	if ( tex->aUsage & VK_IMAGE_USAGE_SAMPLED_BIT )
	{
		gGraphicsAPIData.aSampledTextures.push_back( srHandle );
	}

	VK_SetObjectName( VK_OBJECT_TYPE_IMAGE, (u64)tex->aImage, srPath.c_str() );

	VK_CalcTextureIndices();
	gNeedTextureUpdate = true;

	// VK_CalcTextureMemoryUsage( tex );

	return true;
}


TextureVK* VK_CreateTexture( ChHandle_t& srHandle, const TextureCreateInfo_t& srCreate, const TextureCreateData_t& srCreateData )
{
	if ( srCreate.aSize.x == 0 || srCreate.aSize.y == 0 )
	{
		Log_ErrorF( "Cannot Create Texture with Width or Height of 0" );
		return nullptr;
	}

	TextureVK* tex       = VK_NewTexture( srHandle );
	tex->aIndex          = gTextureHandles.size() - 1;
	tex->aSize           = srCreate.aSize;
	tex->aFormat         = VK_ToVkFormat( srCreate.aFormat );
	tex->aUsage          = VK_ToVkImageUsage( srCreateData.aUsage );
	tex->apName          = Util_AllocString( srCreate.apName );
	tex->aFilter         = VK_ToVkFilter( srCreateData.aFilter );
	tex->aSamplerAddress = VK_ToVkSamplerAddress( srCreateData.aSamplerAddress );
	tex->aDepthCompare   = srCreateData.aDepthCompare;
	tex->aDataSize       = srCreate.aDataSize;

	if ( tex->aUsage & VK_IMAGE_USAGE_SAMPLED_BIT )
	{
		gGraphicsAPIData.aSampledTextures.push_back( srHandle );
	}

	VkImageCreateInfo createInfo{ VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
	createInfo.imageType     = VK_IMAGE_TYPE_2D;
	createInfo.extent.width  = srCreate.aSize.x;
	createInfo.extent.height = srCreate.aSize.y;
	createInfo.extent.depth  = 1;
	createInfo.mipLevels     = 1;
	createInfo.arrayLayers   = 1;
	createInfo.format        = tex->aFormat;

	// hack
	// if ( tex->aBufferMemory == EBufferMemory_Host )
	// {
	// 	createInfo.tiling = VK_IMAGE_TILING_LINEAR;
	// }
	// else
	{
		createInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
	}

	createInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	createInfo.usage         = tex->aUsage;
	createInfo.samples       = srCreateData.aUseMSAA ? VK_GetMSAASamples() : VK_SAMPLE_COUNT_1_BIT;
	createInfo.sharingMode   = VK_SHARING_MODE_EXCLUSIVE;

	if ( srCreate.apData )
		createInfo.usage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;

	// VK_CheckResult( vkCreateImage( VK_GetDevice(), &createInfo, NULL, &tex->aImage ), "Failed to create image" );

	// Allocate and Bind Image Memory
	VK_CreateImage( createInfo, tex );

	// if ( createInfo.usage & VK_IMAGE_USAGE_SAMPLED_BIT )
	// 	VK_SetImageLayout( tex->aImage, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, createInfo.mipLevels );

	bool setToShaderReadOnly = false;

	if ( createInfo.usage & VK_IMAGE_USAGE_SAMPLED_BIT && !( createInfo.usage & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT || createInfo.usage & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT ) )
		setToShaderReadOnly = true;

	if ( createInfo.usage & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT || createInfo.usage & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT )
		tex->aRenderTarget = true;

	if ( srCreate.apData )
	{
		if ( srCreate.aDataSize == 0 )
		{
			// TODO: we can just calculate this ourselves, but that will come later
			Log_Error( gLC_Render, "VK_CreateTexture(): We have texture data, but aDataSize is 0!\n" );
		}
		else
		{
			BufferVK stagingBuffer{
				.aSize = srCreate.aDataSize
			};

			VK_CreateBuffer( &stagingBuffer, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT );

			VK_memcpy( stagingBuffer.aMemory, srCreate.aDataSize, srCreate.apData );

			// Copy Buffer to Image
			VkBufferImageCopy region{};
			region.bufferOffset                    = 0;
			region.bufferRowLength                 = 0;
			region.bufferImageHeight               = 0;

			region.imageSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
			region.imageSubresource.mipLevel       = 0;
			region.imageSubresource.baseArrayLayer = 0;
			region.imageSubresource.layerCount     = 1;

			region.imageOffset                     = { 0, 0, 0 };
			region.imageExtent                     = { (u32)tex->aSize.x, (u32)tex->aSize.y, 1 };

			VkCommandBuffer c                      = VK_BeginOneTimeCommand();

			// Set the layout to Transfer Dest
			VK_SetImageLayout( c, tex->aImage, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, createInfo.mipLevels );

			// Write the image data to the gpu image
			vkCmdCopyBufferToImage( c, stagingBuffer.aBuffer, tex->aImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region );

			// if we need to set it back to shader read only, then do that
			if ( setToShaderReadOnly )
				VK_SetImageLayout( c, tex->aImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, createInfo.mipLevels );

			VK_EndOneTimeCommand( c );

			// Destroy Buffer
			VK_DestroyBuffer( &stagingBuffer );
		}
	}
	else
	{
		if ( setToShaderReadOnly )
			VK_SetImageLayout( tex->aImage, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, createInfo.mipLevels );
	}

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

	switch ( viewType )
	{
		default:
			viewInfo.subresourceRange.layerCount = 1;
			break;

		case VK_IMAGE_VIEW_TYPE_1D_ARRAY:
		case VK_IMAGE_VIEW_TYPE_2D_ARRAY:
		case VK_IMAGE_VIEW_TYPE_CUBE_ARRAY:
			viewInfo.subresourceRange.layerCount = createInfo.arrayLayers;
			break;
	}

	VK_CheckResult( vkCreateImageView( VK_GetDevice(), &viewInfo, nullptr, &tex->aImageView ), "Failed to create Image View" );

	VK_SetObjectName( VK_OBJECT_TYPE_IMAGE, (u64)tex->aImage, tex->apName ? tex->apName : "MANUALLY CREATED TEXTURE" );

	VK_CalcTextureIndices();

	gNeedTextureUpdate = true;

	VK_CalcTextureMemoryUsage( tex );

	return tex;
}


void VK_DestroyTexture( ChHandle_t sTexture )
{
	TextureVK* texture = VK_GetTextureNoMissing( sTexture );

	if ( texture == nullptr )
	{
		Log_Error( "Failed to find texture to destroy\n" );
		return;
	}

	if ( texture->apName )
		VK_FreeString( texture->apName );

	// big hack, blech
	if ( !texture->aSwapChain )
	{
		if ( texture->aImageView )
			vkDestroyImageView( VK_GetDevice(), texture->aImageView, nullptr );

		if ( texture->aMemory )
		{
			if ( texture->aImage )
				vkDestroyImage( VK_GetDevice(), texture->aImage, nullptr );

			vkFreeMemory( VK_GetDevice(), texture->aMemory, nullptr );
		}

		if ( texture->aUsage & VK_IMAGE_USAGE_SAMPLED_BIT )
		{
			gGraphicsAPIData.aSampledTextures.erase( sTexture );
		}
	}

	gTextureHandles.Remove( sTexture );
	delete texture;
}


// Used on shutdown
void VK_DestroyAllTextures()
{
	for ( u32 i = 0; i < gTextureHandles.aHandles.size(); i++ )
	{
		ChHandle_t texHandle = gTextureHandles.aHandles[ i ];
		TextureVK* tex       = nullptr;
		if ( !gTextureHandles.Get( texHandle, &tex ) )
		{
			Log_Error( "Failed to find texture data when destroying all textures\n" );
			continue;
		}

		// basically an ugly hack
		if ( tex->aSwapChain )
			continue;

		vkDestroyImageView( VK_GetDevice(), tex->aImageView, nullptr );
		vkDestroyImage( VK_GetDevice(), tex->aImage, nullptr );

		// TODO: probably change this so i don't call delete on the texture, and maybe i can reuse this image memory somehow
		vkFreeMemory( VK_GetDevice(), tex->aMemory, nullptr );
		delete tex;
	}

	VK_DestroyTextureSamplers();

	gTextureHandles.clear();
}


TextureVK* VK_GetTexture( ChHandle_t sTexture )
{
	TextureVK* find = nullptr;

	if ( sTexture != 0 && gTextureHandles.Get( sTexture, &find ) )
	{
		// Image is loaded
		return find;
	}

	if ( sTexture != 0 && sTexture != gMissingTexHandle )
		Log_ErrorF( gLC_Render, "Failed to find texture from handle %zd!\n", sTexture );

	if ( gTextureHandles.Get( gMissingTexHandle, &find ) )
		return find;

	Log_Fatal( gLC_Render, "Missing Texture is not in texture list?\n" );
	return nullptr;
}


TextureVK* VK_GetTextureNoMissing( ChHandle_t sTexture )
{
	TextureVK* find = nullptr;
	if ( gTextureHandles.Get( sTexture, &find ) )
		return find;

	return nullptr;
}


Handle VK_CreateFramebuffer( const char* name, VkRenderPass sRenderPass, u16 sWidth, u16 sHeight, const VkImageView* spAttachments, u32 sCount )
{
	VkFramebufferCreateInfo framebufferInfo{ VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO };
	framebufferInfo.renderPass      = sRenderPass;
	framebufferInfo.attachmentCount = sCount;
	framebufferInfo.pAttachments    = spAttachments;
	framebufferInfo.width           = sWidth;
	framebufferInfo.height          = sHeight;
	framebufferInfo.layers          = 1;

	VkFramebuffer buffer            = VK_NULL_HANDLE;
	VK_CheckResultF( vkCreateFramebuffer( VK_GetDevice(), &framebufferInfo, nullptr, &buffer ), "Failed to create framebuffer \"%s\"", name ? name : "unnamed" );
	VK_SetObjectName( VK_OBJECT_TYPE_FRAMEBUFFER, (u64)buffer, name );

	return gFramebuffers.Add( buffer );
}


Handle VK_CreateFramebuffer( const char* name, const VkFramebufferCreateInfo& sCreateInfo )
{
	VkFramebuffer buffer = VK_NULL_HANDLE;
	VK_CheckResultF( vkCreateFramebuffer( VK_GetDevice(), &sCreateInfo, nullptr, &buffer ), "Failed to create framebuffer \"%s\"", name ? name : "unnamed" );
	VK_SetObjectName( VK_OBJECT_TYPE_FRAMEBUFFER, (u64)buffer, name );

	return gFramebuffers.Add( buffer );
}


Handle VK_CreateFramebuffer( const CreateFramebuffer_t& srCreate )
{
	VkRenderPass renderPass = VK_GetRenderPass( srCreate.aRenderPass );
	if ( renderPass == VK_NULL_HANDLE )
	{
		Log_Error( gLC_Render, "Failed to create Framebuffer: RenderPass not found!\n" );
		return InvalidHandle;
	}

	size_t count = srCreate.aPass.aAttachColors.size();
	count += srCreate.aPass.aAttachResolve.size();

	if ( srCreate.aPass.aAttachDepth )
		count++;

	VkImageView* attachments = CH_MALLOC( VkImageView, count );

	if ( attachments == nullptr )
	{
		Log_ErrorF( gLC_Render, "STACK OVERFLOW: Failed to allocate stack for Framebuffer attachments (%zu bytes)\n", sizeof( VkImageView ) * count );
		ch_free( attachments );
		return InvalidHandle;
	}

	count = 0;
	for ( size_t i = 0; i < srCreate.aPass.aAttachColors.size(); i++ )
	{
		TextureVK* tex = nullptr;
		if ( !gTextureHandles.Get( srCreate.aPass.aAttachColors[ i ], &tex ) )
		{
			Log_ErrorF( gLC_Render, "Failed to find color texture %u while creating Framebuffer\n", i );
			ch_free( attachments );
			return InvalidHandle;
		}

		attachments[ count++ ] = tex->aImageView;
	}

	if ( srCreate.aPass.aAttachDepth != InvalidHandle )
	{
		TextureVK* tex = nullptr;
		if ( !gTextureHandles.Get( srCreate.aPass.aAttachDepth, &tex ) )
		{
			Log_Error( gLC_Render, "Failed to find depth texture while creating Framebuffer\n" );
			ch_free( attachments );
			return InvalidHandle;
		}

		attachments[ count++ ] = tex->aImageView;
	}

	for ( size_t i = 0; i < srCreate.aPass.aAttachResolve.size(); i++ )
	{
		TextureVK* tex = nullptr;
		if ( !gTextureHandles.Get( srCreate.aPass.aAttachResolve[ i ], &tex ) )
		{
			Log_ErrorF( gLC_Render, "Failed to find resolve texture %u while creating Framebuffer\n", i );
			ch_free( attachments );
			return InvalidHandle;
		}

		attachments[ count++ ] = tex->aImageView;
	}

	Handle handle = VK_CreateFramebuffer( srCreate.apName, renderPass, srCreate.aSize.x, srCreate.aSize.y, attachments, count );

	if ( handle != InvalidHandle )
	{
		gFramebufferSize[ handle ] = srCreate.aSize;
	}

	ch_free( attachments );
	return handle;
}


Handle VK_CreateFramebufferVK( const CreateFramebufferVK& srCreate )
{
	size_t count = srCreate.aAttachColors.size();
	count += srCreate.aAttachResolve.size();

	if ( srCreate.aAttachDepth )
		count++;

	VkImageView* attachments = CH_MALLOC( VkImageView, count );

	if ( attachments == nullptr )
	{
		Log_ErrorF( gLC_Render, "STACK OVERFLOW: Failed to allocate stack for Framebuffer attachments (%zu bytes)\n", sizeof( VkImageView ) * count );
		ch_free( attachments );
		return InvalidHandle;
	}

	count = 0;
	for ( size_t i = 0; i < srCreate.aAttachColors.size(); i++ )
	{
		attachments[ count++ ] = srCreate.aAttachColors[ i ];
	}

	if ( srCreate.aAttachDepth != VK_NULL_HANDLE )
	{
		attachments[ count++ ] = srCreate.aAttachDepth;
	}

	for ( size_t i = 0; i < srCreate.aAttachResolve.size(); i++ )
	{
		attachments[ count++ ] = srCreate.aAttachResolve[ i ];
	}

	Handle handle = VK_CreateFramebuffer( srCreate.apName, srCreate.aRenderPass, srCreate.aSize.x, srCreate.aSize.y, attachments, count );

	if ( handle != InvalidHandle )
	{
		gFramebufferSize[ handle ] = srCreate.aSize;
	}

	ch_free( attachments );
	return handle;
}


void VK_DestroyFramebuffer( Handle shHandle )
{
	VkFramebuffer buffer = VK_NULL_HANDLE;
	if ( !gFramebuffers.Get( shHandle, &buffer ) )
	{
		Log_Error( gLC_Render, "Failed to Get Framebuffer to destroy\n" );
		return;
	}

	vkDestroyFramebuffer( VK_GetDevice(), buffer, nullptr );
	gFramebuffers.Remove( shHandle );
	gFramebufferSize.erase( shHandle );
}


VkFramebuffer VK_GetFramebuffer( Handle shHandle )
{
	VkFramebuffer buffer = VK_NULL_HANDLE;
	if ( !gFramebuffers.Get( shHandle, &buffer ) )
	{
		Log_Error( gLC_Render, "Failed to Get Framebuffer\n" );
		return VK_NULL_HANDLE;
	}

	return buffer;
}

glm::uvec2 VK_GetFramebufferSize( Handle shHandle )
{
	auto it = gFramebufferSize.find( shHandle );
	if ( it == gFramebufferSize.end() )
	{
		Log_Error( gLC_Render, "Failed to Get Framebuffer Size\n" );
		return {};
	}

	return it->second;
}


void VK_CreateRenderTargetInt( RenderTarget* target, const std::vector< TextureVK* >& srImages, u16 sWidth, u16 sHeight )
{
#if 0
	target->aImages.resize( srImages.size() );
	target->aFrameBuffers.resize( srSwapImages.size() );

	for ( size_t i = 0; i < srImages.size(); ++i )
		target->aImages[ i ] = srImages[ i ];

	for ( u32 i = 0; i < VK_GetSwapImageCount(); ++i )
	{
		std::vector< VkImageView > attachments;

		if ( VK_UseMSAA() )
		{
			for ( auto image : srImages )
			{
				attachments.push_back( image->aImageView );
			}

			// color resolve images?
			if ( srSwapImages.size() )
			{
				attachments.push_back( srSwapImages[ i ] );
			}
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
#endif
}


RenderTarget* VK_CreateRenderTarget( const std::vector< TextureVK* >& srImages, u16 sWidth, u16 sHeight )
{
	auto target = gRenderTargets.emplace_back( new RenderTarget );
	VK_CreateRenderTargetInt( target, srImages, sWidth, sHeight );
	return target;
}


void VK_DestroyRenderTarget( RenderTarget* spTarget )
{
	if ( !spTarget )
		return;

	for ( auto frameBuffer : spTarget->aFrameBuffers )
	{
		vkDestroyFramebuffer( VK_GetDevice(), frameBuffer.aBuffer, nullptr );
		gFramebuffers.Remove( gFramebufferHandles[ frameBuffer.aBuffer ] );
		gFramebufferHandles.erase( frameBuffer.aBuffer );
	}

	for ( auto& texture : spTarget->aColors )
		VK_DestroyTexture( texture );

	for ( auto& texture : spTarget->aResolve )
		VK_DestroyTexture( texture );

	if ( spTarget->aDepth )
		VK_DestroyTexture( spTarget->aDepth );

	vec_remove( gRenderTargets, spTarget );
	delete spTarget;
}


void VK_DestroyRenderTargets()
{
	while ( gRenderTargets.size() )
	{
		VK_DestroyRenderTarget( gRenderTargets[ 0 ] );
	}
}


static TextureVK* CreateBackBufferColor( WindowVK* window, const char* title, ChHandle_t& srHandle )
{
	TextureVK* colorTex     = VK_NewTexture( srHandle );
	colorTex->apName        = VK_AllocStringF( "Backbuffer Color - %s", title );
	colorTex->aRenderTarget = true;

	VkImageCreateInfo color{ VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
	color.pNext                 = nullptr;
	color.flags                 = 0;
	color.imageType             = VK_IMAGE_TYPE_2D;
	color.format                = window->swapSurfaceFormat.format;
	color.extent.width          = window->swapExtent.width;
	color.extent.height         = window->swapExtent.height;
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

	// VK_CheckResult( vkCreateImage( VK_GetDevice(), &color, nullptr, &colorTex->aImage ), "Failed to create color image!" );

	VK_CreateImage( color, colorTex );

	// VkMemoryRequirements memReqs;
	// vkGetImageMemoryRequirements( VK_GetDevice(), colorTex->aImage, &memReqs );
	// 
	// VkMemoryAllocateInfo allocInfo{ VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
	// allocInfo.pNext           = nullptr;
	// allocInfo.allocationSize  = memReqs.size;
	// allocInfo.memoryTypeIndex = VK_GetMemoryType( memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT );
	// 
	// VK_CheckResult( vkAllocateMemory( VK_GetDevice(), &allocInfo, nullptr, &colorTex->aMemory ), "Failed to allocate color image memory!" );
	// VK_CheckResult( vkBindImageMemory( VK_GetDevice(), colorTex->aImage, colorTex->aMemory, 0 ), "Failed to bind back buffer color image memory" );

	VkImageViewCreateInfo colorView{ VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
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

	VK_SetObjectName( VK_OBJECT_TYPE_IMAGE_VIEW, (u64)colorTex->aImageView, VK_AllocStringF( "Backbuffer Color - %s", title ) );

	return colorTex;
}


void VK_CreateBackBuffer( WindowVK* window )
{
	/*
     *    Our backbuffer contains either 2 or 3 render operations depending on MSAA: Color, Depth, and Resolve,
     *    so we'll make those now.
     */

	const char* title = SDL_GetWindowTitle( window->window );

	if ( title == nullptr )
		title = CH_DEFAULT_WINDOW_NAME;

	ChHandle_t colorHandle = CH_INVALID_HANDLE;
	ChHandle_t depthHandle = CH_INVALID_HANDLE;

	// ------------------------------------------------------
	// Create Depth Texture

	TextureVK*  depthTex     = VK_NewTexture( depthHandle );
	depthTex->apName         = VK_AllocStringF( "Backbuffer Depth - %s", title );
	depthTex->aRenderTarget = true;

	VkImageCreateInfo depth;
	depth.sType                 = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	depth.pNext                 = nullptr;
	depth.flags                 = 0;
	depth.imageType             = VK_IMAGE_TYPE_2D;
	depth.format                = VK_GetDepthFormat();
	depth.extent.width          = window->swapExtent.width;
	depth.extent.height         = window->swapExtent.height;
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

	// VK_CheckResult( vkCreateImage( VK_GetDevice(), &depth, nullptr, &depthTex->aImage ), "Failed to create depth image!" );

	VK_CreateImage( depth, depthTex );

	// VkMemoryRequirements memReqs;
	// vkGetImageMemoryRequirements( VK_GetDevice(), depthTex->aImage, &memReqs );
	// 
	// VkMemoryAllocateInfo allocInfo{ VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
	// allocInfo.pNext           = nullptr;
	// allocInfo.allocationSize  = memReqs.size;
	// allocInfo.memoryTypeIndex = VK_GetMemoryType( memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT );
	// 
	// VK_CheckResult( vkAllocateMemory( VK_GetDevice(), &allocInfo, nullptr, &depthTex->aMemory ), "Failed to allocate depth image memory!" );
	// VK_CheckResult( vkBindImageMemory( VK_GetDevice(), depthTex->aImage, depthTex->aMemory, 0 ), "Failed to bind back buffer depth image memory" );

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

	VK_SetObjectName( VK_OBJECT_TYPE_IMAGE_VIEW, (u64)depthTex->aImageView, VK_AllocStringF( "Backbuffer Depth View - %s", title ) );

	// TODO: wtf use colorView and depthView, i love memory leaks lol
	// Log_Warn( gLC_Render, "why am i not using the colorView and depthView we created???!?!?!!?\n" );

	RenderTarget* target = new RenderTarget;
	gRenderTargets.push_back( target );

	TextureVK* colorTex = nullptr;

	// gBackbufferHandles.clear();

	// gBackbufferHandles[ swapTextures[ 0 ] ] = gSwapImageHandles[ 0 ];
	// gBackbufferHandles[ swapTextures[ 1 ] ] = gSwapImageHandles[ 1 ];

	if ( VK_UseMSAA() )
	{
		colorTex = CreateBackBufferColor( window, title, colorHandle );
		// gBackbufferHandles[ colorTex ] = colorHandle;

		target->aColors.push_back( colorHandle );
	}
	else
	{
		ChHandle_t texHandle = CH_INVALID_HANDLE;
		TextureVK* tex       = VK_NewTexture( texHandle );
		tex->apName          = VK_AllocStringF( "Backbuffer Color - %s", title );
		tex->aFormat         = gColorFormat;
		tex->aFrames         = 1;
		tex->aMemory         = VK_NULL_HANDLE;
		tex->aMipLevels      = 1;
		tex->aUsage          = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
		tex->aImageView      = window->swapImageViews[ 0 ];
		tex->aSwapChain      = true;

		// uhh
		// target->aColors.push_back( swapTextures[ 0 ] );
		target->aColors.push_back( texHandle );
	}

	// gBackbufferHandles[ depthTex ] = depthHandle;

	target->aDepth = depthHandle;

	if ( VK_UseMSAA() )
	{
		ChHandle_t resolveHandle = CH_INVALID_HANDLE;
		TextureVK* tex           = VK_NewTexture( resolveHandle );
		tex->apName              = VK_AllocStringF( "Backbuffer Resolve - %s", title );
		tex->aFormat             = gColorFormat;
		tex->aFrames             = 1;
		tex->aMemory             = VK_NULL_HANDLE;
		tex->aMipLevels          = 1;
		tex->aUsage              = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
		tex->aImageView          = window->swapImageViews[ 1 ];
		tex->aSwapChain          = true;

		// target->aResolve.push_back( swapTextures[ 0 ] );
		// target->aResolve.push_back( swapTextures[ 1 ] );
		target->aResolve.push_back( resolveHandle );
		// target->aResolve[ 0 ]->aUsage |= VK_IMAGE_USAGE_SAMPLED_BIT; // WHAT
	}

	CreateFramebufferVK createBuffer{};
	createBuffer.apName       = VK_AllocStringF( "Backbuffer Color - %s", title );
	createBuffer.aRenderPass  = VK_GetRenderPass();
	createBuffer.aSize.x      = window->swapExtent.width;
	createBuffer.aSize.y      = window->swapExtent.height;

	createBuffer.aAttachDepth = depthTex->aImageView;

	if ( VK_UseMSAA() )
	{
		createBuffer.aAttachColors.push_back( colorTex->aImageView );
		createBuffer.aAttachResolve.push_back( window->swapImageViews[ 0 ] );
	}
	else
	{
		createBuffer.aAttachColors.push_back( window->swapImageViews[ 0 ] );
	}

	Handle frameBufColorHandle = VK_CreateFramebufferVK( createBuffer );

	if ( VK_UseMSAA() )
	{
		createBuffer.aAttachResolve[ 0 ] = window->swapImageViews[ 1 ];
	}
	else
	{
		createBuffer.aAttachColors[ 0 ] = window->swapImageViews[ 1 ];
	}

	createBuffer.apName        = VK_AllocStringF( "Backbuffer Depth - %s", title );
	Handle frameBufDepthHandle = VK_CreateFramebufferVK( createBuffer );

	target->aFrameBuffers.resize( 2 );
	target->aFrameBuffers[ 0 ].aBuffer                        = VK_GetFramebuffer( frameBufColorHandle );
	target->aFrameBuffers[ 1 ].aBuffer                        = VK_GetFramebuffer( frameBufDepthHandle );
	target->aFrameBuffers[ 0 ].aType                          = EAttachmentType_Color;
	target->aFrameBuffers[ 1 ].aType                          = EAttachmentType_Color;

	gFramebufferHandles[ target->aFrameBuffers[ 0 ].aBuffer ] = frameBufColorHandle;
	gFramebufferHandles[ target->aFrameBuffers[ 1 ].aBuffer ] = frameBufDepthHandle;

#if 0
	for ( u32 i = 0; i < swapTextures.size(); ++i )
	{
		std::vector< VkImageView > attachments;

		if ( VK_UseMSAA() )
		{
			for ( auto image : target->aImages )
			{
				attachments.push_back( image->aImageView );
			}

			// color resolve images?
			if ( swapTextures.size() )
			{
				attachments.push_back( swapTextures[ i ]->aImageView );
			}
		}
		else
		{
			if ( swapTextures.size() )
				attachments.push_back( swapTextures[ i ]->aImageView );

			attachments.push_back( target->aImages[ 1 ]->aImageView );
		}

		VkFramebufferCreateInfo framebufferInfo{ VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO };
		framebufferInfo.renderPass      = VK_GetRenderPass();
		framebufferInfo.attachmentCount = attachments.size();
		framebufferInfo.pAttachments    = attachments.data();
		framebufferInfo.width           = VK_GetSwapExtent().width;
		framebufferInfo.height          = VK_GetSwapExtent().height;
		framebufferInfo.layers          = 1;

		VK_CheckResult( vkCreateFramebuffer( VK_GetDevice(), &framebufferInfo, nullptr, &target->aFrameBuffers[ i ] ), "Failed to create framebuffer" );

		gFramebufferHandles[ target->aFrameBuffers[ i ] ] = gFramebuffers.Add( target->aFrameBuffers[ i ] );
	}
#endif

	// RenderTarget* rt = VK_CreateRenderTarget( { colorTex, depthTex }, VK_GetSwapExtent().width, VK_GetSwapExtent().height, VK_GetSwapImageViews() );

	window->backbuffer = target;
}


Handle VK_GetFramebufferHandle( VkFramebuffer sFrameBuffer )
{
	auto it = gFramebufferHandles.find( sFrameBuffer );
	if ( it != gFramebufferHandles.end() )
	{
		return it->second;
	}

	Log_Error( gLC_Render, "Framebuffer Handle not found!\n" );
	return InvalidHandle;
}


void VK_CreateImage( VkImageCreateInfo& srCreateInfo, TextureVK* spTexture )
{
	Log_Dev( gLC_Render, 2, "Creating Image\n" );

	// VmaAllocationCreateInfo allocInfo = {};
	// allocInfo.usage                   = VMA_MEMORY_USAGE_AUTO;
	// allocInfo.memoryTypeBits          = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

	VK_CheckResult( vkCreateImage( VK_GetDevice(), &srCreateInfo, nullptr, &spTexture->aImage ), "Failed to create image!" );

	VK_SetObjectName( VK_OBJECT_TYPE_IMAGE, (u64)spTexture->aImage, spTexture->apName );

	VkMemoryRequirements memReqs;
	vkGetImageMemoryRequirements( VK_GetDevice(), spTexture->aImage, &memReqs );
	spTexture->aMemorySize = memReqs.size;

	VkMemoryAllocateInfo allocInfo{ VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
	allocInfo.pNext           = nullptr;
	allocInfo.allocationSize  = memReqs.size;

	// if ( spTexture->aBufferMemory == EBufferMemory_Host )
	// {
	// 	allocInfo.memoryTypeIndex = VK_GetMemoryType( memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT );
	// }
	// else
	{
		allocInfo.memoryTypeIndex = VK_GetMemoryType( memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT );
	}

	VK_CheckResult( vkAllocateMemory( VK_GetDevice(), &allocInfo, nullptr, &spTexture->aMemory ), "Failed to allocate image memory!" );
	VK_CheckResult( vkBindImageMemory( VK_GetDevice(), spTexture->aImage, spTexture->aMemory, 0 ), "Failed to bind image memory" );

	VK_SetObjectName( VK_OBJECT_TYPE_DEVICE_MEMORY, (u64)spTexture->aMemory, spTexture->apName );
}


void VK_CreateMissingTexture()
{
	Log_Dev( gLC_Render, 1, "Creating Missing Texture\n" );

	TextureCreateInfo_t create{};
	create.apName    = "__missing";
	create.aSize.x   = gMissingTextureWidth;
	create.aSize.y   = gMissingTextureHeight;
	create.aFormat   = GraphicsFmt::RGBA8888_SRGB;
	create.aViewType = EImageView_2D;
	create.apData    = ( void* )gpMissingTexture;
	create.aDataSize = gMissingTextureWidth * gMissingTextureHeight * 4;

	TextureCreateData_t data{};
	data.aUseMSAA  = false;
	data.aUsage    = EImageUsage_Sampled;
	data.aFilter   = EImageFilter_Nearest;

	TextureVK* tex = VK_CreateTexture( gMissingTexHandle, create, data );

	if ( !tex )
	{
		Log_Fatal( gLC_Render, "Failed to create missing texture!\n" );
		return;
	}

	gNeedTextureUpdate = true;
}

