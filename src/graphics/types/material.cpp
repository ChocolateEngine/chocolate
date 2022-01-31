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
        
}


void Material::Destroy()
{
	
}


bool Material::SetShader( const char* name )
{
	return (apShader = materialsystem->GetShader( name ));
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


MaterialVar *Material::AddVar( MaterialVar *var )
{
	aVars.push_back( var );
	return var;
}


template <typename T>
MaterialVar* Material::AddVarInternal( const std::string &name, const std::string &raw, T data )
{
	MaterialVar *var = new MaterialVar( name, raw, data );
	aVars.push_back( var );
	return var;
}

MaterialVar *Material::AddVar( const std::string &name, const std::string &raw, Texture *data )
{
	return AddVarInternal( name, raw, data );
}

MaterialVar *Material::AddVar( const std::string &name, const std::string &raw, float data )
{
	return AddVarInternal( name, raw, data );
}

MaterialVar *Material::AddVar( const std::string &name, const std::string &raw, int data )
{
	return AddVarInternal( name, raw, data );
}

MaterialVar *Material::AddVar( const std::string &name, const std::string &raw, const glm::vec2 &data )
{
	return AddVarInternal( name, raw, data );
}

MaterialVar *Material::AddVar( const std::string &name, const std::string &raw, const glm::vec3 &data )
{
	return AddVarInternal( name, raw, data );
}

MaterialVar *Material::AddVar( const std::string &name, const std::string &raw, const glm::vec4 &data )
{
	return AddVarInternal( name, raw, data );
}


#define GET_VAR( name ) \
	MaterialVar *var = GetVar( name ); \
	if ( var == nullptr ) \
		return fallback


Texture *Material::GetTexture( const std::string &name, Texture *fallback )
{
	GET_VAR( name );
	return var->GetTexture( fallback );
}

float Material::GetFloat( const std::string &name, float fallback )
{
	GET_VAR( name );
	return var->GetFloat( fallback );
}

int Material::GetInt( const std::string &name, int fallback )
{
	GET_VAR( name );
	return var->GetInt( fallback );
}

glm::vec2 Material::GetVec2( const std::string &name, glm::vec2 fallback )
{
	GET_VAR( name );
	return var->GetVec2( fallback );
}

glm::vec3 Material::GetVec3( const std::string &name, glm::vec3 fallback )
{
	GET_VAR( name );
	return var->GetVec3( fallback );
}

glm::vec4 Material::GetVec4( const std::string &name, glm::vec4 fallback )
{
	GET_VAR( name );
	return var->GetVec4( fallback );
}

#undef GET_VAR

