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
public:
	MaterialVar( const std::string &name, MatVar type, const std::string &raw, void *data ):
		aName( name ), aType( type ), aRaw(raw), apData( data )
	{

	}

	std::string aName;
	MatVar      aType;
	std::string aRaw;
	void       *apData;

	inline Texture*    GetTexture()		{ return (apData != nullptr) ? (Texture *)apData : nullptr; }
	inline float       GetFloat()       { return (apData != nullptr) ? *(float *)&apData : 0.f; }
	inline int         GetInt()         { return (apData != nullptr) ? *(int *)&apData : 0; }
	inline glm::vec2   GetVec2()        { return (apData != nullptr) ? *(glm::vec2 *)&apData : glm::vec2(); }
	inline glm::vec3   GetVec3()        { return (apData != nullptr) ? *(glm::vec3 *)&apData : glm::vec3(); }
	inline glm::vec4   GetVec4()        { return (apData != nullptr) ? *(glm::vec4 *)&apData : glm::vec4(); }
};


class IMaterial
{
public:
	virtual                    ~IMaterial() = default;

	std::string                 aName;

	/* Set the shader for the material by the shader name */
	virtual void                SetShader( const char* name ) = 0;

	// eh
	virtual std::string         GetShaderName(  ) = 0;

	virtual MaterialVar*        GetVar( const std::string& name ) = 0;
	virtual MaterialVar*        AddVar( const std::string& name, MatVar type, void* data ) = 0;
	virtual MaterialVar*        AddVar( const std::string& name, MatVar type, const std::string &raw, void* data ) = 0;
	virtual MaterialVar*        AddVar( MaterialVar* var ) = 0;
	
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

