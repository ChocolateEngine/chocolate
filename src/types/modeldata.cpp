/*
modeldata.cpp ( Authored by p0lyh3dron )

Defines the ModelData class defined in
modeldata.h.  
*/
#include "../../inc/types/modeldata.h"
/* Allocates the model, and loads its textures etc.  */
void ModelData::Init(  )
{
	aTextureLayout = InitDescriptorSetLayout( { { DescriptorLayoutBinding( VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1,
											   VK_SHADER_STAGE_FRAGMENT_BIT, NULL ) } } );
	
	aUniformLayout = InitDescriptorSetLayout( { { DescriptorLayoutBinding( VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1,
											   VK_SHADER_STAGE_VERTEX_BIT, NULL ) } } );
	
	InitUniformData( aUniformData, aUniformLayout );
	VkDescriptorSetLayout layouts[  ] = { aTextureLayout, aUniformLayout };
	aPipelineLayout = InitPipelineLayouts( layouts, 2 );
	InitGraphicsPipeline< vertex_3d_t >( aPipeline, aPipelineLayout, aUniformLayout, "materials/shaders/3dvert.spv",
							 "materials/shaders/3dfrag.spv", 0 );
	
}
/* Reinitialize data that is useless after the swapchain becomes outdated.  */
void ModelData::Reinit(  )
{
    	aTextureLayout = InitDescriptorSetLayout( { { DescriptorLayoutBinding( VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1,
											   VK_SHADER_STAGE_FRAGMENT_BIT, NULL ) } } );
	
	aUniformLayout = InitDescriptorSetLayout( { { DescriptorLayoutBinding( VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1,
											   VK_SHADER_STAGE_VERTEX_BIT, NULL ) } } );
	
	InitUniformData( aUniformData, aUniformLayout );
	VkDescriptorSetLayout layouts[  ] = { aTextureLayout, aUniformLayout };
	aPipelineLayout = InitPipelineLayouts( layouts, 2 );
	InitGraphicsPipeline< vertex_3d_t >( aPipeline, aPipelineLayout, aUniformLayout, "materials/shaders/3dvert.spv",
							 "materials/shaders/3dfrag.spv", 0 );
}
/* Binds the model data to the GPU to be rendered later.  */
void ModelData::Bind( VkCommandBuffer c, uint32_t i )
{
	if ( aTextureCount ) aNoDraw = false;
	if ( !aNoDraw )
	{
		VkBuffer 	vBuffers[  ] 	= { aVertexBuffer };
		VkDeviceSize 	offsets[  ] 	= { 0 };
		vkCmdBindPipeline( c, VK_PIPELINE_BIND_POINT_GRAPHICS, aPipeline );
		vkCmdBindVertexBuffers( c, 0, 1, vBuffers, offsets );
		vkCmdBindIndexBuffer( c, aIndexBuffer, 0, VK_INDEX_TYPE_UINT32 );
	}
}
/* Draws the model to the screen.  */
void ModelData::Draw( VkCommandBuffer c, uint32_t i )
{
	if ( !aNoDraw )
	{
		for ( auto&& index : gOffsets )
		{
			if ( gTextures[ index.aMaterialId ]->aSets.IsValid(  ) )
			{
				VkDescriptorSet sets[  ] = { gTextures[ index.aMaterialId ]->aSets.GetBuffer(  )[ i ], aUniformData.aSets.GetBuffer(  )[ i ] };
				printf( "Binding descriptor set: 0x%lX, and 0x%lX\n", sets[ 0 ], sets[ 1 ] );
				vkCmdBindDescriptorSets( c, VK_PIPELINE_BIND_POINT_GRAPHICS, aPipelineLayout, 0, 2, sets, 0, NULL );
				vkCmdDrawIndexed( c, index.aIndexCount, 1, index.aOffset, 0, 0 );
			}
		}
	}
}
/* Adds a material to the model.  */
void ModelData::AddMaterial( const std::string &srTexturePath, uint32_t sMaterialId, VkSampler sSampler )
{
	if ( srTexturePath == "" )
	{
		gTextures.push_back( InitTexture( "materials/act_like_a_baka.jpg", aTextureLayout, *gpPool, sSampler ) );
		++aTextureCount;
		return;
	}
	gTextures.push_back( InitTexture( srTexturePath, aTextureLayout, *gpPool, sSampler ) );
	gTextures[ aTextureCount ]->aMaterialId = sMaterialId;
	++aTextureCount;
}
/* Adds an index group to the model which groups together indices in the same material to be used for multiple draw calls for multiple textures.  */
void ModelData::AddIndexGroup( std::vector< uint32_t > sVec )
{
	uint32_t search;
	uint32_t numIndices;
	for ( uint32_t i = 0; i < sVec.size(  ); )
	{
		search  = sVec[ i ];
		for ( numIndices = 0; i < sVec.size(  ) && search == sVec[ i ]; ++i, ++numIndices );
		gOffsets.push_back( { numIndices, i - numIndices, sVec[ i - 1 ] } );
	}
}
/* Default the model and set limits.  */
ModelData::ModelData(  ) : apTextures( NULL ), aTextureCount( 0 ), apVertices( NULL ), apIndices( NULL ), aNoDraw( false )
{

}
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
