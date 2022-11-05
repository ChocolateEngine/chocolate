#include "render_vk.h"

#include "ktx.h"
#include "ktxvulkan.h"


struct KTXTexture_t
{
	ktxTexture*       apTexture;
	ktxVulkanTexture* apVkTexture;
};


constexpr ktx_transcode_fmt_e gKtxFallbackFmt = KTX_TTF_BC7_RGBA;


static bool LoadKTX2( ktxTexture2* spKTexture2, ktxVulkanDeviceInfo& vdi )
{
	// ktxTexture2_CreateFromNamedFile
	KTX_error_code result = KTX_SUCCESS;

	if ( !ktxTexture2_NeedsTranscoding( spKTexture2 ) && !spKTexture2->isCompressed )
	{
		result = ktxTexture2_CompressBasis( spKTexture2, 0 );
		if ( KTX_SUCCESS != result )
		{
			Log_ErrorF( gLC_Render, "Encoding of ktxTexture2 to Basis failed: %s", ktxErrorString( result ) );
			return false;
		}

		return true;
	}

	result = ktxTexture2_TranscodeBasis( spKTexture2, gKtxFallbackFmt, 0 );

	if ( KTX_SUCCESS != result )
	{
		Log_ErrorF( gLC_Render, "Transcoding of ktxTexture2 to %s failed: %s\n", ktxTranscodeFormatString( gKtxFallbackFmt ), ktxErrorString( result ) );
		return false;
	}

	return true;
}


bool KTX_LoadTexture( TextureVK* spTexture, const char* spPath )
{
	ktxVulkanDeviceInfo vdi;

	// TODO: can i just create this once?
	KTX_error_code      result = ktxVulkanDeviceInfo_Construct(
			&vdi,
			VK_GetPhysicalDevice(),
			VK_GetDevice(),
			VK_GetGraphicsQueue(),
			VK_GetPrimaryCommandPool(),
			nullptr );

	if ( result != KTX_SUCCESS )
	{
		Log_ErrorF( gLC_Render, "KTX Error %d: %s - Failed to Construct KTX Vulkan Device\n", result, ktxErrorString( result ) );
		return false;
	}

	ktxTexture* kTexture = nullptr;

	result               = ktxTexture_CreateFromNamedFile( spPath, KTX_TEXTURE_CREATE_NO_FLAGS, &kTexture );

	if ( result != KTX_SUCCESS )
	{
		Log_ErrorF( gLC_Render, "KTX Error %d: %s - Failed to open texture: %s\n", result, ktxErrorString( result ), spPath );
		ktxVulkanDeviceInfo_Destruct( &vdi );
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
		if ( !LoadKTX2( (ktxTexture2*)kTexture, vdi ) )
		{
			ktxVulkanDeviceInfo_Destruct( &vdi );
			return false;
		}
	}

	ktxVulkanTexture kVkTexture;

	result = ktxTexture_VkUploadEx(
	  kTexture,
	  &vdi,
	  &kVkTexture,
	  VK_IMAGE_TILING_OPTIMAL,
	  VK_IMAGE_USAGE_SAMPLED_BIT,
	  VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL );

	if ( result != KTX_SUCCESS )
	{
		Log_ErrorF( gLC_Render, "KTX Error %d: %s - Failed to upload texture: %s\n", result, ktxErrorString( result ), spPath );
		ktxTexture_Destroy( kTexture );
		ktxVulkanDeviceInfo_Destruct( &vdi );
		return false;
	}

	Log_DevF( gLC_Render, 2, "Loaded Image: %s - dataSize: %d\n", spPath, kTexture->dataSize );

	spTexture->aSize.x    = kTexture->baseWidth;
	spTexture->aSize.y    = kTexture->baseHeight;
	spTexture->aMipLevels = kTexture->numLevels;
	spTexture->aImage     = kVkTexture.image;
	spTexture->aMemory    = kVkTexture.deviceMemory;
	spTexture->aFormat    = kVkTexture.imageFormat;
	spTexture->aViewType  = kVkTexture.viewType;
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

	ktxTexture_Destroy( kTexture );
	ktxVulkanDeviceInfo_Destruct( &vdi );

	return true;
}

