#pragma once

#include "gutil.hh"

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

class Material
{
    std::vector< MaterialVar > aVars;
public:
     Material();
    ~Material();

    void SetShader( const std::string &srName );
};