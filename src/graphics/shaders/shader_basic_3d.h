/*
shader_basic_3d.h ( Authored by Demez )

The Basic 3D Shader, starting point shader
*/
#pragma once

#include "shaders.h"


class Basic3D : public BaseShader
{
public:	
	                    Basic3D(): BaseShader() {}

	virtual void        Init() override;
	virtual void        ReInit() override;

	virtual void                                      CreateDescriptorSetLayout() override;
	virtual std::vector<VkDescriptorSetLayoutBinding> GetDescriptorSetLayoutBindings(  ) override;

	virtual void        CreateGraphicsPipeline(  ) override;

	virtual void        InitUniformBuffer( IMesh* mesh ) override;

	virtual void        UpdateBuffers( uint32_t sCurrentImage, BaseRenderable* spRenderable ) override;

	virtual void        Draw( BaseRenderable* renderable, VkCommandBuffer c, uint32_t commandBufferIndex ) override;

	inline bool         UsesUniformBuffers(  ) override { return true; };
};

