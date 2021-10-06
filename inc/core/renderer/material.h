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
class Material
{
public:
	                    Material(  ) {}
	virtual            ~Material(  ) {}

	//virtual void        Init(  ) = 0;

	std::string                 aName;

	// TODO: remove these parameters, maybe do that "MaterialVar" thing source has
	// as shaders would all use different paramters
	std::filesystem::path       aDiffusePath;
	std::filesystem::path       aNormalPath;
	// std::filesystem::path       aEmissionPath;

	TextureDescriptor*          apDiffuse;
	//TextureDescriptor*          apNormal;
	//TextureDescriptor*          apEmission;

	BaseShader*                 apShader = nullptr;
};

