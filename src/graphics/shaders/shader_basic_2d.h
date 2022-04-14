/*
shader_basic_3d.h ( Authored by Demez )

The Basic 3D Shader, starting point shader
*/
#pragma once

#include "shaders.h"


class Basic2D : public BaseShader
{
public:	
	                    Basic2D(): BaseShader() {}

	virtual void        Init() override;
	virtual void        ReInit() override;

	virtual void        CreateGraphicsPipeline(  ) override;

	virtual void        UpdateBuffers( uint32_t sCurrentImage, size_t renderableIndex, BaseRenderable* spRenderable ) override;

	virtual void        Draw( size_t renderableIndex, BaseRenderable* renderable, VkCommandBuffer c, uint32_t commandBufferIndex ) override;

	inline bool         UsesUniformBuffers(  ) override { return false; };

	VkVertexInputBindingDescription                             GetBindingDesc(  );
	std::array< VkVertexInputAttributeDescription, 3 >          GetAttributeDesc(  );

	virtual void        AllocDrawData( size_t sRenderableCount ) override;
	virtual void        PrepareDrawData( size_t renderableIndex, BaseRenderable* renderable, uint32_t commandBufferCount ) override;

	std::unordered_map< BaseRenderable*, push_constant_t* > aDrawData;
	MemPool aDrawDataPool;
};

