/*
shader_basic_3d.h ( Authored by Demez )

The Basic 3D Shader, starting point shader
*/
#pragma once

#include "shaders.h"

class ShaderUnlit : public BaseShader
{
protected:
	const char *pVShader = "shaders/unlit_v.spv";
	const char *pFShader = "shaders/unlit_f.spv";

public:	
	                    ShaderUnlit(): BaseShader() {}

	virtual void        Init() override;
	virtual void        ReInit() override;

	virtual std::vector<VkDescriptorSetLayoutBinding> GetDescriptorSetLayoutBindings(  ) override;

	virtual void        CreateGraphicsPipeline(  ) override;

	virtual void        UpdateBuffers( uint32_t sCurrentImage, BaseRenderable* spRenderable ) override;

	virtual void        Draw( BaseRenderable* renderable, VkCommandBuffer c, uint32_t commandBufferIndex ) override;

	//inline bool         UsesUniformBuffers(  ) override { return true; };

	VkVertexInputBindingDescription                             GetBindingDesc(  );
	std::array< VkVertexInputAttributeDescription, 4 >          GetAttributeDesc(  );
};
