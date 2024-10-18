#include "util.h"
#include "render/irender.h"
#include "graphics_int.h"


struct ShaderUnlit_Push
{
	alignas( 16 ) glm::mat4 aModelMatrix{};  // model matrix
	alignas( 16 ) int aViewport = 0;         // projection * view index
	int aDiffuse                = 0;         // aldedo
};


static void Shader_ShaderUnlit_GetPipelineLayoutCreate( PipelineLayoutCreate_t& srPipeline )
{
	srPipeline.aPushConstants.push_back( { ShaderStage_Vertex | ShaderStage_Fragment, 0, sizeof( ShaderUnlit_Push ) } );
}


static void Shader_ShaderUnlit_GetGraphicsPipelineCreate( GraphicsPipelineCreate_t& srGraphics )
{
	srGraphics.aShaderModules.push_back( { ShaderStage_Vertex, "shaders/unlit.vert.spv", "main" } );
	srGraphics.aShaderModules.push_back( { ShaderStage_Fragment, "shaders/unlit.frag.spv", "main" } );

	srGraphics.aColorBlendAttachments.push_back( { false } ); 

	srGraphics.aPrimTopology   = EPrimTopology_Tri;
	srGraphics.aDynamicState   = EDynamicState_Viewport | EDynamicState_Scissor;
	srGraphics.aCullMode       = ECullMode_Back;
}


static void Shader_ShaderUnlit_PushConstants( ch_handle_t cmd, ch_handle_t sLayout, const ShaderPushData_t& sPushData )
{
	PROF_SCOPE();

	ShaderUnlit_Push push;
	push.aModelMatrix = sPushData.apRenderable->aModelMatrix;
	push.aViewport    = sPushData.aViewportIndex;

	ch_handle_t mat        = sPushData.apRenderable->apMaterials[ sPushData.aSurfaceDraw.aSurface ];

	if ( mat == CH_INVALID_HANDLE )
		return;

	push.aDiffuse = gGraphics.Mat_GetTextureIndex( mat, "diffuse" );
	render->CmdPushConstants( cmd, sLayout, ShaderStage_Vertex | ShaderStage_Fragment, 0, sizeof( ShaderUnlit_Push ), &push );
}


ShaderCreate_t gShaderCreate_Unlit = {
	.apName           = "unlit",
	.aStages          = ShaderStage_Vertex | ShaderStage_Fragment,
	.aBindPoint       = EPipelineBindPoint_Graphics,
	.aDynamicState    = EDynamicState_Viewport | EDynamicState_Scissor,
	.aVertexFormat    = VertexFormat_Position | VertexFormat_TexCoord,
	.apInit           = nullptr,
	.apDestroy        = nullptr,
	.apLayoutCreate   = Shader_ShaderUnlit_GetPipelineLayoutCreate,
	.apGraphicsCreate = Shader_ShaderUnlit_GetGraphicsPipelineCreate,
	.apShaderPush     = Shader_ShaderUnlit_PushConstants,
};


// CH_REGISTER_SHADER( gShaderCreate_Unlit );

