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

	virtual void                       CreateLayouts();

	virtual void                       CreateGraphicsPipeline() = 0;

	inline virtual void                InitUniformBuffer( IMesh* mesh ) {};

	virtual void                       UpdateBuffers( uint32_t sCurrentImage, size_t renderableIndex, BaseRenderable* spRenderable ) {};

	// um 2
	virtual void                       UpdateBuffers( uint32_t sCurrentImage, BaseRenderable* spRenderable )
	{
		UpdateBuffers( sCurrentImage, 0, spRenderable );
	}

	virtual void                       BindBuffers( BaseRenderable* renderable, VkCommandBuffer c, uint32_t cIndex );

	virtual void                       Bind( VkCommandBuffer c, uint32_t cIndex );
	virtual void                       Draw( BaseRenderable* renderable, VkCommandBuffer c, uint32_t commandBufferIndex ) {};

	// um
	virtual void                       Draw( size_t renderableIndex, BaseRenderable* renderable, VkCommandBuffer c, uint32_t commandBufferIndex )
	{
		Draw( renderable, c, commandBufferIndex );
	}

	virtual void                       AllocDrawData( size_t sRenderableCount ) {};
	virtual void                       PrepareDrawData( size_t renderableIndex, BaseRenderable* renderable, uint32_t commandBufferCount ) {};

	virtual bool                       UsesUniformBuffers() = 0;

	inline VkPipeline                  GetPipeline() const        { return aPipeline; }
	inline VkPipelineLayout            GetPipelineLayout() const  { return aPipelineLayout; }
};


