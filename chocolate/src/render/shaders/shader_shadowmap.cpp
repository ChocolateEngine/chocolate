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


struct ShadowMap_Material
{
	int diffuse;
	u32 alphaTest;
};


static ShaderMaterialVarDesc gShadowMap_MaterialVars[] = {
	CH_SHADER_MATERIAL_VAR( ShadowMap_Material, diffuse, "Diffuse Texture", "" ),
	CH_SHADER_MATERIAL_VAR( ShadowMap_Material, alphaTest, "Alpha Testing", 0 ),
};


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

	// this should never be nullptr, but it sometimes is according to another shader, not sure if it's fixed or not
	if ( sPushData.apMaterialData )
	{
		bool alphaTest = sPushData.apMaterialData->vars[ 1 ].aInt;
		if ( alphaTest )
		{
			ch_handle_t texture = sPushData.apMaterialData->vars[ 0 ].aTexture;
			push.aAlbedo        = render->GetTextureIndex( texture );
		}
	}

	render->CmdPushConstants( cmd, sLayout, ShaderStage_Vertex | ShaderStage_Fragment, 0, sizeof( ShadowMap_Push ), &push );
}


ShaderCreate_t gShaderCreate_ShadowMap = {
	.apName             = "__shadow_map",
	.aStages            = ShaderStage_Vertex | ShaderStage_Fragment,
	.aBindPoint         = EPipelineBindPoint_Graphics,
	.aDynamicState      = EDynamicState_Viewport | EDynamicState_Scissor,
	.aVertexFormat      = VertexFormat_Position | VertexFormat_TexCoord,
	.aRenderPass        = ERenderPass_Shadow,
	.apInit             = nullptr,
	.apDestroy          = nullptr,
	.apLayoutCreate     = Shader_ShadowMap_GetPipelineLayoutCreate,
	.apGraphicsCreate   = Shader_ShadowMap_GetGraphicsPipelineCreate,
	.apShaderPush       = Shader_ShadowMap_PushConstants,

	.apMaterialVars     = gShadowMap_MaterialVars,
	.aMaterialVarCount  = CH_ARR_SIZE( gShadowMap_MaterialVars ),
	.aMaterialSize      = sizeof( ShadowMap_Material ),
	.aUseMaterialBuffer = false,
};


CH_REGISTER_SHADER( gShaderCreate_ShadowMap );

