/*
shaders.h ( Authored by p0lyhdron )

Declares shader effect stuff.
*/
#pragma once

#include "../../types/databuffer.hh"
#include "../core/renderer/shaders.h"
#include "allocator.h"

#include <vulkan/vulkan.hpp>

class Renderer;
class ModelData;
class Mesh;


class BaseShader
{
	typedef DataBuffer< VkDescriptorSetLayout >     Layouts;
	typedef DataBuffer< VkShaderModule >            ShaderModules;

// protected:
public:
	VkDescriptorSetLayout              aTextureLayout = nullptr;
	VkPipeline                         aPipeline = nullptr;
	VkPipelineLayout                   aPipelineLayout = nullptr;
	Layouts                            aSetLayouts;
	ShaderModules                      aModules;
	Renderer*                          apRenderer = nullptr;

public:
	                                   BaseShader( Renderer* renderer );
	virtual                            ~BaseShader();

	virtual void                       Init();
	virtual void                       Destroy();

	/* Reinitialize data that is useless after the swapchain becomes outdated.  */
	virtual void                       ReInit();

	virtual void                       UpdateUniformBuffers( uint32_t sCurrentImage, ModelData &srModelData, Mesh &srMesh ) = 0;

	inline VkPipeline                  GetPipeline() const        { return aPipeline; }
	inline VkPipelineLayout            GetPipelineLayout() const  { return aPipelineLayout; }
	inline VkDescriptorSetLayout       GetTextureLayout() const   { return aTextureLayout; }
};


class Basic3D : public BaseShader
{
protected:
	const char *pVShader = "materials/shaders/3dvert.spv";
	const char *pFShader = "materials/shaders/3dfrag.spv";
public:	
	                    Basic3D( Renderer* renderer ): BaseShader( renderer ) {}

	virtual void        Init() override;
	virtual void        UpdateUniformBuffers( uint32_t sCurrentImage, ModelData &srModelData, Mesh &srMesh ) override;
};

