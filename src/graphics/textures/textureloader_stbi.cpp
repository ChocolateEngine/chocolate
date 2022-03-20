/*
textureloader_stbi.h ( Authored by Demez )

Load Normal Images with stb_image (blech)
*/

#include "../renderer.h"

#include "io/stb_image.h"
#include "../types/missingtexture.h"


class STBITextureLoader: public ITextureLoader
{
public:

	STBITextureLoader() {}
	~STBITextureLoader() override {}

	bool CheckExt( const char* ext ) override
	{
		// meh
		return true;
	}

	TextureDescriptor* LoadTexture( const std::string path ) override
	{
		TextureDescriptor *pTexture = new TextureDescriptor;

		int             texWidth;
		int             texHeight;
		int             texChannels;
		VkBuffer        stagingBuffer;
		VkDeviceMemory  stagingBufferMemory;

		stbi_uc 	*pPixels = stbi_load( path.c_str(  ), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha );
		bool		noTexture = false;

		if ( !pPixels )
		{
			// This texture loader handles missing texture creation, i know, awful right?
			if ( matsys->GetMissingTexture() )
			{
				LogWarn( "Failed to open texture: %s\n", path.c_str() );
				return matsys->GetMissingTexture();
			}

			texWidth 			= gMissingTextureWidth;
			texHeight 			= gMissingTextureHeight;
			pPixels				= ( stbi_uc* )gpMissingTexture;
			noTexture 			= true;
		}

		VkDeviceSize imageSize = texWidth * texHeight * 4;

		/* Divides res by 2 each level.  */
		pTexture->aMipLevels = ( uint32_t )std::floor( std::log2( std::max( texWidth, texHeight ) ) ) + 1;

		InitBuffer( imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
				   stagingBuffer, stagingBufferMemory );

		MapMemory( stagingBufferMemory, imageSize, pPixels );
		if ( !noTexture )
			stbi_image_free( pPixels );

		InitImage( Image( texWidth, texHeight, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL,
						 VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT, pTexture->aMipLevels, VK_SAMPLE_COUNT_1_BIT ),
				  VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, pTexture->aTextureImage, pTexture->aTextureImageMem );

		TransitionImageLayout( pTexture->aTextureImage, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, pTexture->aMipLevels );

		CopyBufferToImage( stagingBuffer, pTexture->aTextureImage, ( uint32_t )texWidth, ( uint32_t )texHeight );
		GenerateMipMaps( pTexture->aTextureImage, VK_FORMAT_R8G8B8A8_SRGB, texWidth, texHeight, pTexture->aMipLevels );

		/* Clean memory  */
		vkDestroyBuffer( DEVICE, stagingBuffer, NULL );
		vkFreeMemory( DEVICE, stagingBufferMemory, NULL );

		InitTextureImageView( pTexture->aTextureImageView, pTexture->aTextureImage, pTexture->aMipLevels );

		return pTexture;
	}
};


STBITextureLoader gSTBITextureLoader;

