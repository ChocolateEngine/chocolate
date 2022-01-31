/*
textureloader_ktx.h ( Authored by Demez )

Load KTX Textures
*/

#pragma once

#include "types/databuffer.hh"
#include "graphics/imaterialsystem.h"
#include "../shaders/shaders.h"
#include "../allocator.h"
#include "../types/material.h"
#include "itextureloader.h"


class KTXTextureLoader: public ITextureLoader
{
public:

	KTXTextureLoader();
	~KTXTextureLoader() override;

	// void                        Init() override;

	bool                        CheckExt( const char* ext ) override;

	// ideally this shouldn't need a path, should really be reworked to be closer to thermite, more flexible design
	// also this probably shouldn't use a shader for a parameter
	TextureDescriptor*          LoadTexture( const std::string path ) override;

	bool                        LoadKTXTexture( TextureDescriptor* texture, const String &srImagePath, VkImage &srTImage, VkDeviceMemory &srTImageMem, uint32_t &srMipLevels );
};


