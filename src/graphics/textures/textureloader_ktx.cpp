/*
textureloader_ktx.h ( Authored by Demez )

Load KTX Textures
*/

#include "../graphics.h"
#include "../renderer.h"

#ifdef KTX
#include "ktx.h"
#include "ktxvulkan.h"
#endif


#define LogError( channel, ... ) \
	LogError( g##channel##Channel, __VA_ARGS__ )

#define LogWarn( channel, ... ) \
	LogWarn( g##channel##Channel, __VA_ARGS__ )


class KTXTextureLoader: public ITextureLoader
{
public:
	// =============================================================
	// Vars

	// =============================================================
	// Functions

	KTXTextureLoader() {}
	~KTXTextureLoader() override {}

	bool CheckExt( const char* ext ) override
	{
		return (strncmp( "ktx", ext, 3) == 0);
	}


	TextureDescriptor* LoadTexture( const std::string srPath ) override
	{
		ktxVulkanDeviceInfo vdi;

		KTX_error_code result = ktxVulkanDeviceInfo_Construct(&vdi, gpDevice->GetPhysicalDevice(), DEVICE,
			gpDevice->GetGraphicsQueue(), gpDevice->GetCommandPool(), nullptr);

		if ( result != KTX_SUCCESS )
		{
			LogError( Graphics, "KTX Error %d: %s - Failed to Construct KTX Vulkan Device\n", result, ktxErrorString(result) );
			return nullptr;
		}

		TextureDescriptor *pTexture = new TextureDescriptor;

		result = ktxTexture_CreateFromNamedFile( srPath.c_str(), KTX_TEXTURE_CREATE_NO_FLAGS, &pTexture->kTexture );

		if ( result != KTX_SUCCESS )
		{
			LogError( Graphics, "KTX Error %d: %s - Failed to open texture: %s\n", result, ktxErrorString(result), srPath.c_str(  ) );
			ktxVulkanDeviceInfo_Destruct( &vdi );
			delete pTexture;
			return nullptr;
		}

		VkFormat vkFormat = ktxTexture_GetVkFormat( pTexture->kTexture );

		// WHY does this happen so often, wtf
		if ( vkFormat == VK_FORMAT_UNDEFINED )
		{
			LogWarn( Graphics, "KTX Warning: - No Vulkan Format Found in KTX File: %s\n", srPath.c_str(  ) );
			//ktxVulkanDeviceInfo_Destruct( &vdi );
			//delete pTexture;
			//return nullptr;
		}

		if ( pTexture->kTexture->classId == class_id::ktxTexture2_c )
		{
			if ( !LoadKTX2( pTexture, (ktxTexture2*)pTexture->kTexture, vdi ) )
			{
				ktxVulkanDeviceInfo_Destruct( &vdi );
				delete pTexture;
				return nullptr;
			}
		}

		result = ktxTexture_VkUploadEx(
			pTexture->kTexture,
			&vdi,
			&pTexture->texture,
			VK_IMAGE_TILING_OPTIMAL,
			VK_IMAGE_USAGE_SAMPLED_BIT,
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
		);

		if ( result != KTX_SUCCESS )
		{
			LogError( Graphics, "KTX Error %d: %s - Failed to upload texture: %s\n", result, ktxErrorString(result), srPath.c_str(  ) );
			ktxTexture_Destroy( pTexture->kTexture );
			ktxVulkanDeviceInfo_Destruct( &vdi );
			delete pTexture;
			return nullptr;
		}

		LogDev( gGraphicsChannel, 2, "Loaded Image: %s - dataSize: %d\n", srPath.c_str(  ), pTexture->kTexture->dataSize );

		pTexture->aMipLevels = pTexture->kTexture->numLevels;
		pTexture->aTextureImage = pTexture->texture.image;
		pTexture->aTextureImageMem = pTexture->texture.deviceMemory;

		auto i = ImageView( pTexture->aTextureImage, pTexture->texture.imageFormat, 
			VK_IMAGE_ASPECT_COLOR_BIT, pTexture->aMipLevels, pTexture->texture.viewType, pTexture->texture.layerCount );

		InitImageView( pTexture->aTextureImageView, i );

		ktxTexture_Destroy( pTexture->kTexture );
		ktxVulkanDeviceInfo_Destruct( &vdi );

		return pTexture;
	}


	bool LoadKTX2( TextureDescriptor* pTexture, ktxTexture2* kTexture2, ktxVulkanDeviceInfo& vdi )
	{
		// ktxTexture2_CreateFromNamedFile
		KTX_error_code result = KTX_SUCCESS;

		if ( !ktxTexture2_NeedsTranscoding( kTexture2 ) && !kTexture2->isCompressed )
		{
			result = ktxTexture2_CompressBasis(kTexture2, 0);
			if (KTX_SUCCESS != result)
			{
				LogError( Graphics, "Encoding of ktxTexture2 to Basis failed: %s", ktxErrorString( result ) );
				return false;
			}
		}

		// result = ktxTexture2_TranscodeBasis( kTexture2, KTX_TTF_NOSELECTION, 0 );
		result = ktxTexture2_TranscodeBasis( kTexture2, KTX_TTF_BC3_RGBA, 0 );

		if (KTX_SUCCESS != result)
		{
			LogError( Graphics, "Transcoding of ktxTexture2 to %s failed: %s\n", ktxTranscodeFormatString( KTX_TTF_BC3_RGBA ), ktxErrorString( result ) );
			return false;
		}

		return true;
	}
};


KTXTextureLoader gKTXLoader;

