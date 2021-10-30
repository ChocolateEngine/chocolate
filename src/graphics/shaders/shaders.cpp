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
}

void BaseShader::ReInit()
{
	//InitUniformData( aUniformData, aUniformLayout );
}

void BaseShader::Destroy()
{
	gPipelineBuilder.RemovePipeline( &aPipeline );

	vkDestroyPipeline( DEVICE, aPipeline, NULL );
	vkDestroyPipelineLayout( DEVICE, aPipelineLayout, NULL );
}

// =========================================================

void Basic3D::Init()
{
	BaseShader::Init();

	aModules.Allocate(2);
	aModules[0] = CreateShaderModule( ReadFile( pVShader ) );
	aModules[1] = CreateShaderModule( ReadFile( pFShader ) );
	aSetLayouts.Allocate(2);

	gLayoutBuilder.Queue( &aSetLayouts[0], { { DescriptorLayoutBinding( VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1,
									     VK_SHADER_STAGE_FRAGMENT_BIT, NULL ) } } );

	gLayoutBuilder.Queue( &aSetLayouts[1], { { DescriptorLayoutBinding( VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1,
									    VK_SHADER_STAGE_VERTEX_BIT, NULL ) } } );

	gPipelineBuilder.Queue( &aPipeline, &aPipelineLayout, &aSetLayouts, pVShader, pFShader, 0 );
}


void Basic3D::ReInit()
{
	BaseShader::ReInit();

	gLayoutBuilder.Queue( &aSetLayouts[0], { { DescriptorLayoutBinding( VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1,
									     VK_SHADER_STAGE_FRAGMENT_BIT, NULL ) } } );

	gLayoutBuilder.Queue( &aSetLayouts[1], { { DescriptorLayoutBinding( VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1,
									    VK_SHADER_STAGE_VERTEX_BIT, NULL ) } } );

	gPipelineBuilder.Queue( &aPipeline, &aPipelineLayout, &aSetLayouts, pVShader, pFShader, 0 );
}

// void Basic3D::UpdateUniformBuffers( uint32_t sCurrentImage, ModelData &srModelData, Mesh* srMesh )
void Basic3D::UpdateUniformBuffers( uint32_t sCurrentImage, BaseRenderable* spRenderable )
{
	ubo_3d_t ubo{  };

	ubo.model = spRenderable->GetModelMatrix(  );
	ubo.view  = apRenderer->aView.viewMatrix;
	ubo.proj  = apRenderer->aView.GetProjection(  );

	void* data;
	vkMapMemory( DEVICE, apMaterialSystem->GetUniformData( spRenderable->GetID() ).aMem[ sCurrentImage ], 0, sizeof( ubo ), 0, &data );
	memcpy( data, &ubo, sizeof( ubo ) );
	vkUnmapMemory( DEVICE, apMaterialSystem->GetUniformData( spRenderable->GetID() ).aMem[ sCurrentImage ] );
}

