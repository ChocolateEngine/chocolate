/*
modeldata.cpp ( Authored by p0lyh3dron )

Defines the ModelData class defined in
modeldata.h.  
*/
#include "../../inc/types/modeldata.h"
/* Allocates the model, and loads its textures etc.  */
void ModelData::Init( Allocator &srAllocator, const std::string &srModelPath, const std::string &srTexturePath,
		      VkDescriptorSetLayout srModelSetLayout, VkSampler srTextureSampler, VkDescriptorPool srDescPool, VkExtent2D srSwapChainExtent )
{
	aNoDraw = true;
	aTextureCount = 0;
	aTextureLayout = srAllocator.InitDescriptorSetLayout( { { DescriptorLayoutBinding( VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1,
											   VK_SHADER_STAGE_FRAGMENT_BIT, NULL ) } } );
	
	aUniformLayout = srAllocator.InitDescriptorSetLayout( { { DescriptorLayoutBinding( VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1,
											   VK_SHADER_STAGE_VERTEX_BIT, NULL ) } } );
	
	srAllocator.InitUniformData( aUniformData, aUniformLayout );
	VkDescriptorSetLayout layouts[  ] = { aTextureLayout, aUniformLayout };
	aPipelineLayout = srAllocator.InitPipelineLayouts( layouts, 2 );
	srAllocator.InitGraphicsPipeline< vertex_3d_t >( aPipeline, aPipelineLayout, aUniformLayout, "materials/shaders/3dvert.spv",
							 "materials/shaders/3dfrag.spv", 0 );
	
}
/* Reinitialize data that is useless after the swapchain becomes outdated.  */
void ModelData::Reinit( Allocator &srAllocator )
{
        aTextureLayout = srAllocator.InitDescriptorSetLayout( { { DescriptorLayoutBinding( VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1,
											   VK_SHADER_STAGE_FRAGMENT_BIT, NULL ) } } );
	
	aUniformLayout = srAllocator.InitDescriptorSetLayout( { { DescriptorLayoutBinding( VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1,
											   VK_SHADER_STAGE_VERTEX_BIT, NULL ) } } );
	
	srAllocator.InitUniformData( aUniformData, aUniformLayout );
	VkDescriptorSetLayout layouts[  ] = { aTextureLayout, aUniformLayout };
	aPipelineLayout = srAllocator.InitPipelineLayouts( layouts, 2 );
	srAllocator.InitGraphicsPipeline< vertex_3d_t >( aPipeline, aPipelineLayout, aUniformLayout, "materials/shaders/3dvert.spv",
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
				vkCmdBindDescriptorSets( c, VK_PIPELINE_BIND_POINT_GRAPHICS, aPipelineLayout, 0, 2, sets, 0, NULL );
				vkCmdDrawIndexed( c, index.aIndexCount, 1, index.aOffset, 0, 0 );
			}
		}
	}
}
/* Adds a material to the model.  */
void ModelData::AddMaterial( const std::string &srTexturePath, Allocator &srAllocator, uint32_t sMaterialId, VkDescriptorPool sPool, VkSampler sSampler )
{
	if ( srTexturePath == "" )
	{
		gTextures.push_back( srAllocator.InitTexture( "materials/act_like_a_baka.jpg", aTextureLayout, sPool, sSampler ) );
		++aTextureCount;
		return;
	}
	gTextures.push_back( srAllocator.InitTexture( srTexturePath, aTextureLayout, sPool, sSampler ) );
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
	
}
/* Frees all memory used by model.  */
ModelData::~ModelData(  )
{
	
}
