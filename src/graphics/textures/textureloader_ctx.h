/*
 *	textureloader_ctx.h ( Authored by p0lyh3dron )
 *
 *	Loader for .ctx files.
 */
#pragma once

#include "types/databuffer.hh"
#include "graphics/imaterialsystem.h"
#include "../shaders/shaders.h"
#include "../allocator.h"
#include "../types/material.h"
#include "itextureloader.h"

class CTXTextureLoader : public ITextureLoader
{
public:
	/* Checks for .ctx extension.  */
	bool                        CheckExt( const char* ext ) override;
	/* Loads the texture into vulkan objects.  */
	TextureDescriptor*          LoadTexture( IMaterial* material, const std::string path ) override;

	explicit                    CTXTextureLoader();
	                           ~CTXTextureLoader() override;
};
