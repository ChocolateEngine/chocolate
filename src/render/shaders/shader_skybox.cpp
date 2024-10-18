#include "util.h"
#include "render/irender.h"
#include "graphics_int.h"


struct Skybox_Push
{
	glm::mat4 aModelMatrix{};   // model matrix
	int       aSky        = 0;  // sky texture index
	u32       aRenderable = 0;  // renderable index
	u32       aViewport   = 0;  // viewport index
};


static void Shader_Skybox_GetPipelineLayoutCreate( PipelineLayoutCreate_t& srPipeline )
{
	srPipeline.aPushConstants.push_back( { ShaderStage_Vertex | ShaderStage_Fragment, 0, sizeof( Skybox_Push ) } );
}


static void Shader_Skybox_GetGraphicsPipelineCreate( GraphicsPipelineCreate_t& srGraphics )
{
	srGraphics.aShaderModules.push_back( { ShaderStage_Vertex, "shaders/skybox.vert.spv", "main" } );
	srGraphics.aShaderModules.push_back( { ShaderStage_Fragment, "shaders/skybox.frag.spv", "main" } );

	srGraphics.aColorBlendAttachments.push_back( { true } );

	srGraphics.aPrimTopology = EPrimTopology_Tri;
	srGraphics.aDynamicState = EDynamicState_Viewport | EDynamicState_Scissor;
	srGraphics.aCullMode     = ECullMode_Back;
}


static void Shader_Skybox_PushConstants( ch_handle_t cmd, ch_handle_t sLayout, const ShaderPushData_t& sPushData )
{
	PROF_SCOPE();

	Skybox_Push push;

	push.aModelMatrix = sPushData.apRenderable->aModelMatrix;
	ch_handle_t mat        = sPushData.apRenderable->apMaterials[ sPushData.aSurfaceDraw.aSurface ];
	push.aSky         = gGraphics.Mat_GetTextureIndex( mat, "sky" );
	push.aRenderable  = sPushData.aRenderableIndex;
	push.aViewport    = sPushData.aViewportIndex;

	render->CmdPushConstants( cmd, sLayout, ShaderStage_Vertex | ShaderStage_Fragment, 0, sizeof( Skybox_Push ), &push );
}


ShaderCreate_t gShaderCreate_Skybox = {
	.apName           = "skybox",
	.aStages          = ShaderStage_Vertex | ShaderStage_Fragment,
	.aBindPoint       = EPipelineBindPoint_Graphics,
	.aDynamicState    = EDynamicState_Viewport | EDynamicState_Scissor,
	.aVertexFormat    = VertexFormat_Position,
	.apInit           = nullptr,
	.apDestroy        = nullptr,
	.apLayoutCreate   = Shader_Skybox_GetPipelineLayoutCreate,
	.apGraphicsCreate = Shader_Skybox_GetGraphicsPipelineCreate,
	.apShaderPush     = Shader_Skybox_PushConstants,
};


CH_REGISTER_SHADER( gShaderCreate_Skybox );

