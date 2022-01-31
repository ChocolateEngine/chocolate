/*
imaterial.h ( Authored by Demez )

Interface for the material class to be used outside graphics
*/
#pragma once

#include "types/databuffer.hh"
#include "graphics/renderertypes.h"

#include <filesystem>
#include <string>


class BaseShader;


// types of material vars needed:
//  texture
//  string
//  float
//  int
//  range (just a min and max value)?
//  RGB color/vector
//  RGBA color/vector



enum class MatVar
{
	Invalid = 0,
	Texture,
	Float,
	Int,
	Vec2,
	Vec3,
	Vec4,
};


// oh FUCK i completely overlooked this
// where am i going to store the path for the texture????
// well, i COULD do it like valve does
// i set keyvalues data in the material, and then init the material
// where it parses the material kv, and creates these material vars for you
// i guess that works? idk lol

// TODO: really should remove this std::string parameter probably, idk
class MaterialVar
{
private:
	MaterialVar( const std::string &name, MatVar type ):
		aName( name ), aType( type )
	{
	}

public:
	MaterialVar( const std::string &name, Texture *data ):
		MaterialVar( name, MatVar::Texture )
	{ aDataTexture = data; }

	MaterialVar( const std::string &name, float data ):
		MaterialVar( name, MatVar::Float )
	{ aDataFloat = data; }

	MaterialVar( const std::string &name, int data ):
		MaterialVar( name, MatVar::Int )
	{ aDataInt = data; }

	MaterialVar( const std::string &name, const glm::vec2 &data ):
		MaterialVar( name, MatVar::Vec2 )
	{ aDataVec2 = data; }

	MaterialVar( const std::string &name, const glm::vec3 &data ):
		MaterialVar( name, MatVar::Vec3 )
	{ aDataVec3 = data; }

	MaterialVar( const std::string &name, const glm::vec4 &data ):
		MaterialVar( name, MatVar::Vec4 )
	{ aDataVec4 = data; }

	std::string aName;
	MatVar      aType;
	
	union
	{
		Texture*    aDataTexture;
		float       aDataFloat;
		int         aDataInt;
		glm::vec2   aDataVec2;
		glm::vec3   aDataVec3;
		glm::vec4   aDataVec4;
	};

	inline void        SetTexture( Texture *val )                   { aType = MatVar::Texture; aDataTexture = val; }
	inline void        SetFloat( float val )                        { aType = MatVar::Float; aDataFloat = val; }
	inline void        SetInt( int val )                            { aType = MatVar::Int; aDataInt = val; }
	inline void        SetVec2( const glm::vec2 &val )              { aType = MatVar::Vec2; aDataVec2 = val; }
	inline void        SetVec3( const glm::vec3 &val )              { aType = MatVar::Vec3; aDataVec3 = val; }
	inline void        SetVec4( const glm::vec4 &val )              { aType = MatVar::Vec4; aDataVec4 = val; }

	inline Texture*    GetTexture( Texture *fallback = nullptr )	{ return (aType == MatVar::Texture) ? aDataTexture : fallback; }
	inline float       GetFloat( float fallback = 0.f )             { return (aType == MatVar::Float) ? aDataFloat : fallback; }
	inline int         GetInt( int fallback = 0 )                   { return (aType == MatVar::Int) ? aDataInt : fallback; }
	inline glm::vec2   GetVec2( const glm::vec2 &fallback )         { return (aType == MatVar::Vec2) ? aDataVec2 : fallback; }
	inline glm::vec3   GetVec3( const glm::vec3 &fallback )         { return (aType == MatVar::Vec3) ? aDataVec3 : fallback; }
	inline glm::vec4   GetVec4( const glm::vec4 &fallback )         { return (aType == MatVar::Vec4) ? aDataVec4 : fallback; }
};


class IMaterial
{
public:
	virtual                    ~IMaterial() = default;

	std::string                 aName;

	/* Set the shader for the material by the shader name */
	virtual bool                SetShader( const char* name ) = 0;

	// eh
	virtual std::string         GetShaderName(  ) = 0;

	virtual MaterialVar*        GetVar( const std::string& name ) = 0;

	virtual void                SetVar( const std::string& name, Texture* data ) = 0;
	virtual void                SetVar( const std::string& name, float data ) = 0;
	virtual void                SetVar( const std::string& name, int data ) = 0;
	virtual void                SetVar( const std::string& name, const glm::vec2 &data ) = 0;
	virtual void                SetVar( const std::string& name, const glm::vec3 &data ) = 0;
	virtual void                SetVar( const std::string& name, const glm::vec4 &data ) = 0;
	
	// Quicker Access to getting the data from the material vars stored for the shader
	// So it doesn't need to check for if MaterialVar is null and can get a quick fallback in the 2nd default parameter
	// TODO: shaders should probably have some shader data struct filled with the information from the material
	//  so it doesn't need to search multiple times every frame
	virtual Texture*            GetTexture( const std::string &name, Texture* fallback = nullptr ) = 0;
	virtual float               GetFloat( const std::string &name, float fallback = 0.f ) = 0;
	virtual int                 GetInt( const std::string &name, int fallback = 0 ) = 0;
	virtual glm::vec2           GetVec2( const std::string &name, glm::vec2 fallback = {} ) = 0;
	virtual glm::vec3           GetVec3( const std::string &name, glm::vec3 fallback = {} ) = 0;
	virtual glm::vec4           GetVec4( const std::string &name, glm::vec4 fallback = {} ) = 0;
};

