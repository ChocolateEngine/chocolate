/*
shader_skybox.h ( Authored by Demez )

3D Skybox Shader
*/
#pragma once

#include "shaders.h"


struct SkyboxPushConst
{
	alignas( 16 )glm::mat4 trans;
	alignas( 16 )int       sky;
};


class ShaderSkybox: public BaseShader
{
public:	
	                    ShaderSkybox(): BaseShader() {}

	virtual void        Init() override;
	virtual void        ReInit() override;

	virtual void        CreateGraphicsPipeline(  ) override;

	virtual void        Draw( size_t renderableIndex, IRenderable* spRenderable, size_t matIndex, VkCommandBuffer c, uint32_t commandBufferIndex ) override;

	inline bool         UsesUniformBuffers(  ) override { return false; };

	virtual void        AllocDrawData( size_t sRenderableCount ) override;
	virtual void        PrepareDrawData( size_t renderableIndex, IRenderable* spRenderable, size_t material, uint32_t commandBufferCount ) override;

	VertexFormat        GetVertexFormat() override
	{
		return VertexFormat_Position;
	}

	MemPool aDrawDataPool;
};

