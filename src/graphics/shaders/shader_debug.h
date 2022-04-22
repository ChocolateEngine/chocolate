/*
shader_debug.h ( Authored by Demez )

The Debug Shader, used for wireframe
*/
#pragma once

#include "shaders.h"

class ShaderDebug: public BaseShader
{
public:	
	                    ShaderDebug();
	                    ShaderDebug( const std::string& name );

	// virtual void        RegisterShader();

	virtual void        Init() override;
	virtual void        ReInit() override;

	virtual void        CreateGraphicsPipeline(  ) override;

	virtual void        Draw( BaseRenderable* renderable, VkCommandBuffer c, uint32_t commandBufferIndex ) override;

	inline bool         UsesUniformBuffers() override { return false; };

	virtual VkPrimitiveTopology GetTopologyType() { return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST; }
	virtual void        CmdPushConst( BaseRenderable* renderable, VkCommandBuffer c, uint32_t cIndex );
};


class ShaderDebugLine: public ShaderDebug
{
public:	
	                    ShaderDebugLine( const std::string& name );
						
	// virtual void        RegisterShader() override;

	virtual void        ReInit() override;

	VkPrimitiveTopology GetTopologyType() override { return VK_PRIMITIVE_TOPOLOGY_LINE_LIST; }
	void                CmdPushConst( BaseRenderable* renderable, VkCommandBuffer c, uint32_t cIndex ) override;
};


