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
	CreateDescriptorSetLayout(  );
	CreateGraphicsPipeline(  );
}

void BaseShader::ReInit()
{
	CreateDescriptorSetLayout(  );
	CreateGraphicsPipeline(  );
}

void BaseShader::Destroy()
{
	for ( uint32_t i = 0; i < aModules.GetSize(); i++ )
		vkDestroyShaderModule( DEVICE, aModules[i], nullptr );

	vkDestroyPipeline( DEVICE, aPipeline, NULL );
	vkDestroyPipelineLayout( DEVICE, aPipelineLayout, NULL );
}

void BaseShader::CreateDescriptorSetLayout(  )
{
	auto bindings = GetDescriptorSetLayoutBindings(  );

	aLayouts.Allocate( 1 );

	aLayouts[0] = matsys->aImageLayout;
}
