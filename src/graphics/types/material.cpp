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


MaterialVar* Material::GetVar( const std::string &name )
{
	for ( auto var : aVars )
	{
		if ( var->aName == name )
			return var;
	}

	return nullptr;
}


MaterialVar* Material::AddVar( const std::string &name, MatVar type, void *data )
{
	MaterialVar *var = new MaterialVar( name, type, "", data);
	aVars.push_back( var );
	return var;
}

MaterialVar* Material::AddVar( const std::string &name, MatVar type, const std::string &raw, void *data )
{
	MaterialVar *var = new MaterialVar( name, type, raw, data );
	aVars.push_back( var );
	return var;
}


MaterialVar* Material::AddVar( MaterialVar* var )
{
	aVars.push_back( var );
	return var;
}


// not used for textures - ALSO UNTESTED
template <typename T>
T Material::GetVarData( const std::string &name, T fallback )
{
	MaterialVar *var = GetVar( name );
	if ( var == nullptr )
		return fallback;

	#define RETURN_CHECK() return (var->apData != nullptr) ? *(T *)&var->apData : fallback

	switch ( var->aType )
	{
		// this (probably) can't do textures
		case MatVar::Texture:
			return fallback;

		case MatVar::Float:
		case MatVar::Int:
		case MatVar::Vec2:
		case MatVar::Vec3:
		case MatVar::Vec4:
			RETURN_CHECK();

		default:
			Print( "[Graphics] Unknown Material Type on Material \"%s\": %d", var->aName, var->aType );
			return fallback;
	}

	#undef RETURN_CHECK

	return fallback;
}


Texture *Material::GetTexture( const std::string &name, Texture *fallback )
{
	MaterialVar *var = GetVar( name );
	if ( var == nullptr )
		return fallback;

	return (var->apData != nullptr) ? var->GetTexture() : fallback;
}

float Material::GetFloat( const std::string &name, float fallback )
{
	return GetVarData<float>( name, fallback );
}

int Material::GetInt( const std::string &name, int fallback )
{
	return GetVarData<int>( name, fallback );
}

glm::vec2 Material::GetVec2( const std::string &name, glm::vec2 fallback )
{
	return GetVarData<glm::vec2>( name, fallback );
}

glm::vec3 Material::GetVec3( const std::string &name, glm::vec3 fallback )
{
	return GetVarData<glm::vec3>( name, fallback );
}

glm::vec4 Material::GetVec4( const std::string &name, glm::vec4 fallback )
{
	return GetVarData<glm::vec4>( name, fallback );
}


