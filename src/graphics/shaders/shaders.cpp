/*
shaders.cpp ( Authored by p0lyhdron )

Defines shader effect stuff.
*/
#include "../renderer.h"
#include "shaders.h"

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


void BaseShader::CreateLayouts(  )
{
	aLayouts.Allocate( 1 );
	aLayouts[0] = matsys->aImageLayout;
}


void BaseShader::BindBuffers( BaseRenderable* renderable, VkCommandBuffer c, uint32_t cIndex )
{
	const std::vector< RenderableBuffer* >& renderBuffers = matsys->GetRenderBuffers( renderable );

	// Bind the mesh's vertex and index buffers
	for ( RenderableBuffer* buffer : renderBuffers )
	{
		if ( buffer->aFlags & gVertexBufferFlags )
		{
			VkBuffer        vBuffers[]  = { buffer->aBuffer };
			VkDeviceSize	offsets[]   = { 0 };

			vkCmdBindVertexBuffers( c, 0, 1, vBuffers, offsets );
		}
		else if ( buffer->aFlags & gIndexBufferFlags )
		{
			vkCmdBindIndexBuffer( c, buffer->aBuffer, 0, VK_INDEX_TYPE_UINT32 );
		}
	}
}


void BaseShader::Bind( VkCommandBuffer c, uint32_t cIndex )
{
	vkCmdBindPipeline( c, VK_PIPELINE_BIND_POINT_GRAPHICS, aPipeline );
}

