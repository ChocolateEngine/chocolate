// material.cpp ( Authored by Demez )

#include "material.h"
#include "graphics.h"
#include "shadersystem.h"


// =========================================================


void Material::Init()
{
        
}


void Material::Destroy()
{
	
}


bool Material::SetShader( std::string_view name )
{
	aShader = GetShaderSystem().GetShader( name );
	return (aShader != InvalidHandle);
}


MaterialVar* Material::GetVar( const std::string& name )
{
	for ( auto var : aVars )
	{
		if ( var->aName == name )
			return var;
	}

	return nullptr;
}


MaterialVar* Material::GetVar( size_t sIndex )
{
	Assert( sIndex < aVars.size() );

	if ( sIndex >= aVars.size() )
	{
		LogError( "Material::GetVar: Index out of range\n" );
		return nullptr;
	}
	
	return aVars[sIndex];
}


size_t Material::GetVarCount()
{
	return aVars.size();
}


template <typename T>
MaterialVar* Material::CreateVar( const std::string& name, T data )
{
	MaterialVar *var = new MaterialVar( name, data );
	aVars.push_back( var );
	return var;
}


// below is some compact stuff

#define SET_VAR( func ) \
	for ( auto var : aVars ) { \
		if ( var->aName == name ) { \
			var->func( data ); \
			return; \
		} \
	} \
	CreateVar( name, data )


void Material::SetVar( const std::string& name, Texture *data )           { SET_VAR( SetTexture ); }
void Material::SetVar( const std::string& name, float data )              { SET_VAR( SetFloat );   }
void Material::SetVar( const std::string& name, int data )                { SET_VAR( SetInt );     }
void Material::SetVar( const std::string& name, const glm::vec2 &data )   { SET_VAR( SetVec2 );    }
void Material::SetVar( const std::string& name, const glm::vec3 &data )   { SET_VAR( SetVec3 );    }
void Material::SetVar( const std::string& name, const glm::vec4 &data )   { SET_VAR( SetVec4 );    }

#undef SET_VAR


#define GET_VAR( func, fallback ) \
	MaterialVar *var = GetVar( name ); \
	if ( var == nullptr ) \
		return fallback; \
	return var->func( fallback )

const int       gFallbackInt{};
const float     gFallbackFloat{};
const glm::vec2 gFallbackVec2{};
const glm::vec3 gFallbackVec3{};
const glm::vec4 gFallbackVec4{};

Texture*           Material::GetTexture( const std::string& name, Texture *fallback )    { GET_VAR( GetTexture, fallback ); }

// make const ref?
const float&       Material::GetFloat(   const std::string& name, const float& fallback )       { GET_VAR( GetFloat, fallback );   }
const int&         Material::GetInt(     const std::string& name, const int& fallback )         { GET_VAR( GetInt, fallback );     }

const glm::vec2&   Material::GetVec2(    const std::string& name, const glm::vec2& fallback )   { GET_VAR( GetVec2, fallback );    }
const glm::vec3&   Material::GetVec3(    const std::string& name, const glm::vec3& fallback )   { GET_VAR( GetVec3, fallback );    }
const glm::vec4&   Material::GetVec4(    const std::string& name, const glm::vec4& fallback )   { GET_VAR( GetVec4, fallback );    }


// const float&       Material::GetFloat(   const std::string& name )   { GET_VAR( GetFloat, gFallbackInt );    }
// const int&         Material::GetInt(     const std::string& name )   { GET_VAR( GetInt, gFallbackFloat );    }

const glm::vec2&   Material::GetVec2(    const std::string& name )   { GET_VAR( GetVec2, gFallbackVec2 );    }
const glm::vec3&   Material::GetVec3(    const std::string& name )   { GET_VAR( GetVec3, gFallbackVec3 );    }
const glm::vec4&   Material::GetVec4(    const std::string& name )   { GET_VAR( GetVec4, gFallbackVec4 );    }

#undef GET_VAR


size_t Material::GetTextureId( const std::string& name, Texture *fallback )
{
	if ( Texture* tex = GetTexture( name, fallback ) )
		return tex->aId;

	return matsys->GetMissingTexture()->aId;
}


VertexFormat Material::GetVertexFormat()
{
	return (apShader) ? apShader->GetVertexFormat() : VertexFormat_None;
}

