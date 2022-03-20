/*
shader_skybox.h ( Authored by Demez )

3D Skybox Shader
*/
#pragma once

#include "shaders.h"

class ShaderSkybox: public BaseShader
{
public:	
	                    ShaderSkybox(): BaseShader() {}

	virtual void        Init() override;
	virtual void        ReInit() override;

	// virtual std::vector<VkDescriptorSetLayoutBinding> GetDescriptorSetLayoutBindings(  ) override;

	virtual void        CreateGraphicsPipeline(  ) override;

	virtual void        UpdateBuffers( uint32_t sCurrentImage, BaseRenderable* spRenderable ) override;

	virtual void        Draw( BaseRenderable* renderable, VkCommandBuffer c, uint32_t commandBufferIndex ) override;

	inline bool         UsesUniformBuffers(  ) override { return false; };
};

