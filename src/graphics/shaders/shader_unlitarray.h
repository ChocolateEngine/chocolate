/*
shader_basic_3d.h ( Authored by Demez )

The Basic 3D Shader, starting point shader
*/
#pragma once

#include "shaders.h"


struct UnlitArrayPushConstant
{
	alignas( 16 )glm::mat4 trans;
	alignas( 16 )int       index;
	int       layer;
};


class ShaderUnlitArray : public BaseShader
{
protected:


public:	
	                    ShaderUnlitArray(): BaseShader() {}

	virtual void        Init() override;
	virtual void        ReInit() override;

	virtual void        CreateGraphicsPipeline(  ) override;

	virtual void        UpdateBuffers( uint32_t sCurrentImage, BaseRenderable* spRenderable ) override;

	virtual void        Draw( size_t renderableIndex, BaseRenderable* renderable, VkCommandBuffer c, uint32_t commandBufferIndex ) override;

	inline bool         UsesUniformBuffers(  ) override { return false; };

	VkVertexInputBindingDescription                             GetBindingDesc(  );
	std::array< VkVertexInputAttributeDescription, 4 >          GetAttributeDesc(  );

	std::unordered_map< BaseRenderable*, UnlitArrayPushConstant* > aDrawData;
	MemPool aDrawDataPool;

	virtual void        AllocDrawData( size_t sRenderableCount ) override;
	virtual void        PrepareDrawData( size_t renderableIndex, BaseRenderable* renderable, uint32_t commandBufferCount ) override;
};

