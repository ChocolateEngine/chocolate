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
	for ( int i = 0; i < aModules.GetSize(); i++ )
		vkDestroyShaderModule( DEVICE, aModules[i], nullptr );

	vkDestroyPipeline( DEVICE, aPipeline, NULL );
	vkDestroyPipelineLayout( DEVICE, aPipelineLayout, NULL );
}

void BaseShader::CreateDescriptorSetLayout(  )
{
	auto bindings = GetDescriptorSetLayoutBindings(  );

	aLayouts.Allocate( bindings.size() );

	for ( int i = 0; i < bindings.size(); i++ )
		aLayouts[i] = InitDescriptorSetLayout( {bindings[i]} );  // stupid vector input
}
