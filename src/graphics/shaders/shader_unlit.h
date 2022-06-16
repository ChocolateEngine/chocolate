/*
shader_basic_3d.h ( Authored by Demez )

The Basic 3D Shader, starting point shader
*/
#pragma once

#include "shaders.h"


struct UnlitPushConstant
{
	alignas( 16 )glm::mat4 trans;
	alignas( 16 )int       index;
};


class ShaderUnlit : public BaseShader
{
public:	
	                    ShaderUnlit(): BaseShader() {}

	virtual void        Init() override;
	virtual void        ReInit() override;

	virtual void        CreateGraphicsPipeline(  ) override;

	virtual void        Draw( size_t renderableIndex, IRenderable* renderable, size_t matIndex, VkCommandBuffer c, uint32_t commandBufferIndex ) override;

	inline bool         UsesUniformBuffers() override { return false; };

	virtual void        AllocDrawData( size_t sRenderableCount ) override;
	virtual void        PrepareDrawData( size_t renderableIndex, IRenderable* renderable, size_t matIndex, uint32_t commandBufferCount ) override;

	virtual VertexFormat GetVertexFormat() override
	{
		return VertexFormat_Position | VertexFormat_TexCoord;
	}

	MemPool aDrawDataPool;
};

