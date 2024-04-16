#include "graphics_int.h"
#include "render/irender.h"
#include "util.h"


constexpr const char*      gpFallbackNormalPath     = "materials/base/black.ktx";

constexpr u32              CH_WATER_MAX_MATERIALS = 2048;

static glm::vec3           gWaterColor              = { 90.f / 255.f, 188.f / 255.f, 216.f / 255.f };


static CreateDescBinding_t gWater_Bindings[]      = {
    { EDescriptorType_StorageBuffer, ShaderStage_Vertex | ShaderStage_Fragment, 0, CH_WATER_MAX_MATERIALS },
};


struct Water_Push_t
{
	glm::vec3 aColor;

	u32       aRenderable = 0;
	// u32       aMaterial   = 0;
	u32       aViewport   = 0;


	// Hack for GTX 1050 Ti until I figure this out properly
	// bool aPCF = false;
};


struct Water_Material_t
{
	u32 aNormalMap = 0;
};


static std::unordered_map< SurfaceDraw_t*, Water_Push_t > gPushData;


static void Shader_Water_GetPipelineLayoutCreate( PipelineLayoutCreate_t& srPipeline )
{
	// NOTE: maybe create the descriptor set layout for this shader here, then add it? idk

	srPipeline.aPushConstants.push_back( { ShaderStage_Vertex | ShaderStage_Fragment, 0, sizeof( Water_Push_t ) } );
}


static void Shader_Water_GetGraphicsPipelineCreate( GraphicsPipelineCreate_t& srGraphics )
{
	srGraphics.aShaderModules.push_back( { ShaderStage_Vertex, "shaders/water.vert.spv", "main" } );
	srGraphics.aShaderModules.push_back( { ShaderStage_Fragment, "shaders/water.frag.spv", "main" } );

	srGraphics.aColorBlendAttachments.push_back( { false } );

	srGraphics.aPrimTopology = EPrimTopology_Tri;
	srGraphics.aDynamicState = EDynamicState_Viewport | EDynamicState_Scissor;
	srGraphics.aCullMode     = ECullMode_Back;
}


// TODO: Move this push constant management into the shader system
// we should only need the SetupPushData function
static void Shader_Water_ResetPushData()
{
	gPushData.clear();
}


static void Shader_Water_SetupPushData( u32 sRenderableIndex, u32 sViewportIndex, Renderable_t* spRenderable, SurfaceDraw_t& srDrawInfo, ShaderMaterialData* spMaterialData )
{
	PROF_SCOPE();

	Water_Push_t& push = gPushData[ &srDrawInfo ];
	// push.aModelMatrix  = spRenderable->aModelMatrix;
	push.aRenderable   = spRenderable->aIndex;
	push.aViewport     = sViewportIndex;
	push.aColor        = gWaterColor;
}


static void Shader_Water_PushConstants( Handle cmd, Handle sLayout, SurfaceDraw_t& srDrawInfo )
{
	PROF_SCOPE();

	Water_Push_t& push = gPushData.at( &srDrawInfo );
	render->CmdPushConstants( cmd, sLayout, ShaderStage_Vertex | ShaderStage_Fragment, 0, sizeof( Water_Push_t ), &push );
}


static IShaderPush gShaderPush_Water = {
	.apReset = Shader_Water_ResetPushData,
	.apSetup = Shader_Water_SetupPushData,
	.apPush  = Shader_Water_PushConstants,
};


ShaderCreate_t gShaderCreate_Water = {
	.apName           = "water",
	.aStages          = ShaderStage_Vertex | ShaderStage_Fragment,
	.aBindPoint       = EPipelineBindPoint_Graphics,
	.aFlags           = EShaderFlags_PushConstant,
	.aDynamicState    = EDynamicState_Viewport | EDynamicState_Scissor,
	.aVertexFormat    = VertexFormat_Position | VertexFormat_Normal | VertexFormat_TexCoord,

	.apLayoutCreate   = Shader_Water_GetPipelineLayoutCreate,
	.apGraphicsCreate = Shader_Water_GetGraphicsPipelineCreate,
	.apShaderPush     = &gShaderPush_Water,

	.apBindings       = gWater_Bindings,
	.aBindingCount    = CH_ARR_SIZE( gWater_Bindings ),
};


CH_REGISTER_SHADER( gShaderCreate_Water );

