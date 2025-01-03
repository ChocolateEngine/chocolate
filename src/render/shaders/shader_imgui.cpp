#include "core/util.h"
#include "render/irender.h"
#include "graphics_int.h"

#if 0

extern IRender*             render;

static ch_handle_t               gPipeline       = CH_INVALID_HANDLE;
static ch_handle_t               gPipelineLayout = CH_INVALID_HANDLE;

// constexpr const char* gpVertShader    = "shaders/basic3d.vert.spv";
// constexpr const char* gpFragShader    = "shaders/basic3d.frag.spv";

constexpr const char*       gpVertShader    = "shaders/imgui.vert.spv";
constexpr const char*       gpFragShader    = "shaders/imgui.frag.spv";

extern ShaderBufferArray_t gUniformSampler;

struct UI_Push
{
	int index;
};


// struct Basic3D_UBO
// {
// 	int   diffuse = 0, ao = 0, emissive = 0;
// 	float aoPower = 1.f, emissivePower = 1.f;
// 
// 	float morphWeight = 0.f;
// };


static std::unordered_map< ch_handle_t, UI_Push > gPushData;


ch_handle_t Shader_UI_Create( ch_handle_t sRenderPass, bool sRecreate )
{
	PipelineLayoutCreate_t pipelineCreateInfo{};
	pipelineCreateInfo.aLayouts.push_back( gUniformSampler.aLayout );
	pipelineCreateInfo.aPushConstants.emplace_back( ShaderStage_Vertex | ShaderStage_Fragment, 0, sizeof( UI_Push ) );

	// --------------------------------------------------------------

	GraphicsPipelineCreate_t pipelineInfo{};

	pipelineInfo.apName = "imgui";
	pipelineInfo.aShaderModules.emplace_back( ShaderStage_Vertex, gpVertShader, "main" );
	pipelineInfo.aShaderModules.emplace_back( ShaderStage_Fragment, gpFragShader, "main" );

	pipelineInfo.aColorBlendAttachments.emplace_back( true );

	pipelineInfo.aPrimTopology   = EPrimTopology_Tri;
	pipelineInfo.aDynamicState   = EDynamicState_Viewport | EDynamicState_Scissor;
	pipelineInfo.aCullMode       = ECullMode_None;
	pipelineInfo.aPipelineLayout = gPipelineLayout;
	pipelineInfo.aRenderPass     = sRenderPass;

	// TODO: expose the rest later

	// --------------------------------------------------------------

	if ( !render->CreatePipelineLayout( gPipelineLayout, pipelineCreateInfo ) )
	{
		Log_Error( "Failed to create Pipeline Layout\n" );
		return CH_INVALID_HANDLE;
	}

	pipelineInfo.aPipelineLayout = gPipelineLayout;
	if ( !render->CreateGraphicsPipeline( gPipeline, pipelineInfo ) )
	{
		Log_Error( "Failed to create Graphics Pipeline\n" );
		return CH_INVALID_HANDLE;
	}

	return gPipeline;
}


void Shader_UI_Destroy()
{
	if ( gPipelineLayout )
		render->DestroyPipelineLayout( gPipelineLayout );

	if ( gPipeline )
		render->DestroyPipeline( gPipeline );

	gPipelineLayout = CH_INVALID_HANDLE;
	gPipeline       = CH_INVALID_HANDLE;
}


void Shader_UI_Draw( ch_handle_t cmd, size_t sCmdIndex, ch_handle_t shColor )
{
	if ( !render->CmdBindPipeline( cmd, gPipeline ) )
	{
		Log_Error( gLC_ClientGraphics, "UI Shader Not Found!\n" );
		return;
	}

	UI_Push push{};
	push.index = render->GetTextureIndex( shColor );

	render->CmdPushConstants( cmd, gPipelineLayout, ShaderStage_Vertex | ShaderStage_Fragment, 0, sizeof( UI_Push ), &push );
	render->CmdBindDescriptorSets( cmd, sCmdIndex, EPipelineBindPoint_Graphics, gPipelineLayout, &gUniformSampler.aSets[ sCmdIndex ], 1 );
	render->CmdDraw( cmd, 3, 1, 0, 0 );
}

#endif
