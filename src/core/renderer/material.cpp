/*
material.cpp ( Authored by Demez )


*/
#include "../../../inc/core/renderer/renderer.h"
#include "../../../inc/core/renderer/shaders.h"
#include "../../../inc/core/renderer/material.h"

// =========================================================


void Material::Init()
{
	apTextureLayout = InitDescriptorSetLayout( { { DescriptorLayoutBinding( VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, NULL ) } } );
}


void Material::Destroy()
{
	vkDestroyDescriptorSetLayout( DEVICE, apTextureLayout, NULL );
}
