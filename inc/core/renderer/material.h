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
	                    Material(  );
	virtual            ~Material(  );

	//virtual void        Init(  ) = 0;

	std::string                 aName;
	std::filesystem::path       aDiffuseTexture;
	std::filesystem::path       aNormalTexture;
	BaseShader*                 apShader = nullptr;
};

