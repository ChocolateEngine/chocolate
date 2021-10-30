/*
material.h ( Authored by Demez )


*/
#pragma once

#include "types/databuffer.hh"
#include "../allocator.h"
#include "../shaders/shaders.h"
#include "graphics/imaterial.h"

#include <vulkan/vulkan.hpp>
#include <filesystem>

class Renderer;
class ModelData;


// basically tied to obj files right now
// 152 bytes
class Material: public IMaterial
{
public:
	                    Material() {}
	virtual            ~Material() {}

	void                Init();
	void                Destroy();

	void                SetShader( const char* name ) override;

	// This REALLY SHOULD NOT BE HERE
	//virtual BaseShader*         GetShader(  ) = 0;
	std::string         GetShaderName(  ) override { return apShader->aName; }

	// TODO: make an IMaterialVar class after this refactor is done,
	//  cause this is NOT flexible for different shaders at all
	void                SetDiffusePath( const std::filesystem::path& path ) override { aDiffusePath = path; }
	void                SetDiffuse( TextureDescriptor* texture ) override { apDiffuse = texture; }

	std::string                 aName;

	// TODO: remove these parameters, maybe do that "MaterialVar" thing source has
	// as shaders would all use different paramters
	std::filesystem::path       aDiffusePath;
	std::filesystem::path       aNormalPath;
	// std::filesystem::path       aEmissionPath;

	TextureDescriptor*          apDiffuse = nullptr;
	//TextureDescriptor*          apNormal = nullptr;
	//TextureDescriptor*          apEmission = nullptr;
	
	VkDescriptorSetLayout       apTextureLayout = nullptr;

	BaseShader*                 apShader = nullptr;

	inline VkDescriptorSetLayout       GetTextureLayout() const   { return apTextureLayout; }
};

