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
	alignas( 16 ) u32 aViewport   = 0;  // viewport index
	u32 aRenderable = 0;  // renderable index
};


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


static glm::vec4 AdjustColor( glm::vec4 baseColor, glm::vec4 darkenColor )
{
	return glm::mix( baseColor, darkenColor * 2.f, 0.5f );
}


static void Shader_Gizmo_PushConstants( ch_handle_t cmd, ch_handle_t sLayout, const ShaderPushData_t& sPushData )
{
	Gizmo_Push push{};
	push.aModelMatrix = sPushData.apRenderable->aModelMatrix;
	ch_handle_t mat        = sPushData.apRenderable->apMaterials[ sPushData.aSurfaceDraw.aSurface ];
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

	push.aRenderable = sPushData.aRenderableIndex;
	push.aViewport   = sPushData.aViewportIndex;

	render->CmdPushConstants( cmd, sLayout, ShaderStage_Vertex | ShaderStage_Fragment, 0, sizeof( Gizmo_Push ), &push );
}


ShaderCreate_t gShaderCreate_Gizmo = {
	.apName           = "gizmo",
	.aStages          = ShaderStage_Vertex | ShaderStage_Fragment,
	.aBindPoint       = EPipelineBindPoint_Graphics,
	.aDynamicState    = EDynamicState_Viewport | EDynamicState_Scissor,
	.aVertexFormat    = VertexFormat_Position,
	.apInit           = nullptr,
	.apDestroy        = nullptr,
	.apLayoutCreate   = Shader_Gizmo_GetPipelineLayoutCreate,
	.apGraphicsCreate = Shader_Gizmo_GetGraphicsPipelineCreate,
	.apShaderPush     = Shader_Gizmo_PushConstants,
};


CH_REGISTER_SHADER( gShaderCreate_Gizmo );

