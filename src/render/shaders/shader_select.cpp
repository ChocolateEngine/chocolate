#include "graphics_int.h"

constexpr ShaderStage      gSelectShaderStage = ShaderStage_Vertex | ShaderStage_Fragment;

static CreateDescBinding_t gSelect_Bindings[] = {
	{ EDescriptorType_StorageBuffer, gSelectShaderStage, 0, 1 },  // outputData
};


static void Shader_Select_GetPipelineLayoutCreate( PipelineLayoutCreate_t& srPipeline )
{
	srPipeline.aPushConstants.push_back( { gSelectShaderStage, 0, sizeof( ShaderSelect_Push ) } );
}


static void Shader_Select_GetGraphicsPipelineCreate( GraphicsPipelineCreate_t& srGraphics )
{
	srGraphics.aShaderModules.push_back( { ShaderStage_Vertex, "shaders/select.vert.spv", "main" } );
	srGraphics.aShaderModules.push_back( { ShaderStage_Fragment, "shaders/select.frag.spv", "main" } );

	srGraphics.aColorBlendAttachments.push_back( { false } );

	srGraphics.aPrimTopology = EPrimTopology_Tri;
	srGraphics.aDynamicState = EDynamicState_Viewport | EDynamicState_Scissor;
	srGraphics.aCullMode     = ECullMode_None;
}


static void Shader_SelectResult_GetPipelineLayoutCreate( PipelineLayoutCreate_t& srPipeline )
{
	srPipeline.aPushConstants.push_back( { ShaderStage_Compute, 0, sizeof( ShaderSelect_Push ) } );
}


static void Shader_SelectResult_GetComputePipelineCreate( ComputePipelineCreate_t& srCreate )
{
	srCreate.aShaderModule.aModulePath = "shaders/select.comp.spv";
	srCreate.aShaderModule.apEntry     = "main";
	srCreate.aShaderModule.aStage      = gSelectShaderStage;
}


static void Shader_Select_PushConstants( ch_handle_t cmd, ch_handle_t sLayout, const ShaderPushData_t& sPushData )
{
	PROF_SCOPE();

	// Get the color we need
	u8 color[ 3 ];
	for ( SelectionRenderable& selectRenderable : gRenderOld.aSelectionRenderables )
	{
		if ( selectRenderable.renderable != sPushData.aSurfaceDraw.aRenderable )
			continue;

		if ( selectRenderable.colors.size() < sPushData.aSurfaceDraw.aSurface )
		{
			Log_ErrorF( "No Material Color for material %d!\n", sPushData.aSurfaceDraw.aSurface );
		}
		else
		{
			u32 colorSelect = selectRenderable.colors[ sPushData.aSurfaceDraw.aSurface ];

			color[ 0 ]      = ( colorSelect >> 16 ) & 0xFF;
			color[ 1 ]      = ( colorSelect >> 8 ) & 0xFF;
			color[ 2 ]      = colorSelect & 0xFF;
		}

		break;
	}

	ShaderSelect_Push push;
	push.aViewport           = Graphics_GetShaderSlot( gGraphicsData.aViewportSlots, gRenderOld.aSelectionViewport );
	push.aRenderable         = sPushData.apRenderable->aIndex;
	push.color.x            = color[ 0 ] / 255.f;
	push.color.y            = color[ 1 ] / 255.f;
	push.color.z            = color[ 2 ] / 255.f;
	push.aDiffuse            = -1;

	ch_handle_t mat           = sPushData.apRenderable->apMaterials[ sPushData.aSurfaceDraw.aSurface ];

	if ( mat != CH_INVALID_HANDLE )
	{
		ch_handle_t texture = gGraphics.Mat_GetTexture( mat, "diffuse" );

		if ( texture != CH_INVALID_HANDLE )
		{
			bool alphaTest = gGraphics.Mat_GetBool( mat, "alphaTest" );

			if ( alphaTest )
				push.aDiffuse = render->GetTextureIndex( texture );
		}
	}

	render->CmdPushConstants( cmd, sLayout, ShaderStage_Vertex | ShaderStage_Fragment, 0, sizeof( ShaderSelect_Push ), &push );
}


ShaderCreate_t gShaderCreate_Select = {
	.apName           = CH_SHADER_NAME_SELECT,
	.aStages          = gSelectShaderStage,
	.aBindPoint       = EPipelineBindPoint_Graphics,
	.aDynamicState    = EDynamicState_Viewport | EDynamicState_Scissor,
	.aRenderPass      = ERenderPass_Select,
	.apLayoutCreate   = Shader_Select_GetPipelineLayoutCreate,
	.apGraphicsCreate = Shader_Select_GetGraphicsPipelineCreate,
	.apShaderPush     = Shader_Select_PushConstants,
};


ShaderCreate_t gShaderCreate_SelectResult = {
	.apName          = CH_SHADER_NAME_SELECT_RESULT,
	.aStages         = gSelectShaderStage,
	.aBindPoint      = EPipelineBindPoint_Graphics,
	.apLayoutCreate  = Shader_SelectResult_GetPipelineLayoutCreate,
	.apComputeCreate = Shader_SelectResult_GetComputePipelineCreate,
	.apBindings      = gSelect_Bindings,
	.aBindingCount   = CH_ARR_SIZE( gSelect_Bindings ),
};


CH_REGISTER_SHADER( gShaderCreate_Select );
// CH_REGISTER_SHADER( gShaderCreate_SelectResult );

