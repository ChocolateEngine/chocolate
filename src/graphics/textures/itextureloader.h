/*
itextureloader.h ( Authored by Demez )

Interface for loading textures in the engine
*/

#pragma once

#include "types/databuffer.hh"
#include "graphics/imaterialsystem.h"
#include "../allocator.h"
#include "../types/material.h"


// most likely going to have features added to it when needed
class ITextureLoader
{
public:

	virtual ~ITextureLoader() = default;

	// virtual void                        Init() = 0;
	
	virtual bool                        CheckExt( const char* ext ) = 0;

	// ideally this shouldn't need a path, should really be reworked to be closer to thermite, more flexible design
	// also this probably shouldn't use a shader for a parameter
	virtual TextureDescriptor*          LoadTexture( const std::string path ) = 0;

	// will do later lol
	// virtual void                        FreeTexture( TextureDescriptor* texture ) = 0;
};

