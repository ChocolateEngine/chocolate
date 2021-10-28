/*
material.h ( Authored by Demez )


*/
#pragma once

#include "../../types/databuffer.hh"
#include "allocator.h"
#include "shaders.h"

#include <vulkan/vulkan.hpp>
#include <filesystem>

class Renderer;
class ModelData;


// basically tied to obj files right now
// 152 bytes
class Material
{
public:
	                    Material() {}
	virtual            ~Material() {}

	void                Init();
	void                Destroy();

	std::string                 aName;

	// TODO: remove these parameters, maybe do that "MaterialVar" thing source has
	// as shaders would all use different paramters
	std::filesystem::path       aDiffusePath;
	std::filesystem::path       aNormalPath;
	// std::filesystem::path       aEmissionPath;

	TextureDescriptor*          apDiffuse;
	//TextureDescriptor*          apNormal;
	//TextureDescriptor*          apEmission;
	
	// TODO: make a "MaterialInternal" like class when the client is able to create materials
	VkDescriptorSetLayout       apTextureLayout = nullptr;

	BaseShader*                 apShader = nullptr;

	inline VkDescriptorSetLayout       GetTextureLayout() const   { return apTextureLayout; }
};

