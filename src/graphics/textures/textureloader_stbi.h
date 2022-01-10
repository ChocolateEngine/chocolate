/*
textureloader_stbi.h ( Authored by Demez )

Load Normal Images with stb_image (blech)
*/

#pragma once

#include "types/databuffer.hh"
#include "graphics/imaterialsystem.h"
#include "../shaders/shaders.h"
#include "../allocator.h"
#include "../types/material.h"
#include "itextureloader.h"


class STBITextureLoader: public ITextureLoader
{
public:

	STBITextureLoader();
	~STBITextureLoader() override;

	// void                        Init() override;

	bool                        CheckExt( const char* ext ) override;

	// ideally this shouldn't need a path, should really be reworked to be closer to thermite, more flexible design
	// also this probably shouldn't use a shader for a parameter
	TextureDescriptor*          LoadTexture( IMaterial* material, const std::string path ) override;
};


