/*
spritedata.cpp ( Authored by p0lyh3dron )

Defines the SpriteData class defined in
spritedata.h.  
*/
#include "../../inc/types/spritedata.h"
/* Allocates the model, and loads its textures etc.  */
void SpriteData::Init(  )
{
	aTextureLayout = InitDescriptorSetLayout( { { DescriptorLayoutBinding( VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1,
											   VK_SHADER_STAGE_FRAGMENT_BIT, NULL ) } } );
	
	VkDescriptorSetLayout layouts[  ] = { aTextureLayout };
	aPipelineLayout = InitPipelineLayouts( layouts, 1 );
	aPipeline = InitGraphicsPipeline< vertex_2d_t >( aPipelineLayout, "materials/shaders/2dvert.spv", "materials/shaders/2dfrag.spv", NO_CULLING | NO_DEPTH );
}
/* Reinitialize data that is useless after the swapchain becomes outdated.  */
void SpriteData::Reinit(  )
{
    	aTextureLayout = InitDescriptorSetLayout( { { DescriptorLayoutBinding( VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1,
											   VK_SHADER_STAGE_FRAGMENT_BIT, NULL ) } } );
	
	VkDescriptorSetLayout layouts[  ] = { aTextureLayout };
	aPipelineLayout = InitPipelineLayouts( layouts, 1 );
	aPipeline = InitGraphicsPipeline< vertex_2d_t >( aPipelineLayout, "materials/shaders/2dvert.spv", "materials/shaders/2dfrag.spv", NO_CULLING | NO_DEPTH );
}
/* Binds the model data to the GPU to be rendered later.  */
void SpriteData::Bind( VkCommandBuffer c, uint32_t i )
{
	if ( !aNoDraw )
	{
		VkBuffer 	vBuffers[  ] 	= { aVertexBuffer };
		VkDeviceSize 	offsets[  ] 	= { 0 };
		VkDescriptorSet sets[  ] = { apTexture->aSets.GetBuffer(  )[ i ] };
		vkCmdBindPipeline( c, VK_PIPELINE_BIND_POINT_GRAPHICS, aPipeline );
		vkCmdBindVertexBuffers( c, 0, 1, vBuffers, offsets );
		vkCmdBindIndexBuffer( c, aIndexBuffer, 0, VK_INDEX_TYPE_UINT32 );
		vkCmdBindDescriptorSets( c, VK_PIPELINE_BIND_POINT_GRAPHICS, aPipelineLayout, 0, 1, sets, 0, NULL );
	}
}
/* Draws the model to the screen.  */
void SpriteData::Draw( VkCommandBuffer c )
{
	if ( !aNoDraw )
		vkCmdDrawIndexed( c, aIndexCount, 1, 0, 0, 0 );
}
/* Adds a material to the model.  */
void SpriteData::SetTexture( const std::string &srTexturePath, VkSampler sSampler )
{
	apTexture = InitTexture( srTexturePath, aTextureLayout, *gpPool, sSampler, &aWidth, &aHeight );
}
/* Frees the memory used by objects outdated by a new swapchain state.  */
void SpriteData::FreeOldResources(  )
{
	/* Destroy Vulkan objects, keeping the allocations for their storage units.  */
	vkDestroyPipeline( DEVICE, aPipeline, NULL );
	vkDestroyPipelineLayout( DEVICE, aPipelineLayout, NULL );
}
/* Frees all memory used by model.  */
SpriteData::~SpriteData(  )
{
	FreeOldResources(  );
        vkDestroyImageView( DEVICE, apTexture->aTextureImageView, NULL );
        vkDestroyImage( DEVICE, apTexture->aTextureImage, NULL );
        vkFreeMemory( DEVICE, apTexture->aTextureImageMem, NULL );
	vkDestroyDescriptorSetLayout( DEVICE, aTextureLayout, NULL );
	vkDestroyBuffer( DEVICE, aVertexBuffer, NULL );
	vkFreeMemory( DEVICE, aVertexBufferMem, NULL );
	vkDestroyBuffer( DEVICE, aIndexBuffer, NULL );
	vkFreeMemory( DEVICE, aIndexBufferMem, NULL );
	delete[  ] apVertices;
	delete[  ] apIndices;
	delete apTexture;
}
