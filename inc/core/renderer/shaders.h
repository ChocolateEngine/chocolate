/*
shaders.h ( Authored by p0lyhdron )

Declares shader effect stuff.
*/
#pragma once

#include "../../types/databuffer.hh"
#include "allocator.h"

#include <vulkan/vulkan.hpp>

class BaseShader
{
	typedef DataBuffer< VkDescriptorSetLayout > 	Layouts;
	typedef DataBuffer< VkShaderModule >		ShaderModules;
protected:
	VkPipeline		aPipeline;
	VkPipelineLayout 	aPipelineLayout;
	Layouts			aSetLayouts;
	ShaderModules		aModules;
public:
	virtual void 		Init(  ) = 0;
	VkPipeline		GetPipeline(  )		{ return aPipeline; }
	VkPipelineLayout        GetPipelineLayout(  )	{ return aPipelineLayout; }
};

class Basic3D : public BaseShader
{
protected:
	const char *pVShader = "materials/shaders/3dvert.spv";
	const char *pFShader = "materials/shaders/3dfrag.spv";
public:	
	void	Init(  ) override;
};
