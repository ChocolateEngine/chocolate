/*
modeldata.cpp ( Authored by p0lyh3dron )

Defines the ModelData class defined in
modeldata.h.  
*/

#include "../../inc/types/modeldata.h"

extern size_t gModelDrawCalls;

/* Allocates the model's uniform data and layout.  */
void Mesh::Init()
{
	aUniformLayout = InitDescriptorSetLayout( { { DescriptorLayoutBinding( VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT, NULL ) } } );
	InitUniformData( aUniformData, aUniformLayout );
}

void Mesh::ReInit()
{
	aUniformLayout = InitDescriptorSetLayout( { { DescriptorLayoutBinding( VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT, NULL ) } } );
	InitUniformData( aUniformData, aUniformLayout );
}

// =============================================================

void ModelData::Init()
{
	for ( uint32_t j = 0; j < aMeshes.size(  ); ++j )
		aMeshes[j]->Init();
}

void ModelData::ReInit()
{
	for ( uint32_t j = 0; j < aMeshes.size(  ); ++j )
		aMeshes[j]->ReInit();
}

/* Bind and Draws the model to the screen.  */
void ModelData::Draw( VkCommandBuffer c, uint32_t i )
{
	if ( aNoDraw || aMeshes.empty() )
		return;

	for ( uint32_t j = 0; j < aMeshes.size(  ); ++j )
	{
		Mesh& mesh = *aMeshes[ j ];

		// Bind the mesh's vertex and index buffers
		VkBuffer 	vBuffers[  ] 	= { mesh.aVertexBuffer };
		VkDeviceSize 	offsets[  ] 	= { 0 };
		vkCmdBindVertexBuffers( c, 0, 1, vBuffers, offsets );
		vkCmdBindIndexBuffer( c, mesh.aIndexBuffer, 0, VK_INDEX_TYPE_UINT32 );

		// Now draw it
		vkCmdBindPipeline( c, VK_PIPELINE_BIND_POINT_GRAPHICS, mesh.GetShader()->GetPipeline(  ) );
		VkDescriptorSet sets[  ] = { mesh.apMaterial->apDiffuse->aSets[ i ], mesh.GetUniformData().aSets[ i ] };
		vkCmdBindDescriptorSets( c, VK_PIPELINE_BIND_POINT_GRAPHICS, mesh.GetShader()->GetPipelineLayout(  ), 0, 2, sets, 0, NULL );

		vkCmdDrawIndexed( c, (uint32_t)mesh.aIndices.size(), 1, 0, 0, 0 );

		gModelDrawCalls++;
	}
}

/* Default the model and set limits.  */
/* Frees the memory used by objects outdated by a new swapchain state.  */
void ModelData::FreeOldResources(  )
{
	/* Destroy Vulkan objects, keeping the allocations for their storage units.  */

	/* Ugly, fix later.  */
	for ( uint32_t mesh = 0; mesh < aMeshes.size(  ); ++mesh )
	{
		UniformDescriptor& uniformdata = GetUniformData(mesh);

		for ( uint32_t i = 0; i < uniformdata.aSets.GetSize(  ); ++i )
		{
			/* Free uniform data.  */
			vkDestroyBuffer( DEVICE, uniformdata.aData[i], NULL );
			vkFreeMemory( DEVICE, uniformdata.aMem[i], NULL );

			/* Free uniform layout. */
			vkDestroyDescriptorSetLayout( DEVICE, GetUniformLayout(mesh), NULL );
		}
	}
}

/* Frees all memory used by model.  */
ModelData::~ModelData(  )
{
	FreeOldResources(  );

	for ( uint32_t i = 0; i < aMeshes.size(); i++ )
	{
		Mesh& mesh = *aMeshes[i];
		vkDestroyBuffer( DEVICE, mesh.aVertexBuffer, NULL );
		vkFreeMemory( DEVICE, mesh.aVertexBufferMem, NULL );
		vkDestroyBuffer( DEVICE, mesh.aIndexBuffer, NULL );
		vkFreeMemory( DEVICE, mesh.aIndexBufferMem, NULL );
	}
}
