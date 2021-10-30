/*
material.cpp ( Authored by Demez )


*/
#include "../renderer.h"
#include "../shaders/shaders.h"
#include "material.h"

extern MaterialSystem* materialsystem;

// =========================================================


void Material::Init()
{
	apTextureLayout = InitDescriptorSetLayout( { { DescriptorLayoutBinding( VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, NULL ) } } );
}


void Material::Destroy()
{
	vkDestroyDescriptorSetLayout( DEVICE, apTextureLayout, NULL );
}


void Material::SetShader( const char* name )
{
	apShader = materialsystem->GetShader( name );
}
