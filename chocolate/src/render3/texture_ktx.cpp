#include "render.h"

#include "ktx.h"
#include "ktxvulkan.h"


static ktxVulkanFunctions g_ktx_funcs = {
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


constexpr ktx_transcode_fmt_e g_ktx_fallback_fmt = KTX_TTF_BC7_RGBA;
// constexpr ktx_transcode_fmt_e g_ktx_fallback_fmt = KTX_TTF_RGBA32;

static ktxVulkanDeviceInfo    g_ktx_vdi;

bool ktx_init()
{
	KTX_error_code result = ktxVulkanDeviceInfo_ConstructEx(
	  &g_ktx_vdi,
      g_vk_instance,
      g_vk_physical_device,
      g_vk_device,
      g_vk_queue_transfer,
      g_vk_command_pool_transfer,
      nullptr,
	  &g_ktx_funcs );

	if ( result != KTX_SUCCESS )
	{
		Log_ErrorF( gLC_Render, "KTX Error %d: %s - Failed to Construct KTX Vulkan Device\n", result, ktxErrorString( result ) );
		return false;
	}

	return true;
}


void ktx_shutdown()
{
	ktxVulkanDeviceInfo_Destruct( &g_ktx_vdi );
}


static bool load_ktx2( ktxTexture2* spKTexture2 )
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
		result = ktxTexture2_TranscodeBasis( spKTexture2, g_ktx_fallback_fmt, 0 );

		if ( KTX_SUCCESS != result )
		{
			Log_ErrorF( gLC_Render, "Transcoding of ktxTexture2 to %s failed: %s\n", ktxTranscodeFormatString( g_ktx_fallback_fmt ), ktxErrorString( result ) );
			return false;
		}
	}

	return true;
}


bool ktx_load( const char* path, vk_texture_t* texture )
{
	ktxTexture* kTexture = nullptr;

	KTX_error_code result   = ktxTexture_CreateFromNamedFile( path, KTX_TEXTURE_CREATE_NO_FLAGS, &kTexture );

	if ( result != KTX_SUCCESS )
	{
		Log_ErrorF( gLC_Render, "KTX Error %d: %s - Failed to open texture: %s\n", result, ktxErrorString( result ), path );
		return false;
	}

	VkFormat vkFormat = ktxTexture_GetVkFormat( kTexture );

	// WHY does this happen so often, wtf
	if ( vkFormat == VK_FORMAT_UNDEFINED )
		Log_DevF( gLC_Render, 2, "KTX Warning: - No Vulkan Format Found in KTX File: %s\n", path );

	// This is a KTX 2 Format file, so we need to have special handling for that, like transcoding
	if ( kTexture->classId == class_id::ktxTexture2_c )
		if ( !load_ktx2( (ktxTexture2*)kTexture ) )
			return false;

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
		Log_ErrorF( gLC_Render, "KTX Error %d: %s - Failed to upload texture: %s\n", result, ktxErrorString( result ), path );
		ktxTexture_Destroy( kTexture );
		return false;
	}

	Log_DevF( gLC_Render, 2, "Loaded Image: %s - dataSize: %d\n", path, kTexture->dataSize );

	texture->extent.width  = kTexture->baseWidth;
	texture->extent.height = kTexture->baseHeight;
	texture->extent.depth  = kTexture->baseDepth;
	texture->mip_count     = kTexture->numLevels;
	texture->frame_count   = kVkTexture.layerCount;
	texture->image         = kVkTexture.image;
	texture->memory        = kVkTexture.deviceMemory;
	texture->format        = kVkTexture.imageFormat;
	texture->data_size     = kTexture->dataSize;
	texture->usage         = VK_IMAGE_USAGE_SAMPLED_BIT;

	// hack
	if ( kVkTexture.viewType == VK_IMAGE_VIEW_TYPE_2D_ARRAY )
	{
		// only use 2D array if we have more than 1 layer
		if ( kVkTexture.layerCount > 1 )
			texture->view_type = VK_IMAGE_VIEW_TYPE_2D;
	}
	else if ( kVkTexture.viewType == VK_IMAGE_VIEW_TYPE_1D_ARRAY )
	{
		// only use 1D array if we have more than 1 layer
		if ( kVkTexture.layerCount > 1 )
			texture->view_type = VK_IMAGE_VIEW_TYPE_1D;
	}
	else
	{
		texture->view_type = kVkTexture.viewType;
	}

	// Create Image View
	// IDEA: what if we had different image views depending on what it's used for, like a 2D array view and a normal 2D view?
	VkImageViewCreateInfo viewInfo{ VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
	viewInfo.image                           = texture->image;
	viewInfo.viewType                        = texture->view_type;
	viewInfo.format                          = texture->format;
	viewInfo.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
	viewInfo.subresourceRange.baseMipLevel   = 0;
	viewInfo.subresourceRange.levelCount     = texture->mip_count;
	viewInfo.subresourceRange.baseArrayLayer = 0;
	viewInfo.subresourceRange.layerCount     = texture->frame_count;

	vk_check( vkCreateImageView( g_vk_device, &viewInfo, nullptr, &texture->view ), "Failed to create Image View" );

	vk_set_name( VK_OBJECT_TYPE_DEVICE_MEMORY, (u64)texture->memory, path );
	vk_set_name( VK_OBJECT_TYPE_IMAGE_VIEW, (u64)texture->view, path );

	ktxTexture_Destroy( kTexture );
	return true;
}

