#include "render_vk.h"

#include "ktx.h"
#include "ktxvulkan.h"


static ktxVulkanFunctions gKtxFuncs = {
	vkGetInstanceProcAddr,
	vkGetDeviceProcAddr,

	// vkAllocateCommandBuffers,
	// vkAllocateMemory,
	// vkBeginCommandBuffer,
	// vkBindBufferMemory,
	// vkBindImageMemory,
	// vkCmdBlitImage,
	// vkCmdCopyBufferToImage,
	// vkCmdPipelineBarrier,
	// vkCreateImage,
	// vkDestroyImage,
	// vkCreateBuffer,
	// vkDestroyBuffer,
	// vkCreateFence,
	// vkDestroyFence,
	// vkEndCommandBuffer,
	// vkFreeCommandBuffers,
	// vkFreeMemory,
	// vkGetBufferMemoryRequirements,
	// vkGetImageMemoryRequirements,
	// vkGetImageSubresourceLayout,
	// vkGetPhysicalDeviceImageFormatProperties,
	// vkGetPhysicalDeviceFormatProperties,
	// vkGetPhysicalDeviceMemoryProperties,
	// vkMapMemory,
	// vkQueueSubmit,
	// vkQueueWaitIdle,
	// vkUnmapMemory,
	// vkWaitForFences,
};


constexpr ktx_transcode_fmt_e gKtxFallbackFmt = KTX_TTF_BC7_RGBA;
// constexpr ktx_transcode_fmt_e gKtxFallbackFmt = KTX_TTF_RGBA32;

static ktxVulkanDeviceInfo    g_ktx_vdi;

bool KTX_Init()
{
	KTX_error_code result = ktxVulkanDeviceInfo_ConstructEx(
	  &g_ktx_vdi,
      VK_GetInstance(),
      VK_GetPhysicalDevice(),
      VK_GetDevice(),
      VK_GetTransferQueue(),
      VK_GetTransferCommandPool(),
      nullptr,
      &gKtxFuncs );

	if ( result != KTX_SUCCESS )
	{
		Log_ErrorF( gLC_Render, "KTX Error %d: %s - Failed to Construct KTX Vulkan Device\n", result, ktxErrorString( result ) );
		return false;
	}

	return true;
}


void KTX_Shutdown()
{
	ktxVulkanDeviceInfo_Destruct( &g_ktx_vdi );
}


static bool LoadKTX2( ktxTexture2* spKTexture2 )
{
	// ktxTexture2_CreateFromNamedFile
	KTX_error_code result = KTX_SUCCESS;

	if ( !spKTexture2->isCompressed )
	{
		result = ktxTexture2_CompressBasis( spKTexture2, 0 );
		if ( KTX_SUCCESS != result )
		{
			Log_ErrorF( gLC_Render, "Encoding of ktxTexture2 to Basis failed: %s", ktxErrorString( result ) );
			return false;
		}
	}

	if ( ktxTexture2_NeedsTranscoding( spKTexture2 ) )
	{
		result = ktxTexture2_TranscodeBasis( spKTexture2, gKtxFallbackFmt, 0 );

		if ( KTX_SUCCESS != result )
		{
			Log_ErrorF( gLC_Render, "Transcoding of ktxTexture2 to %s failed: %s\n", ktxTranscodeFormatString( gKtxFallbackFmt ), ktxErrorString( result ) );
			return false;
		}
	}

	return true;
}


bool KTX_LoadTexture( TextureVK* spTexture, const char* spPath )
{
	ktxTexture* kTexture = nullptr;

	KTX_error_code result = ktxTexture_CreateFromNamedFile( spPath, KTX_TEXTURE_CREATE_NO_FLAGS, &kTexture );

	if ( result != KTX_SUCCESS )
	{
		Log_ErrorF( gLC_Render, "KTX Error %d: %s - Failed to open texture: %s\n", result, ktxErrorString( result ), spPath );
		return false;
	}

	VkFormat vkFormat = ktxTexture_GetVkFormat( kTexture );

	// WHY does this happen so often, wtf
	if ( vkFormat == VK_FORMAT_UNDEFINED )
	{
		Log_WarnF( gLC_Render, "KTX Warning: - No Vulkan Format Found in KTX File: %s\n", spPath );
	}

	if ( kTexture->classId == class_id::ktxTexture2_c )
	{
		if ( !LoadKTX2( (ktxTexture2*)kTexture ) )
		{
			return false;
		}
	}

	ktxVulkanTexture kVkTexture;

	result = ktxTexture_VkUploadEx(
	  kTexture,
	  &g_ktx_vdi,
	  &kVkTexture,
	  VK_IMAGE_TILING_OPTIMAL,
	  VK_IMAGE_USAGE_SAMPLED_BIT,
	  VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL );

	if ( result != KTX_SUCCESS )
	{
		Log_ErrorF( gLC_Render, "KTX Error %d: %s - Failed to upload texture: %s\n", result, ktxErrorString( result ), spPath );
		ktxTexture_Destroy( kTexture );
		return false;
	}

	Log_DevF( gLC_Render, 2, "Loaded Image: %s - dataSize: %d\n", spPath, kTexture->dataSize );

	spTexture->aSize.x     = kTexture->baseWidth;
	spTexture->aSize.y     = kTexture->baseHeight;
	spTexture->aMipLevels  = kTexture->numLevels;
	spTexture->aImage      = kVkTexture.image;
	spTexture->aMemory     = kVkTexture.deviceMemory;
	spTexture->aFormat     = kVkTexture.imageFormat;
	spTexture->aMemorySize = kTexture->dataSize;
	spTexture->aDataSize   = kTexture->dataSize;

	// hack
	if ( kVkTexture.viewType == VK_IMAGE_VIEW_TYPE_2D_ARRAY )
	{
		// only use 2D array if we have more than 1 layer
		if ( kVkTexture.layerCount > 1 )
			spTexture->aViewType = VK_IMAGE_VIEW_TYPE_2D;
	}
	else if ( kVkTexture.viewType == VK_IMAGE_VIEW_TYPE_1D_ARRAY )
	{
		// only use 1D array if we have more than 1 layer
		if ( kVkTexture.layerCount > 1 )
			spTexture->aViewType = VK_IMAGE_VIEW_TYPE_1D;
	}
	else
	{
		spTexture->aViewType = kVkTexture.viewType;
	}

	spTexture->aFrames    = kVkTexture.layerCount;
	spTexture->aUsage     = VK_IMAGE_USAGE_SAMPLED_BIT;

	// Create Image View
	VkImageViewCreateInfo viewInfo{ VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
	viewInfo.image                           = spTexture->aImage;
	viewInfo.viewType                        = spTexture->aViewType;
	viewInfo.format                          = spTexture->aFormat;
	viewInfo.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
	viewInfo.subresourceRange.baseMipLevel   = 0;
	viewInfo.subresourceRange.levelCount     = spTexture->aMipLevels;
	viewInfo.subresourceRange.baseArrayLayer = 0;
	viewInfo.subresourceRange.layerCount     = spTexture->aFrames;

	VK_CheckResult( vkCreateImageView( VK_GetDevice(), &viewInfo, nullptr, &spTexture->aImageView ), "Failed to create Image View" );

	VK_SetObjectName( VK_OBJECT_TYPE_DEVICE_MEMORY, (u64)spTexture->aMemory, spPath );
	VK_SetObjectName( VK_OBJECT_TYPE_IMAGE_VIEW, (u64)spTexture->aImageView, spPath );

	ktxTexture_Destroy( kTexture );
	return true;
}

