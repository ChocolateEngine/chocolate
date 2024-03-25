#include "util.h"
#include "render/irender.h"
#include "graphics_int.h"


struct Skybox_Push
{
	alignas( 16 ) glm::mat4 aModelMatrix{};  // model matrix
	alignas( 16 ) int aSky = 0;              // sky texture index
	alignas( 16 ) u32 aRenderable = 0;       // renderable index
	alignas( 16 ) u32 aViewport = 0;         // viewport index
};


static std::unordered_map< SurfaceDraw_t*, Skybox_Push > gSkyboxPushData;


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


static void Shader_Skybox_ResetPushData()
{
	gSkyboxPushData.clear();
}


static void Shader_Skybox_SetupPushData( u32 sRenderableIndex, u32 sViewportIndex, Renderable_t* spModelDraw, SurfaceDraw_t& srDrawInfo, ShaderMaterialData* spMaterialData )
{
	PROF_SCOPE();

	Skybox_Push& push = gSkyboxPushData[ &srDrawInfo ];
	push.aModelMatrix = spModelDraw->aModelMatrix;

	Handle mat        = gGraphics.Model_GetMaterial( spModelDraw->aModel, srDrawInfo.aSurface );
	push.aSky         = gGraphics.Mat_GetTextureIndex( mat, "sky" );
	push.aRenderable  = sRenderableIndex;
	push.aViewport    = sViewportIndex;
}


static void Shader_Skybox_PushConstants( Handle cmd, Handle sLayout, SurfaceDraw_t& srDrawInfo )
{
	PROF_SCOPE();

	Skybox_Push& push = gSkyboxPushData.at( &srDrawInfo );
	render->CmdPushConstants( cmd, sLayout, ShaderStage_Vertex | ShaderStage_Fragment, 0, sizeof( Skybox_Push ), &push );
}


static IShaderPush gShaderPush_Skybox = {
	.apReset = Shader_Skybox_ResetPushData,
	.apSetup = Shader_Skybox_SetupPushData,
	.apPush  = Shader_Skybox_PushConstants,
};


ShaderCreate_t gShaderCreate_Skybox = {
	.apName           = "skybox",
	.aStages          = ShaderStage_Vertex | ShaderStage_Fragment,
	.aBindPoint       = EPipelineBindPoint_Graphics,
	.aFlags           = EShaderFlags_PushConstant,
	.aDynamicState    = EDynamicState_Viewport | EDynamicState_Scissor,
	.aVertexFormat    = VertexFormat_Position,
	.apInit           = nullptr,
	.apDestroy        = nullptr,
	.apLayoutCreate   = Shader_Skybox_GetPipelineLayoutCreate,
	.apGraphicsCreate = Shader_Skybox_GetGraphicsPipelineCreate,
	.apShaderPush     = &gShaderPush_Skybox,
};


CH_REGISTER_SHADER( gShaderCreate_Skybox );

