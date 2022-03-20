/*
 *	textureloader_ctx.cpp ( Authored by p0lyh3dron )
 *
 *	Implementation of .ctx loading.
 */

#include "graphics/ctxlib.h"
#include "../graphics.h"
#include "../renderer.h"


class CTXTextureLoader : public ITextureLoader
{
public:
	explicit CTXTextureLoader() {}
	~CTXTextureLoader() override {}

	/* Checks for .ctx extension.  */
	bool CheckExt( const char* ext ) override
	{
		return ( strncmp( "ctx", ext, 3 ) == 0 );
	}

	/* Loads the texture into vulkan objects.  */
	TextureDescriptor* LoadTexture( const std::string path ) override
	{
		ctx_t *pTex = ctx_open( path.c_str() );

		if ( !pTex )
		{
			LogError( gGraphicsChannel, "oops no ctx\n" );
			return nullptr;
		}

		int 		texWidth              = pTex->aHead.aWidth;
		int		texHeight             = pTex->aHead.aHeight;
		VkBuffer 	stagingBuffer;
		VkDeviceMemory 	stagingBufferMemory;
		void           *pData                 = pTex->appImages[ 0 ][ 0 ].apData;
		int             dataSize              = pTex->appImages[ 0 ][ 0 ].aSize;

		TextureDescriptor	*pTexture = new TextureDescriptor;

		VkDeviceSize imageSize = texWidth * texHeight * 4;

		/* Divides res by 2 each level.  */
		pTexture->aMipLevels = ( uint32_t )std::floor( std::log2( std::max( texWidth, texHeight ) ) ) + 1;

		InitBuffer( imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			stagingBuffer, stagingBufferMemory );

		MapMemory( stagingBufferMemory, imageSize, pData );

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

		ctx_free( pTex );

		return pTexture;
	}
};


CTXTextureLoader gCTXTextureLoader;

