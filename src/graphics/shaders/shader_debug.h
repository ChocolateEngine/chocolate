/*
shader_debug.h ( Authored by Demez )

The Debug Shader, used for wireframe
*/
#pragma once

#include "shaders.h"

class ShaderDebug: public BaseShader
{
protected:


public:	
	                    ShaderDebug(): BaseShader() {}

	virtual void        Init() override;
	virtual void        ReInit() override;

	virtual std::vector<VkDescriptorSetLayoutBinding> GetDescriptorSetLayoutBindings(  ) override;

	virtual void        CreateGraphicsPipeline(  ) override;

	virtual void        UpdateBuffers( uint32_t sCurrentImage, BaseRenderable* spRenderable ) override;

	virtual void        Draw( BaseRenderable* renderable, VkCommandBuffer c, uint32_t commandBufferIndex ) override;

	inline bool         UsesUniformBuffers() override { return false; };
};


struct DebugPushConstant
{
	alignas(16) glm::mat4 aTransform;
};


