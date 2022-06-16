/*
shaders.h ( Authored by p0lyhdron )

Declares shader effect stuff.
*/
#pragma once

#include "types/databuffer.hh"
#include "../allocator.h"
#include "graphics/imaterialsystem.h"

#include <vulkan/vulkan.hpp>

class Renderer;
class MaterialSystem;
class IModel;


// Note: a lot of laziness going on here lmao
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

	// Graphics Pipeline Creation
	// virtual VkPipelineVertexInputStateCreateInfo        CreateVertexInputInfo();

	inline virtual void                InitUniformBuffer( IModel* mesh ) {};

	virtual void                       UpdateBuffers( uint32_t sCurrentImage, size_t renderableIndex, IRenderable* spRenderable, size_t matIndex ) {};

	// um 2
	virtual void                       UpdateBuffers( uint32_t sCurrentImage, IRenderable* spRenderable )
	{
		UpdateBuffers( sCurrentImage, 0, spRenderable, 0 );
	}

	virtual void                       BindBuffers( IRenderable* renderable, size_t matIndex, VkCommandBuffer c, uint32_t cIndex );

	virtual void                       Bind( VkCommandBuffer c, uint32_t cIndex );
	virtual void                       Draw( IRenderable* renderable, VkCommandBuffer c, uint32_t commandBufferIndex ) {};

	// um
	virtual void                       Draw( size_t renderableIndex, IRenderable* renderable, size_t matIndex, VkCommandBuffer c, uint32_t commandBufferIndex )
	{
		Draw( renderable, c, commandBufferIndex );
	}

	virtual void                       CmdDraw( IModel* renderable, size_t matIndex, VkCommandBuffer c );

	virtual void                       AllocDrawData( size_t sRenderableCount ) {};
	virtual void                       PrepareDrawData( size_t renderableIndex, IRenderable* renderable, size_t matIndex, uint32_t commandBufferCount ) {};

	virtual bool                       UsesUniformBuffers() = 0;

	inline VkPipeline                  GetPipeline() const        { return aPipeline; }
	inline VkPipelineLayout            GetPipelineLayout() const  { return aPipelineLayout; }

	virtual VertexFormat               GetVertexFormat() = 0;
};

