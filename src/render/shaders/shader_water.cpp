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
};


struct Water_Material_t
{
	glm::vec3 aColor;
	u32       aNormalMap = 0;
};


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


static void Shader_Water_PushConstants( ch_handle_t cmd, ch_handle_t sLayout, const ShaderPushData_t& sPushData )
{
	PROF_SCOPE();

	Water_Push_t push;

	// push.aModelMatrix  = sPushData.apRenderable->aModelMatrix;
	push.aRenderable = sPushData.apRenderable->aIndex;
	push.aViewport   = sPushData.aViewportIndex;
	push.aColor      = gWaterColor;

	render->CmdPushConstants( cmd, sLayout, ShaderStage_Vertex | ShaderStage_Fragment, 0, sizeof( Water_Push_t ), &push );
}


ShaderCreate_t gShaderCreate_Water = {
	.apName           = "water",
	.aStages          = ShaderStage_Vertex | ShaderStage_Fragment,
	.aBindPoint       = EPipelineBindPoint_Graphics,
	.aDynamicState    = EDynamicState_Viewport | EDynamicState_Scissor,
	.aVertexFormat    = VertexFormat_Position | VertexFormat_Normal | VertexFormat_TexCoord,

	.apLayoutCreate   = Shader_Water_GetPipelineLayoutCreate,
	.apGraphicsCreate = Shader_Water_GetGraphicsPipelineCreate,
	.apShaderPush     = Shader_Water_PushConstants,

	.apBindings       = gWater_Bindings,
	.aBindingCount    = CH_ARR_SIZE( gWater_Bindings ),
};


CH_REGISTER_SHADER( gShaderCreate_Water );

