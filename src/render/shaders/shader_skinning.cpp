#include "core/util.h"
#include "render/irender.h"
#include "graphics_int.h"


static CreateDescBinding_t gSkinning_Bindings[] = {
	{ EDescriptorType_StorageBuffer, ShaderStage_Compute, 0, CH_R_MAX_BLEND_SHAPE_WEIGHT_BUFFERS },  // blendWeights
	{ EDescriptorType_StorageBuffer, ShaderStage_Compute, 1, CH_R_MAX_BLEND_SHAPE_DATA_BUFFERS },  // blendData
};


static void Shader_ShaderSkinning_GetPipelineLayoutCreate( PipelineLayoutCreate_t& srPipeline )
{
	srPipeline.aPushConstants.push_back( { ShaderStage_Compute, 0, sizeof( ShaderSkinning_Push ) } );
}


static void Shader_ShaderSkinning_GetComputePipelineCreate( ComputePipelineCreate_t& srCreate )
{
	srCreate.aShaderModule.aModulePath = "shaders/skinning.comp.spv";
	srCreate.aShaderModule.apEntry     = "main";
	srCreate.aShaderModule.aStage      = ShaderStage_Compute;
}


#if 0
// this system does not work for compute shaders, all of them need unique data
static void Shader_ShaderSkinning_SetupPushData( ch_handle_t srRenderableHandle, Renderable_t* spDrawData )
{
	PROF_SCOPE();

	ShaderSkinning_Push& push  = gPushData[ CH_GET_HANDLE_INDEX( srRenderableHandle ) ];

	Model*               model = Graphics_GetModelData( spDrawData->aModel );

	if ( !model || !model->apVertexData )
		return;

	push.aVertexCount     = model->apVertexData->aIndices.empty() ? model->apVertexData->aCount : model->apVertexData->aIndices.size();
	push.aBlendShapeCount = model->apVertexData->aBlendShapeCount;
}


static void Shader_ShaderSkinning_PushConstants( ch_handle_t cmd, ch_handle_t sLayout, ch_handle_t srRenderableHandle, Renderable_t* spDrawData )
{
	PROF_SCOPE();

	ShaderSkinning_Push& push = gPushData.at( CH_GET_HANDLE_INDEX( srRenderableHandle ) );
	render->CmdPushConstants( cmd, sLayout, ShaderStage_Compute, 0, sizeof( ShaderSkinning_Push ), &push );
}
#endif


ShaderCreate_t gShaderCreate_Skinning = {
	.apName           = "__skinning",
	.aStages          = ShaderStage_Compute,
	.aBindPoint       = EPipelineBindPoint_Compute,
	.apInit           = nullptr,
	.apDestroy        = nullptr,
	.apLayoutCreate   = Shader_ShaderSkinning_GetPipelineLayoutCreate,
	.apComputeCreate  = Shader_ShaderSkinning_GetComputePipelineCreate,
	// .apShaderPushComp = &gShaderPush_Skinning,

	.apBindings       = gSkinning_Bindings,
	.aBindingCount    = CH_ARR_SIZE( gSkinning_Bindings ),
};


CH_REGISTER_SHADER( gShaderCreate_Skinning );

