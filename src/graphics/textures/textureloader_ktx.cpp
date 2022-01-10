/*
textureloader_ktx.h ( Authored by Demez )

Load KTX Textures
*/

#include "textureloader_ktx.h"
#include "../renderer.h"
#include "core/console.h"
#include "util.h"

#ifdef KTX
#include "ktx/ktx.h"
#include "ktx/ktxvulkan.h"
#endif


KTXTextureLoader::KTXTextureLoader()
{
}

KTXTextureLoader::~KTXTextureLoader()
{
}


bool KTXTextureLoader::CheckExt( const char* ext )
{
	return (strncmp( "ktx", ext, 3) == 0);
}


TextureDescriptor* KTXTextureLoader::LoadTexture( IMaterial* material, const std::string path )
{
	TextureDescriptor	*pTexture = new TextureDescriptor;

	if ( !LoadKTXTexture( pTexture, path, pTexture->aTextureImage, pTexture->aTextureImageMem, pTexture->aMipLevels ) )
		return nullptr;

	VkDescriptorSetLayout layout = ((Material*)material)->GetTextureLayout();

	InitDescriptorSets(
		pTexture->aSets,
		layout,
		*gpPool,
		{ { {
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			pTexture->aTextureImageView,
			renderer->GetTextureSampler(),
			VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER
		} } },
		{  }
	);

	return pTexture;
}


bool KTXTextureLoader::LoadKTXTexture( TextureDescriptor* pTexture, const String &srImagePath, VkImage &srTImage, VkDeviceMemory &srTImageMem, uint32_t &srMipLevels )
{
#ifndef KTX
	return false;
#else
	int 		texWidth;
	int		texHeight;
	int		texChannels;
	VkBuffer 	stagingBuffer;
	VkDeviceMemory 	stagingBufferMemory;
	bool		noTexture = false;

	ktxVulkanDeviceInfo vdi;

	KTX_error_code result = ktxVulkanDeviceInfo_Construct(&vdi, gpDevice->GetPhysicalDevice(), DEVICE,
														  gpDevice->GetGraphicsQueue(), gpDevice->GetCommandPool(), nullptr);

	if ( result != KTX_SUCCESS )
	{
		Print( "KTX Error %d: %s - Failed to Construct KTX Vulkan Device\n", result, ktxErrorString(result) );
		return false;
	}

	result = ktxTexture_CreateFromNamedFile(srImagePath.c_str(), KTX_TEXTURE_CREATE_NO_FLAGS, &pTexture->kTexture);

	if ( result != KTX_SUCCESS )
	{
		Print( "KTX Error %d: %s - Failed to open texture: %s\n", result, ktxErrorString(result), srImagePath.c_str(  ) );
		ktxVulkanDeviceInfo_Destruct( &vdi );
		return false;
	}

	result = ktxTexture_VkUploadEx(pTexture->kTexture, &vdi, &pTexture->texture,
								   VK_IMAGE_TILING_OPTIMAL,
								   VK_IMAGE_USAGE_SAMPLED_BIT,
								   VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

	if ( result != KTX_SUCCESS )
	{
		Print( "KTX Error %d: %s - Failed to upload texture: %s\n", result, ktxErrorString(result), srImagePath.c_str(  ) );
		ktxTexture_Destroy( pTexture->kTexture );
		ktxVulkanDeviceInfo_Destruct( &vdi );
		return false;
	}

	Print( "Loaded Image: %s - dataSize: %d\n", srImagePath.c_str(  ), pTexture->kTexture->dataSize );

	ktxTexture_Destroy( pTexture->kTexture );
	ktxVulkanDeviceInfo_Destruct( &vdi );

	pTexture->aMipLevels = pTexture->kTexture->numLevels;
	pTexture->aTextureImage = pTexture->texture.image;
	pTexture->aTextureImageMem = pTexture->texture.deviceMemory;

	InitImageView( pTexture->aTextureImageView, pTexture->aTextureImage, pTexture->texture.imageFormat, VK_IMAGE_ASPECT_COLOR_BIT, pTexture->aMipLevels );

	return true;
#endif
}


