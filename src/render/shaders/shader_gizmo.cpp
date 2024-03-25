#include "util.h"
#include "render/irender.h"
#include "graphics_int.h"


constexpr const char* gpVertShader = "shaders/gizmo.vert.spv";
constexpr const char* gpFragShader = "shaders/gizmo.frag.spv";


// Keep in sync with shader!
enum EGizmoColorMode : u8
{
	EGizmoColorMode_None     = 0,
	EGizmoColorMode_Hovered  = ( 1 << 0 ),
	EGizmoColorMode_Selected = ( 1 << 1 ),
};


struct Gizmo_Push
{
	alignas( 16 ) glm::mat4 aModelMatrix;
	alignas( 16 ) glm::vec4 aColor;
	alignas( 16 ) u32 aRenderable = 0;  // renderable index
	alignas( 16 ) u32 aViewport   = 0;  // viewport index

	u32 aColorMode                = EGizmoColorMode_None;
};


static std::unordered_map< SurfaceDraw_t*, Gizmo_Push > gGizmoPushData;


static void Shader_Gizmo_GetPipelineLayoutCreate( PipelineLayoutCreate_t& srPipeline )
{
	srPipeline.aPushConstants.push_back( { ShaderStage_Vertex | ShaderStage_Fragment, 0, sizeof( Gizmo_Push ) } );
}


static void Shader_Gizmo_GetGraphicsPipelineCreate( GraphicsPipelineCreate_t& srGraphics )
{
	srGraphics.aShaderModules.push_back( { ShaderStage_Vertex, gpVertShader, "main" } );
	srGraphics.aShaderModules.push_back( { ShaderStage_Fragment, gpFragShader, "main" } );

	srGraphics.aColorBlendAttachments.emplace_back( true );

	srGraphics.aPrimTopology = EPrimTopology_Tri;
	srGraphics.aLineMode     = false;
	srGraphics.aDynamicState = EDynamicState_Viewport | EDynamicState_Scissor;
	srGraphics.aCullMode     = ECullMode_Back;
}


static void Shader_Gizmo_ResetPushData()
{
	gGizmoPushData.clear();
}


static glm::vec4 AdjustColor( glm::vec4 baseColor, glm::vec4 darkenColor )
{
	return glm::mix( baseColor, darkenColor * 2.f, 0.5f );
}


static void Shader_Gizmo_SetupPushData( u32 sRenderableIndex, u32 sViewportIndex, Renderable_t* spDrawData, SurfaceDraw_t& srDrawInfo, ShaderMaterialData* spMaterialData )
{
	Gizmo_Push& push  = gGizmoPushData[ &srDrawInfo ];
	push.aModelMatrix = spDrawData->aModelMatrix;
	Handle mat        = gGraphics.Model_GetMaterial( spDrawData->aModel, srDrawInfo.aSurface );
	push.aColor       = gGraphics.Mat_GetVec4( mat, "color" );

	if ( gGraphics.Mat_GetBool( mat, "hovered" ) )
	{
		// push.aColor = AdjustColor( push.aColor, { 0.3f, 0.3f, 0.3f, 1.f } );
		push.aColor = { 1.f, 1.f, 0.f, 1.f };
	}

	if ( gGraphics.Mat_GetBool( mat, "selected" ) )
	{
		// push.aColor = AdjustColor( push.aColor, { 0.15f, 0.15f, 0.15f, 1.f } );
		push.aColor = { 1.f, 1.f, 1.f, 1.f };
	}

	push.aRenderable  = sRenderableIndex;
	push.aViewport    = sViewportIndex;
}


static void Shader_Gizmo_PushConstants( Handle cmd, Handle sLayout, SurfaceDraw_t& srDrawInfo )
{
	Gizmo_Push& push = gGizmoPushData.at( &srDrawInfo );
	render->CmdPushConstants( cmd, sLayout, ShaderStage_Vertex | ShaderStage_Fragment, 0, sizeof( Gizmo_Push ), &push );
}


static IShaderPush gShaderPush_Debug = {
	.apReset = Shader_Gizmo_ResetPushData,
	.apSetup = Shader_Gizmo_SetupPushData,
	.apPush  = Shader_Gizmo_PushConstants,
};


ShaderCreate_t gShaderCreate_Gizmo = {
	.apName           = "gizmo",
	.aStages          = ShaderStage_Vertex | ShaderStage_Fragment,
	.aBindPoint       = EPipelineBindPoint_Graphics,
	.aFlags           = EShaderFlags_PushConstant,
	.aDynamicState    = EDynamicState_Viewport | EDynamicState_Scissor,
	.aVertexFormat    = VertexFormat_Position,
	.apInit           = nullptr,
	.apDestroy        = nullptr,
	.apLayoutCreate   = Shader_Gizmo_GetPipelineLayoutCreate,
	.apGraphicsCreate = Shader_Gizmo_GetGraphicsPipelineCreate,
	.apShaderPush     = &gShaderPush_Debug,
};


CH_REGISTER_SHADER( gShaderCreate_Gizmo );

