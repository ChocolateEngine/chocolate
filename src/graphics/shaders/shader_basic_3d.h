/*
shader_basic_3d.h ( Authored by Demez )

The Basic 3D Shader, starting point shader
*/
#pragma once

#include "shaders.h"

class Basic3D : public BaseShader
{
protected:
	const char *pVShader = "materials/shaders/3dvert.spv";
	const char *pFShader = "materials/shaders/3dfrag.spv";

public:	
	                    Basic3D(): BaseShader() {}

	virtual void        Init() override;
	virtual void        ReInit() override;

	virtual std::vector<VkDescriptorSetLayoutBinding> GetDescriptorSetLayoutBindings(  ) override;

	virtual void        CreateGraphicsPipeline(  ) override;

	virtual void        UpdateBuffers( uint32_t sCurrentImage, BaseRenderable* spRenderable ) override;

	//inline bool         UsesUniformBuffers(  ) override { return true; };

	VkVertexInputBindingDescription                             GetBindingDesc(  );
	std::array< VkVertexInputAttributeDescription, 4 >          GetAttributeDesc(  );
};

