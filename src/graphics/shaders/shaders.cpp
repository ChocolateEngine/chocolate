/*
shaders.cpp ( Authored by p0lyhdron )

Defines shader effect stuff.
*/
#include "../renderer.h"
#include "shaders.h"

extern size_t gModelDrawCalls;
extern size_t gVertsDrawn;

// =========================================================

BaseShader::BaseShader()
{
}

BaseShader::~BaseShader()
{
}

void BaseShader::Init()
{
	CreateLayouts();
	CreateGraphicsPipeline();
}

void BaseShader::ReInit()
{
	CreateLayouts();
	CreateGraphicsPipeline();
}

void BaseShader::Destroy()
{
	for ( uint32_t i = 0; i < aModules.GetSize(); i++ )
		vkDestroyShaderModule( DEVICE, aModules[i], nullptr );

	vkDestroyPipeline( DEVICE, aPipeline, NULL );
	vkDestroyPipelineLayout( DEVICE, aPipelineLayout, NULL );
}


// =====================================================================
// base stuff, feel free to override


void BaseShader::CreateLayouts()
{
	aLayouts.Allocate( 1 );
	aLayouts[0] = matsys->aImageLayout;
}


void BaseShader::BindBuffers( IRenderable* renderable, size_t matIndex, VkCommandBuffer c, uint32_t cIndex )
{
	// temp debugging
	const std::vector< InternalMeshData_t >& meshDataList = matsys->GetMeshData( renderable );

	const InternalMeshData_t& meshData = meshDataList[matIndex];

	// Bind the mesh's vertex and index buffers
	if ( !meshData.apVertexBuffer )
	{
		LogWarn( "Mesh Without a Vertex Buffer Trying to Draw !!!!\n" );
		return;
	}

	meshData.apVertexBuffer->Bind( c );

	if ( meshData.apIndexBuffer )
		meshData.apIndexBuffer->Bind( c );
}


void BaseShader::Bind( VkCommandBuffer c, uint32_t cIndex )
{
	vkCmdBindPipeline( c, VK_PIPELINE_BIND_POINT_GRAPHICS, aPipeline );
}


void BaseShader::CmdDraw( IRenderable* renderable, size_t matIndex, VkCommandBuffer c )
{
	// TODO: figure out a way to get back to the design below with this vertex format stuff
	// ideally, it would be less vertex buffer binding, but would be harder to pull off
#if 1
	if ( matsys->HasIndexBuffer( renderable, matIndex ) )
		// um do i put in the vertex offset here too?
		vkCmdDrawIndexed(
			c,
			renderable->GetSurfaceIndices( matIndex ).size(),
			1,
			0,  // first index
			0,  // vertex offset
			0
		);

	else
		vkCmdDraw(
			c,
			renderable->GetSurfaceVertexData( matIndex ).aCount,
			1,
			0,  // no offset
			0
		);

	gModelDrawCalls++;
	gVertsDrawn += renderable->GetSurfaceVertexData( matIndex ).aCount;

#else

	if ( matsys->HasIndexBuffer( renderable ) )
		// um do i put in the vertex offset here too?
		vkCmdDrawIndexed(
			c,
			renderable->GetIndexCount( matIndex ),
			1,
			renderable->GetIndexOffset( matIndex ),
			renderable->GetVertexOffset( matIndex ),
			0
		);

	else
		vkCmdDraw(
			c,
			renderable->GetVertexCount( matIndex ),
			1,
			renderable->GetVertexOffset( matIndex ),
			0
		);

	gModelDrawCalls++;
	gVertsDrawn += renderable->GetVertexCount( matIndex );

#endif
}

