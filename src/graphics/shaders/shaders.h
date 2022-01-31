/*
shaders.h ( Authored by p0lyhdron )

Declares shader effect stuff.
*/
#pragma once

#include "types/databuffer.hh"
#include "../allocator.h"

#include <vulkan/vulkan.hpp>

class Renderer;
class MaterialSystem;
class BaseRenderable;


class BaseShader
{
	typedef DataBuffer< VkDescriptorSetLayout >     Layouts;
	typedef DataBuffer< VkShaderModule >            ShaderModules;

// protected:
public:
	VkPipeline                         aPipeline = nullptr;
	VkPipelineLayout                   aPipelineLayout = nullptr;
	Layouts                            aLayouts;
	ShaderModules                      aModules;

	std::string                        aName;

public:
	                                   BaseShader();
	virtual                            ~BaseShader();

	virtual void                       Init();
	virtual void                       Destroy();

	/* Reinitialize data that is useless after the swapchain becomes outdated.  */
	virtual void                       ReInit();

	void                                              CreateDescriptorSetLayout(  );
	virtual std::vector<VkDescriptorSetLayoutBinding> GetDescriptorSetLayoutBindings(  ) = 0;

	virtual void                       CreateGraphicsPipeline(  ) = 0;

	virtual void                       UpdateBuffers( uint32_t sCurrentImage, BaseRenderable* spRenderable ) = 0;

	virtual void                       Draw( BaseRenderable* renderable, VkCommandBuffer c, uint32_t commandBufferIndex ) = 0;

	virtual bool                       UsesUniformBuffers(  ) = 0;

	inline VkPipeline                  GetPipeline() const        { return aPipeline; }
	inline VkPipelineLayout            GetPipelineLayout() const  { return aPipelineLayout; }
};


