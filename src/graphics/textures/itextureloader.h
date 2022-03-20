/*
itextureloader.h ( Authored by Demez )

Interface for loading textures in the engine
*/

#pragma once

#include "types/databuffer.hh"
#include "../allocator.h"
#include "../types/material.h"


class ITextureLoader;
extern void AddTextureLoader( ITextureLoader* loader );


// most likely going to have features added to it when needed
class ITextureLoader
{
public:

	ITextureLoader()
	{
		Init();
	}

	virtual ~ITextureLoader() = default;

	virtual inline void Init()
	{
		AddTextureLoader( this ); 
	}
	
	virtual bool                        CheckExt( const char* ext ) = 0;

	// ideally this shouldn't need a path, should really be reworked to be closer to thermite, more flexible design
	// also this probably shouldn't use a shader for a parameter
	virtual TextureDescriptor*          LoadTexture( const std::string path ) = 0;

	// will do later lol
	// virtual void                        FreeTexture( TextureDescriptor* texture ) = 0;
};

