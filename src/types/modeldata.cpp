/*
modeldata.cpp ( Authored by p0lyh3dron )

Defines the ModelData class defined in
modeldata.h.  
*/
#include "../../inc/types/modeldata.h"
/* Allocates the model, and loads its textures etc.  */
void ModelData::Init(  )
{
	aUniformLayout = InitDescriptorSetLayout( { { DescriptorLayoutBinding( VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT, NULL ) } } );
	aTextureLayout = InitDescriptorSetLayout( { { DescriptorLayoutBinding( VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, NULL ) } } );
	InitUniformData( aUniformData, aUniformLayout );
}
/* Reinitialize data that is useless after the swapchain becomes outdated.  */
void ModelData::Reinit(  )
{	
	InitUniformData( aUniformData, aUniformLayout );
}
/* Binds the model data to the GPU to be rendered later.  */
void ModelData::Bind( VkCommandBuffer c, uint32_t i )
{
	if ( aMeshes.GetSize(  ) ) aNoDraw = false;
	if ( !aNoDraw )
	{
		VkBuffer 	vBuffers[  ] 	= { aVertexBuffer };
		VkDeviceSize 	offsets[  ] 	= { 0 };
		vkCmdBindVertexBuffers( c, 0, 1, vBuffers, offsets );
		vkCmdBindIndexBuffer( c, aIndexBuffer, 0, VK_INDEX_TYPE_UINT32 );
	}
}
/* Draws the model to the screen.  */
void ModelData::Draw( VkCommandBuffer c, uint32_t i )
{
	if ( !aNoDraw )
	{
		for ( uint32_t j = 0; j < aMeshes.GetSize(  ); ++j )
		{
			if ( aMeshes.IsValid(  ) )
			{
				vkCmdBindPipeline( c, VK_PIPELINE_BIND_POINT_GRAPHICS, aMeshes[ j ].apShader->GetPipeline(  ) );
				VkDescriptorSet sets[  ] = { aMeshes[ j ].apTexture->aSets[ i ], aUniformData.aSets[ i ] };
				vkCmdBindDescriptorSets( c, VK_PIPELINE_BIND_POINT_GRAPHICS, aMeshes[ j ].apShader->GetPipelineLayout(  ), 0, 2, sets, 0, NULL );
				vkCmdDrawIndexed( c, aMeshes[ j ].aShape.aIndexCount, 1, aMeshes[ j ].aShape.aOffset, 0, 0 );
			}
		}
	}
}
/* Adds a material to the model.  */
void ModelData::AddMaterial( const std::string &srTexturePath, uint32_t sMaterialId, VkSampler sSampler )
{
/*	aMeshes.Increment(  );
	aMeshes.GetBuffer(  )[ aMeshes.GetSize(  ) ].aTexture = InitTexture( srTexturePath, aTextureLayout, *gpPool, sSampler ); */
	/* add material draw thingy */
}
/* Sets the shader for use at draw time.  */
void ModelData::SetShader( BaseShader *spShader )
{
	apShader = spShader;
}
/* Adds a mesh to the model.  */
void ModelData::AddMesh( const std::string &srTexturePath, uint32_t sIndexCount, uint32_t sIndexOffset, VkSampler sSampler )
{
	aMeshes.Increment(  );
	/* Construct mesh with index count and offset into index buffer to construct faces, and load material via a file.  */
	aMeshes.GetTop(  ) = { { sIndexCount, sIndexOffset }, InitTexture( srTexturePath, aTextureLayout, *gpPool, sSampler ), apShader };
}
/* Adds an index group to the model which groups together indices in the same material to be used for multiple draw calls for multiple textures.  */
void ModelData::AddIndexGroup( std::vector< uint32_t > sVec )
{
	
}
/* Default the model and set limits.  */
/* Frees the memory used by objects outdated by a new swapchain state.  */
void ModelData::FreeOldResources(  )
{
	/* Destroy Vulkan objects, keeping the allocations for their storage units.  */
	vkDestroyPipeline( DEVICE, aPipeline, NULL );
	vkDestroyPipelineLayout( DEVICE, aPipelineLayout, NULL );
	/* Ugly, fix later.  */
	for ( uint32_t i = 0; i < PSWAPCHAIN.GetImages(  ).size(  ); ++i )
	{
		/* Free uniform data.  */
		vkDestroyBuffer( DEVICE, aUniformData.aData.GetBuffer(  )[ i ], NULL );
		vkFreeMemory( DEVICE, aUniformData.aMem.GetBuffer(  )[ i ], NULL );
	}
}
/* Frees all memory used by model.  */
ModelData::~ModelData(  )
{
	FreeOldResources(  );
	for ( auto&& texture : gTextures )
	{
		vkDestroyImageView( DEVICE, texture->aTextureImageView, NULL );
		vkDestroyImage( DEVICE, texture->aTextureImage, NULL );
		vkFreeMemory( DEVICE, texture->aTextureImageMem, NULL );
		delete texture;
	}
	vkDestroyDescriptorSetLayout( DEVICE, aUniformLayout, NULL );
	vkDestroyDescriptorSetLayout( DEVICE, aTextureLayout, NULL );
	vkDestroyBuffer( DEVICE, aVertexBuffer, NULL );
	vkFreeMemory( DEVICE, aVertexBufferMem, NULL );
	vkDestroyBuffer( DEVICE, aIndexBuffer, NULL );
	vkFreeMemory( DEVICE, aIndexBufferMem, NULL );
	delete[  ] apVertices;
	delete[  ] apIndices;
}
