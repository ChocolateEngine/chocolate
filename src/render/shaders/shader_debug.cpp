#include "util.h"
#include "render/irender.h"
#include "graphics_int.h"


constexpr const char*       gpVertColShader = "shaders/debug_col.vert.spv";
constexpr const char*       gpVertShader    = "shaders/debug.vert.spv";
constexpr const char*       gpFragShader    = "shaders/debug.frag.spv";


struct Debug_Push
{
	alignas( 16 ) glm::mat4 aModelMatrix;
	alignas( 16 ) glm::vec4 aColor;
	u32 aRenderable = 0;  // renderable index
	u32 aViewport   = 0;  // viewport index
};


static std::unordered_map< SurfaceDraw_t*, Debug_Push > gDebugPushData;
static std::unordered_map< SurfaceDraw_t*, Debug_Push > gWireframePushData;
static std::unordered_map< SurfaceDraw_t*, Debug_Push > gDebugLinePushData;


// -----------------------------------------------------------------------------------------------
// Debug Shader


static void Shader_Debug_GetPipelineLayoutCreate( PipelineLayoutCreate_t& srPipeline )
{
	srPipeline.aPushConstants.push_back( { ShaderStage_Vertex | ShaderStage_Fragment, 0, sizeof( Debug_Push ) } );
}


static void Shader_Debug_GetGraphicsPipelineCreate( GraphicsPipelineCreate_t& srGraphics )
{
	srGraphics.aShaderModules.push_back( { ShaderStage_Vertex, gpVertColShader, "main" } );
	srGraphics.aShaderModules.push_back( { ShaderStage_Fragment, gpFragShader, "main" } );

	srGraphics.aColorBlendAttachments.emplace_back( true );

	srGraphics.aPrimTopology = EPrimTopology_Tri;
	srGraphics.aLineMode     = true;
	srGraphics.aDynamicState = EDynamicState_Viewport | EDynamicState_Scissor | EDynamicState_LineWidth;
	srGraphics.aCullMode     = ECullMode_None;
}


static void Shader_Debug_ResetPushData()
{
	gDebugPushData.clear();
}


static void Shader_Debug_SetupPushData( u32 sRenderableIndex, u32 sViewportIndex, Renderable_t* spDrawData, SurfaceDraw_t& srDrawInfo, ShaderMaterialData* spMaterialData )
{
	Debug_Push& push  = gDebugPushData[ &srDrawInfo ];
	push.aModelMatrix = spDrawData->aModelMatrix;
	Handle mat        = gGraphics.Model_GetMaterial( spDrawData->aModel, srDrawInfo.aSurface );
	push.aColor       = gGraphics.Mat_GetVec4( mat, "color", { 1.f, 1.f, 1.f, 1.f } );
	push.aRenderable  = sRenderableIndex;
	push.aViewport    = sViewportIndex;
}


static void Shader_Debug_PushConstants( Handle cmd, Handle sLayout, SurfaceDraw_t& srDrawInfo )
{
	Debug_Push& push = gDebugPushData.at( &srDrawInfo );
	render->CmdPushConstants( cmd, sLayout, ShaderStage_Vertex | ShaderStage_Fragment, 0, sizeof( Debug_Push ), &push );
}


static IShaderPush gShaderPush_Debug = {
	.apReset = Shader_Debug_ResetPushData,
	.apSetup = Shader_Debug_SetupPushData,
	.apPush  = Shader_Debug_PushConstants,
};


ShaderCreate_t gShaderCreate_Debug = {
	.apName           = "debug",
	.aStages          = ShaderStage_Vertex | ShaderStage_Fragment,
	.aBindPoint       = EPipelineBindPoint_Graphics,
	.aFlags           = EShaderFlags_PushConstant,
	.aDynamicState    = EDynamicState_Viewport | EDynamicState_Scissor | EDynamicState_LineWidth,
	.aVertexFormat    = VertexFormat_Position,
	.apInit           = nullptr,
	.apDestroy        = nullptr,
	.apLayoutCreate   = Shader_Debug_GetPipelineLayoutCreate,
	.apGraphicsCreate = Shader_Debug_GetGraphicsPipelineCreate,
	.apShaderPush     = &gShaderPush_Debug,
};


CH_REGISTER_SHADER( gShaderCreate_Debug );


// -----------------------------------------------------------------------------------------------
// Debug Line Shader


static void Shader_DebugLine_ResetPushData()
{
	gDebugLinePushData.clear();
}


static void Shader_DebugLine_SetupPushData( u32 sRenderableIndex, u32 sViewportIndex, Renderable_t* spDrawData, SurfaceDraw_t& srDrawInfo, ShaderMaterialData* spMaterialData )
{
	Debug_Push& push  = gDebugLinePushData[ &srDrawInfo ];
	push.aModelMatrix = spDrawData->aModelMatrix;
	Handle mat        = gGraphics.Model_GetMaterial( spDrawData->aModel, srDrawInfo.aSurface );
	push.aColor       = gGraphics.Mat_GetVec4( mat, "color", { 1.f, 1.f, 1.f, 1.f } );
	push.aRenderable  = sRenderableIndex;
	push.aViewport    = sViewportIndex;
}


static void Shader_DebugLine_PushConstants( Handle cmd, Handle sLayout, SurfaceDraw_t& srDrawInfo )
{
	Debug_Push& push = gDebugLinePushData.at( &srDrawInfo );
	render->CmdPushConstants( cmd, sLayout, ShaderStage_Vertex | ShaderStage_Fragment, 0, sizeof( Debug_Push ), &push );
}


static void Shader_DebugLine_GetGraphicsPipelineCreate( GraphicsPipelineCreate_t& srGraphics )
{
	srGraphics.aShaderModules.push_back( { ShaderStage_Vertex, gpVertShader, "main" } );
	srGraphics.aShaderModules.push_back( { ShaderStage_Fragment, gpFragShader, "main" } );

	srGraphics.aColorBlendAttachments.emplace_back( true );

	srGraphics.aPrimTopology = EPrimTopology_Line;
	srGraphics.aDynamicState = EDynamicState_Viewport | EDynamicState_Scissor | EDynamicState_LineWidth;
	srGraphics.aCullMode     = ECullMode_Back;
}


static IShaderPush gShaderPush_DebugLine = {
	.apReset = Shader_DebugLine_ResetPushData,
	.apSetup = Shader_DebugLine_SetupPushData,
	.apPush  = Shader_DebugLine_PushConstants,
};


ShaderCreate_t gShaderCreate_DebugLine = {
	.apName           = "debug_line",
	.aBindPoint       = EPipelineBindPoint_Graphics,
	.aFlags           = EShaderFlags_PushConstant,
	.aDynamicState    = EDynamicState_Viewport | EDynamicState_Scissor | EDynamicState_LineWidth,
	.aVertexFormat    = VertexFormat_Position | VertexFormat_Color,
	.apInit           = nullptr,
	.apDestroy        = nullptr,
	.apLayoutCreate   = Shader_Debug_GetPipelineLayoutCreate,
	.apGraphicsCreate = Shader_DebugLine_GetGraphicsPipelineCreate,
	.apShaderPush     = &gShaderPush_DebugLine,
};


CH_REGISTER_SHADER( gShaderCreate_DebugLine );


// -----------------------------------------------------------------------------------------------
// Wireframe Shader is just the debug shader with a different push constant lol


// CONVAR( r_wireframe_color_mode, 0 );
constexpr glm::vec4 gWireframeColor = { 0.f, 221.f / 255.f, 1.f, 1.f };


static void Shader_Wireframe_ResetPushData()
{
	gWireframePushData.clear();
}


static void Shader_Wireframe_SetupPushData( u32 sRenderableIndex, u32 sViewportIndex, Renderable_t* spDrawData, SurfaceDraw_t& srDrawInfo, ShaderMaterialData* spMaterialData )
{
	Debug_Push& push  = gWireframePushData[ &srDrawInfo ];
	push.aModelMatrix = spDrawData->aModelMatrix;
	Handle mat        = gGraphics.Model_GetMaterial( spDrawData->aModel, srDrawInfo.aSurface );
	push.aColor       = gGraphics.Mat_GetVec4( mat, "color", gWireframeColor );
	push.aRenderable  = sRenderableIndex;
	push.aViewport    = sViewportIndex;
}


static void Shader_Wireframe_PushConstants( Handle cmd, Handle sLayout, SurfaceDraw_t& srDrawInfo )
{
	Debug_Push& push = gWireframePushData.at( &srDrawInfo );
	render->CmdPushConstants( cmd, sLayout, ShaderStage_Vertex | ShaderStage_Fragment, 0, sizeof( Debug_Push ), &push );
}


static IShaderPush gShaderPush_Wireframe = {
	.apReset = Shader_Wireframe_ResetPushData,
	.apSetup = Shader_Wireframe_SetupPushData,
	.apPush  = Shader_Wireframe_PushConstants,
};


ShaderCreate_t gShaderCreate_Wireframe = {
	.apName           = "wireframe",
	.aStages          = ShaderStage_Vertex | ShaderStage_Fragment,
	.aBindPoint       = EPipelineBindPoint_Graphics,
	.aFlags           = EShaderFlags_PushConstant,
	.aDynamicState    = EDynamicState_Viewport | EDynamicState_Scissor | EDynamicState_LineWidth,
	.aVertexFormat    = VertexFormat_Position,
	.apInit           = nullptr,
	.apDestroy        = nullptr,
	.apLayoutCreate   = Shader_Debug_GetPipelineLayoutCreate,
	.apGraphicsCreate = Shader_Debug_GetGraphicsPipelineCreate,
	.apShaderPush     = &gShaderPush_Wireframe,
};


CH_REGISTER_SHADER( gShaderCreate_Wireframe );

