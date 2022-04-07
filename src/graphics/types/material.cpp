/*
material.cpp ( Authored by Demez )


*/
#include "../renderer.h"
#include "../shaders/shaders.h"
#include "material.h"

extern MaterialSystem* matsys;

// =========================================================


void Material::Init()
{
        
}


void Material::Destroy()
{
	
}


bool Material::SetShader( const char* name )
{
	return (apShader = matsys->GetShader( name ));
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
// void Material::SetVar( const std::string& name, const glm::vec3 &data )   { SET_VAR( SetVec3 );    }
void Material::SetVar( const std::string& name, const glm::vec4 &data )   { SET_VAR( SetVec4 );    }


void Material::SetVar( const std::string& name, const glm::vec3 &data )
{
	gui->DebugMessage( "Material SetVar - Vec3: %s", Vec2Str( data ).c_str() );
	SET_VAR( SetVec3 );
}

#undef SET_VAR

#define GET_VAR( func ) \
	MaterialVar *var = GetVar( name ); \
	if ( var == nullptr ) \
		return fallback; \
	return var->func( fallback )

Texture*    Material::GetTexture( const std::string& name, Texture *fallback )    { GET_VAR( GetTexture ); }
float       Material::GetFloat(   const std::string& name, float fallback )       { GET_VAR( GetFloat );   }
int         Material::GetInt(     const std::string& name, int fallback )         { GET_VAR( GetInt );     }
glm::vec2   Material::GetVec2(    const std::string& name, glm::vec2 fallback )   { GET_VAR( GetVec2 );    }
glm::vec3   Material::GetVec3(    const std::string& name, glm::vec3 fallback )   { GET_VAR( GetVec3 );    }
glm::vec4   Material::GetVec4(    const std::string& name, glm::vec4 fallback )   { GET_VAR( GetVec4 );    }

#undef GET_VAR


int Material::GetTextureId( const std::string& name, Texture *fallback )
{
	return matsys->GetTextureId( GetTexture( name, fallback ) );
}

