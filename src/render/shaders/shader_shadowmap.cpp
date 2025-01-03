#include "core/util.h"
#include "render/irender.h"
#include "graphics_int.h"


struct ShadowMap_Push
{
	int aAlbedo     = 0;       // albedo texture index
	u32 aRenderable = 0;       // renderable index
	u32 aViewport   = 0;       // viewport index
	alignas( 16 ) glm::mat4 aModelMatrix{};  // model matrix
};


static int  gShadowViewInfoIndex = 0;


static void Shader_ShadowMap_GetPipelineLayoutCreate( PipelineLayoutCreate_t& srPipeline )
{
	srPipeline.aPushConstants.push_back( { ShaderStage_Vertex | ShaderStage_Fragment, 0, sizeof( ShadowMap_Push ) } );
}


static void Shader_ShadowMap_GetGraphicsPipelineCreate( GraphicsPipelineCreate_t& srGraphics )
{
	srGraphics.aShaderModules.push_back( { ShaderStage_Vertex, "shaders/shadow.vert.spv", "main" } );
	srGraphics.aShaderModules.push_back( { ShaderStage_Fragment, "shaders/shadow.frag.spv", "main" } );
	srGraphics.aColorBlendAttachments.push_back( { false } );
	srGraphics.aPrimTopology    = EPrimTopology_Tri;
	srGraphics.aDynamicState    = EDynamicState_Viewport | EDynamicState_Scissor | EDynamicState_DepthBias;
	srGraphics.aCullMode        = ECullMode_None;
	srGraphics.aDepthBiasEnable = true;
}


// blech
void Shader_ShadowMap_SetViewInfo( u32 sViewInfo )
{
	gShadowViewInfoIndex = Graphics_GetShaderSlot( gGraphicsData.aViewportSlots, sViewInfo );
}


static void Shader_ShadowMap_PushConstants( ch_handle_t cmd, ch_handle_t sLayout, const ShaderPushData_t& sPushData )
{
	PROF_SCOPE();

	ShadowMap_Push push;
	push.aModelMatrix    = sPushData.apRenderable->aModelMatrix;
	push.aRenderable     = sPushData.apRenderable->aIndex;
	push.aViewport       = gShadowViewInfoIndex;
	push.aAlbedo         = -1;

	ch_handle_t mat           = sPushData.apRenderable->apMaterials[ sPushData.aSurfaceDraw.aSurface ];
	if ( mat == CH_INVALID_HANDLE )
		return;

	ch_handle_t texture = gGraphics.Mat_GetTexture( mat, "diffuse" );

	if ( texture == CH_INVALID_HANDLE )
		return;

	bool alphaTest = gGraphics.Mat_GetBool( mat, "alphaTest" );

#if 0
	GraphicsFmt format   = render->GetTextureFormat( texture );

	// Check the texture format to see if it has an alpha channel
	switch ( format )
	{
		default:
			break;

		case GraphicsFmt::BC1_RGBA_SRGB_BLOCK:
		case GraphicsFmt::BC3_SRGB_BLOCK:
		case GraphicsFmt::BC7_SRGB_BLOCK:
			alphaTest = true;
	}
#endif

	if ( alphaTest )
		push.aAlbedo = render->GetTextureIndex( texture );

	render->CmdPushConstants( cmd, sLayout, ShaderStage_Vertex | ShaderStage_Fragment, 0, sizeof( ShadowMap_Push ), &push );
}


ShaderCreate_t gShaderCreate_ShadowMap = {
	.apName           = "__shadow_map",
	.aStages          = ShaderStage_Vertex | ShaderStage_Fragment,
	.aBindPoint       = EPipelineBindPoint_Graphics,
	.aDynamicState    = EDynamicState_Viewport | EDynamicState_Scissor,
	.aVertexFormat    = VertexFormat_Position | VertexFormat_TexCoord,
	.aRenderPass      = ERenderPass_Shadow,
	.apInit           = nullptr,
	.apDestroy        = nullptr,
	.apLayoutCreate   = Shader_ShadowMap_GetPipelineLayoutCreate,
	.apGraphicsCreate = Shader_ShadowMap_GetGraphicsPipelineCreate,
	.apShaderPush     = Shader_ShadowMap_PushConstants,
};


CH_REGISTER_SHADER( gShaderCreate_ShadowMap );

