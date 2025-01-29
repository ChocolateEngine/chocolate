#include "core/util.h"
#include "render/irender.h"
#include "graphics_int.h"


constexpr const char*       gpVertNormalShader = "shaders/vertex_normals.vert.spv";
constexpr const char*       gpVertColShader = "shaders/debug_col.vert.spv";
constexpr const char*       gpVertShader    = "shaders/debug.vert.spv";
constexpr const char*       gpFragShader    = "shaders/debug.frag.spv";


// CONVAR( r_wireframe_color_mode, 0 );
constexpr glm::vec4         gWireframeColor = { 0.f, 221.f / 255.f, 1.f, 1.f };


struct Debug_Push
{
	alignas( 16 ) glm::mat4 aModelMatrix;
	alignas( 16 ) glm::vec4 color;
	u32 aRenderable = 0;  // renderable index
	u32 aViewport   = 0;  // viewport index
};


struct Debug_Material
{
	glm::vec4 color;
};


static ShaderMaterialVarDesc gDebug_MaterialVars[] = {
	CH_SHADER_MATERIAL_VAR( Debug_Material, color, "Color", glm::vec4( 1.f, 1.f, 1.f, 1.f ) ),
};


static ShaderMaterialVarDesc gWireframe_MaterialVars[] = {
	CH_SHADER_MATERIAL_VAR( Debug_Material, color, "Color", gWireframeColor ),
};


constexpr int COLOR_MAT_INDEX = 0;


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


static void Shader_Debug_PushConstants( ch_handle_t cmd, ch_handle_t sLayout, const ShaderPushData_t& sPushData )
{
	Debug_Push push{};
	push.aModelMatrix = sPushData.apRenderable->aModelMatrix;

	// this should never happen, but it is, need to fix
	if ( sPushData.apMaterialData->vars.empty() )
	{
		push.color = glm::vec4( 1.f, 1.f, 1.f, 1.f );
	}
	else
	{
		push.color = sPushData.apMaterialData->vars[ COLOR_MAT_INDEX ].aVec4;
	}

	push.aRenderable  = sPushData.aRenderableIndex;
	push.aViewport    = sPushData.aViewportIndex;

	render->CmdPushConstants( cmd, sLayout, ShaderStage_Vertex | ShaderStage_Fragment, 0, sizeof( Debug_Push ), &push );
}


ShaderCreate_t gShaderCreate_Debug = {
	.apName             = "debug",
	.aStages            = ShaderStage_Vertex | ShaderStage_Fragment,
	.aBindPoint         = EPipelineBindPoint_Graphics,
	.aDynamicState      = EDynamicState_Viewport | EDynamicState_Scissor | EDynamicState_LineWidth,
	.aVertexFormat      = VertexFormat_Position,

	.apInit             = nullptr,
	.apDestroy          = nullptr,
	.apLayoutCreate     = Shader_Debug_GetPipelineLayoutCreate,
	.apGraphicsCreate   = Shader_Debug_GetGraphicsPipelineCreate,
	.apShaderPush       = Shader_Debug_PushConstants,

	.apMaterialVars     = gDebug_MaterialVars,
	.aMaterialVarCount  = CH_ARR_SIZE( gDebug_MaterialVars ),
	.aMaterialSize      = sizeof( Debug_Material ),
	.aUseMaterialBuffer = false,
};


CH_REGISTER_SHADER( gShaderCreate_Debug );


// -----------------------------------------------------------------------------------------------
// Debug Line Shader


static void Shader_DebugLine_GetGraphicsPipelineCreate( GraphicsPipelineCreate_t& srGraphics )
{
	srGraphics.aShaderModules.push_back( { ShaderStage_Vertex, gpVertShader, "main" } );
	srGraphics.aShaderModules.push_back( { ShaderStage_Fragment, gpFragShader, "main" } );

	srGraphics.aColorBlendAttachments.emplace_back( true );

	srGraphics.aPrimTopology = EPrimTopology_Line;
	srGraphics.aDynamicState = EDynamicState_Viewport | EDynamicState_Scissor | EDynamicState_LineWidth;
	srGraphics.aCullMode     = ECullMode_Back;
}


ShaderCreate_t gShaderCreate_DebugLine = {
	.apName             = "debug_line",
	.aBindPoint         = EPipelineBindPoint_Graphics,
	.aDynamicState      = EDynamicState_Viewport | EDynamicState_Scissor | EDynamicState_LineWidth,
	.aVertexFormat      = VertexFormat_Position | VertexFormat_Color,
	.apInit             = nullptr,
	.apDestroy          = nullptr,
	.apLayoutCreate     = Shader_Debug_GetPipelineLayoutCreate,
	.apGraphicsCreate   = Shader_DebugLine_GetGraphicsPipelineCreate,
	.apShaderPush       = Shader_Debug_PushConstants,

	.apMaterialVars     = gDebug_MaterialVars,
	.aMaterialVarCount  = CH_ARR_SIZE( gDebug_MaterialVars ),
	.aMaterialSize      = sizeof( Debug_Material ),
	.aUseMaterialBuffer = false,
};


CH_REGISTER_SHADER( gShaderCreate_DebugLine );


// -----------------------------------------------------------------------------------------------
// Wireframe Shader is just the debug shader with a different push constant lol


static void Shader_Wireframe_PushConstants( ch_handle_t cmd, ch_handle_t sLayout, const ShaderPushData_t& sPushData )
{
	Debug_Push push{};
	push.aModelMatrix = sPushData.apRenderable->aModelMatrix;
	push.aRenderable  = sPushData.aRenderableIndex;
	push.aViewport    = sPushData.aViewportIndex;

	// this should never happen, but it is, need to fix
	if ( sPushData.apMaterialData )
		push.color = sPushData.apMaterialData->vars[ COLOR_MAT_INDEX ].aVec4;
	else
		push.color = gWireframeColor;

	render->CmdPushConstants( cmd, sLayout, ShaderStage_Vertex | ShaderStage_Fragment, 0, sizeof( Debug_Push ), &push );
}


ShaderCreate_t gShaderCreate_Wireframe = {
	.apName             = "wireframe",
	.aStages            = ShaderStage_Vertex | ShaderStage_Fragment,
	.aBindPoint         = EPipelineBindPoint_Graphics,
	.aDynamicState      = EDynamicState_Viewport | EDynamicState_Scissor | EDynamicState_LineWidth,
	.aVertexFormat      = VertexFormat_Position,
	.apInit             = nullptr,
	.apDestroy          = nullptr,
	.apLayoutCreate     = Shader_Debug_GetPipelineLayoutCreate,
	.apGraphicsCreate   = Shader_Debug_GetGraphicsPipelineCreate,
	.apShaderPush       = Shader_Wireframe_PushConstants,

	.apMaterialVars     = gWireframe_MaterialVars,
	.aMaterialVarCount  = CH_ARR_SIZE( gWireframe_MaterialVars ),
	.aMaterialSize      = sizeof( Debug_Material ),
	.aUseMaterialBuffer = false,
};


CH_REGISTER_SHADER( gShaderCreate_Wireframe );


// -----------------------------------------------------------------------------------------------

#if 0

static void Shader_VertexNormals_GetPipelineLayoutCreate( PipelineLayoutCreate_t& srPipeline )
{
	srPipeline.aPushConstants.push_back( { ShaderStage_Vertex | ShaderStage_Fragment, 0, sizeof( Debug_Push ) } );
}


static void Shader_VertexNormals_GetGraphicsPipelineCreate( GraphicsPipelineCreate_t& srGraphics )
{
	srGraphics.aShaderModules.push_back( { ShaderStage_Vertex, gpVertNormalShader, "main" } );
	srGraphics.aShaderModules.push_back( { ShaderStage_Fragment, gpFragShader, "main" } );

	srGraphics.aColorBlendAttachments.emplace_back( true );

	srGraphics.aPrimTopology = EPrimTopology_Line;
	srGraphics.aLineMode     = false;
	srGraphics.aDynamicState = EDynamicState_Viewport | EDynamicState_Scissor | EDynamicState_LineWidth;
	srGraphics.aCullMode     = ECullMode_None;
}


static void Shader_VertexNormals_PushConstants( ch_handle_t cmd, ch_handle_t sLayout, const ShaderPushData_t& sPushData )
{
	Debug_Push push{};
	push.aModelMatrix = sPushData.apRenderable->aModelMatrix;
	push.aRenderable  = sPushData.aRenderableIndex;
	push.aViewport    = sPushData.aViewportIndex;

	// this should never happen, but it is, need to fix
//	if ( sPushData.apMaterialData )
//		push.color = sPushData.apMaterialData->vars[ COLOR_MAT_INDEX ].aVec4;
//	else
//		push.color = gWireframeColor;

	render->CmdPushConstants( cmd, sLayout, ShaderStage_Vertex | ShaderStage_Fragment, 0, sizeof( Debug_Push ), &push );
}


ShaderCreate_t gShaderCreate_VertexNormals = {
	.apName           = "__vertex_normals",
	.aStages          = ShaderStage_Vertex | ShaderStage_Fragment,
	.aBindPoint       = EPipelineBindPoint_Graphics,
	.aDynamicState    = EDynamicState_Viewport | EDynamicState_Scissor | EDynamicState_LineWidth,
	.aVertexFormat    = VertexFormat_Position | VertexFormat_Normal,
	.apInit           = nullptr,
	.apDestroy        = nullptr,
	.apLayoutCreate   = Shader_VertexNormals_GetPipelineLayoutCreate,
	.apGraphicsCreate = Shader_VertexNormals_GetGraphicsPipelineCreate,
	.apShaderPush     = Shader_VertexNormals_PushConstants,
};


CH_REGISTER_SHADER( gShaderCreate_VertexNormals );

#endif
