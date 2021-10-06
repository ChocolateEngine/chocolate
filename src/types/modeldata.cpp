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
#if MESH_UNIFORM_DATA
	aUniformLayout = InitDescriptorSetLayout( { { DescriptorLayoutBinding( VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT, NULL ) } } );
	InitUniformData( aUniformData, aUniformLayout );
#endif
}

void Mesh::ReInit()
{
#if MESH_UNIFORM_DATA
	InitUniformData( aUniformData, aUniformLayout );
#endif
}

// =============================================================

void ModelData::Init()
{
#if MESH_UNIFORM_DATA
	for ( uint32_t j = 0; j < aMeshes.GetSize(  ); ++j )
		aMeshes[j].Init();
#else
	// TEMP
	aUniformLayout = InitDescriptorSetLayout( { { DescriptorLayoutBinding( VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT, NULL ) } } );
	InitUniformData( aUniformData, aUniformLayout );
#endif
}

void ModelData::ReInit()
{
#if MESH_UNIFORM_DATA
	for ( uint32_t j = 0; j < aMeshes.GetSize(  ); ++j )
		aMeshes[j].ReInit();
#else
	// TEMP
	InitUniformData( aUniformData, aUniformLayout );
#endif
}

/* Binds the model data to the GPU to be rendered later.  */
void ModelData::Bind( VkCommandBuffer c, uint32_t i )
{
	if ( aMeshes.GetSize(  ) ) aNoDraw = false;
}

/* Draws the model to the screen.  */
void ModelData::Draw( VkCommandBuffer c, uint32_t i )
{
	if ( aNoDraw || !aMeshes.IsValid() )
		return;

	for ( uint32_t j = 0; j < aMeshes.GetSize(  ); ++j )
	{
		Mesh& mesh = aMeshes[ j ];

		// Bind the mesh's vertex and index buffers
		VkBuffer 	vBuffers[  ] 	= { mesh.aVertexBuffer };
		VkDeviceSize 	offsets[  ] 	= { 0 };
		vkCmdBindVertexBuffers( c, 0, 1, vBuffers, offsets );
		vkCmdBindIndexBuffer( c, mesh.aIndexBuffer, 0, VK_INDEX_TYPE_UINT32 );

		// Now draw it
		vkCmdBindPipeline( c, VK_PIPELINE_BIND_POINT_GRAPHICS, mesh.GetShader()->GetPipeline(  ) );

#if MESH_UNIFORM_DATA
		VkDescriptorSet sets[  ] = { mesh.apMaterial->apDiffuse->aSets[ i ], mesh.GetUniformData().aSets[ i ] };
#else
		VkDescriptorSet sets[  ] = { mesh.apMaterial->apDiffuse->aSets[ i ], aUniformData.aSets[ i ] };
#endif

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
	for ( uint32_t mesh = 0; mesh < aMeshes.GetSize(  ); ++mesh )
	{
		for ( uint32_t i = 0; i < PSWAPCHAIN.GetImages(  ).size(  ); ++i )
		{
			/* Free uniform data.  */
			UniformDescriptor& uniformdata = GetUniformData(mesh);
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

	for ( uint32_t i = 0; i < aMeshes.GetSize(); i++ )
	{
		Mesh& mesh = aMeshes[i];
		vkDestroyBuffer( DEVICE, mesh.aVertexBuffer, NULL );
		vkFreeMemory( DEVICE, mesh.aVertexBufferMem, NULL );
		vkDestroyBuffer( DEVICE, mesh.aIndexBuffer, NULL );
		vkFreeMemory( DEVICE, mesh.aIndexBufferMem, NULL );
	}

	delete[  ] apVertices;
	delete[  ] apIndices;
}
