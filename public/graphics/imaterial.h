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



class IMaterial
{
public:
	virtual                    ~IMaterial() = default;

	std::string                 aName;

	/* Set the shader for the material by the shader name */
	virtual void                SetShader( const char* name ) = 0;

	// eh
	virtual std::string         GetShaderName(  ) = 0;

	// TODO: make an IMaterialVar class after this refactor is done,
	//  cause this is NOT flexible for different shaders at all
	virtual void                SetDiffusePath( const std::filesystem::path& path ) = 0;
	virtual void                SetDiffuse( TextureDescriptor* texture ) = 0;
};

