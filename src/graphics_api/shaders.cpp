#include "core/platform.h"
#include "core/log.h"
#include "util.h"

#include "render/irender.h"
#include "render_vk.h"


struct ShaderVK
{
	VkPipeline          aPipeline = VK_NULL_HANDLE;
	// VkPipelineLayout    aPipelineLayout = nullptr;
	VkPipelineBindPoint aBindPoint;

	// TODO: move this elsewhere, this is loaded in cache whenever we bind a shader, but is never used
	VkShaderModule*     apShaderModules = nullptr;
	u32                 aShaderModuleCount;
};


static ResourceList< ShaderVK >         gShaders;
static ResourceList< VkPipelineLayout > gPipelineLayouts;


CONVAR_BOOL_CMD( r_msaa_textures, 0, CVARF_ARCHIVE, "Enable/Disable MSAA on Textures, this is VERY EXPENSIVE! It enables VkPipelineMultisampleStateCreateInfo::sampleShadingEnable" )
{
	if ( !VK_UseMSAA() )
		return;

	VK_ResetAll( ERenderResetFlags_MSAA );
}


void VK_BindDescSets()
{
}


bool VK_BindShader( VkCommandBuffer c, Handle handle )
{
	PROF_SCOPE();

	ShaderVK* shader = gShaders.Get( handle );
	if ( !shader )
	{
		Log_Warn( gLC_Render, "VK_BindShader(): Shader not found!\n" );
		return false;
	}

	vkCmdBindPipeline( c, shader->aBindPoint, shader->aPipeline );
	return true;
}


VkPipelineLayout VK_GetPipelineLayout( Handle handle )
{
	VkPipelineLayout* layout = gPipelineLayouts.Get( handle );
	if ( layout == nullptr )
	{
		Log_Warn( gLC_Render, "VK_GetPipelineLayout(): Pipeline Layout not found!\n" );
		return VK_NULL_HANDLE;
	}

	return *layout;
}


bool VK_CreatePipelineLayout( ChHandle_t& srHandle, PipelineLayoutCreate_t& srPipelineCreate )
{
	std::vector< VkDescriptorSetLayout > layouts;

	for ( const auto& handle : srPipelineCreate.aLayouts )
	{
		VkDescriptorSetLayout layout = VK_GetDescLayout( handle );

		if ( layout == VK_NULL_HANDLE )
			return false;

		layouts.push_back( layout );
	}

	std::vector< VkPushConstantRange > pushConstantRanges;

	for ( const auto& pushConst : srPipelineCreate.aPushConstants )
	{
		VkPushConstantRange pushRange{};

		pushRange.stageFlags = VK_ToVkShaderStage( pushConst.aShaderStages );
		pushRange.offset = pushConst.aOffset;
		pushRange.size   = pushConst.aSize;

		pushConstantRanges.push_back( pushRange );
	}

	VkPipelineLayoutCreateInfo pipelineCreateInfo{ VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };
	pipelineCreateInfo.setLayoutCount         = layouts.size();
	pipelineCreateInfo.pSetLayouts            = layouts.data();
	pipelineCreateInfo.pushConstantRangeCount = pushConstantRanges.size();
	pipelineCreateInfo.pPushConstantRanges    = pushConstantRanges.data();

	VkPipelineLayout pipelineLayout;
	VK_CheckResult( vkCreatePipelineLayout( VK_GetDevice(), &pipelineCreateInfo, NULL, &pipelineLayout ), "Failed to create pipeline layout" );

	if ( srHandle != InvalidHandle )
		return gPipelineLayouts.Update( srHandle, pipelineLayout );

	srHandle = gPipelineLayouts.Add( pipelineLayout );
	return true;
}


bool VK_GetShaderStageCreateInfo( VkPipelineShaderStageCreateInfo* spStageCreate, ShaderModule_t* spShaderModules, u32 sStageCount, VkShaderModule* vkShaderModules )
{
	vkShaderModules = ch_malloc< VkShaderModule >( sStageCount );

	for ( u32 i = 0; i < sStageCount; i++ )
	{
		ShaderModule_t& stage   = spShaderModules[ i ];

		ch_string_auto  absPath = FileSys_FindFile( stage.aModulePath );
		if ( !absPath.data )
		{
			Log_ErrorF( gLC_Render, "Failed to find shader: \"%s\"\n", stage.aModulePath );
			ch_free( vkShaderModules );
			return false;
		}

		ch_string_auto fileData = FileSys_ReadFile( absPath.data, absPath.size );

		if ( !fileData.data )
		{
			Log_ErrorF( gLC_Render, "Shader file is empty: \"%s\"\n", stage.aModulePath );
			ch_free( vkShaderModules );
			return false;
		}

		auto& stageInfo  = spStageCreate[ i ];
		stageInfo.sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		stageInfo.pName  = stage.apEntry;

		VkShaderModuleCreateInfo createInfo{ VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO };
		createInfo.codeSize = fileData.size;
		createInfo.pCode    = (u32*)fileData.data;

		if ( !VK_CheckResultE( vkCreateShaderModule( VK_GetDevice(), &createInfo, NULL, &stageInfo.module ), "Failed to create shader module!" ) )
		{
			ch_free( vkShaderModules );
			return false;
		}

		vkShaderModules[ i ] = stageInfo.module;

		if ( stage.aStage & ShaderStage_Vertex )
			stageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;

		else if ( stage.aStage & ShaderStage_Fragment )
			stageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;

		else if ( stage.aStage & ShaderStage_Compute )
			stageInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
	}

	return true;
}


bool VK_CreateComputePipeline( ChHandle_t& srHandle, ComputePipelineCreate_t& srPipelineCreate )
{
	VkPipelineLayout* layout = gPipelineLayouts.Get( srPipelineCreate.aPipelineLayout );
	if ( layout == nullptr )
	{
		Log_ErrorF( gLC_Render, "VK_CreateGraphicsPipeline(): Pipeline Layout not found: \"%s\"\n", srPipelineCreate.apName );
		return false;
	}

	VkPipelineShaderStageCreateInfo shaderStageCreate{};
	VkShaderModule*                 shaderModules = nullptr;
	if ( !VK_GetShaderStageCreateInfo( &shaderStageCreate, &srPipelineCreate.aShaderModule, 1, shaderModules ) )
	{
		Log_ErrorF( gLC_Render, "VK_CreateGraphicsPipeline(): Failed to create shader stage info: \"%s\"\n", srPipelineCreate.apName );
		return false;
	}

	//	Combine all the objects above into one parameter for graphics pipeline creation
	VkComputePipelineCreateInfo pipelineInfo{ VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO };
	pipelineInfo.flags               = 0;
	pipelineInfo.stage               = shaderStageCreate;
	pipelineInfo.layout              = *layout;
	pipelineInfo.basePipelineHandle  = VK_NULL_HANDLE;  // Optional, very important for later when making new pipelines. It is less expensive to reference an existing similar pipeline
	pipelineInfo.basePipelineIndex   = -1;              // Optional

	ShaderVK* shader                 = nullptr;

	if ( srHandle != CH_INVALID_HANDLE )
	{
		shader = gShaders.Get( srHandle );
		if ( !shader )
		{
			Log_ErrorF( gLC_Render, "VK_CreateGraphicsPipeline(): Shader not found for recreation: \"%s\"\n", srPipelineCreate.apName );
			return false;
		}

		if ( shader->aShaderModuleCount )
		{
			// was this already destroyed?
			// for ( u32 i = 0; i < shader->aShaderModuleCount; i++ )
			// 	vkDestroyShaderModule( VK_GetDevice(), shader->apShaderModules[ i ], nullptr );

			ch_free( shader->apShaderModules );
			shader->apShaderModules    = nullptr;
			shader->aShaderModuleCount = 0;
		}
	}
	else
	{
		srHandle = gShaders.Create( &shader );
	}

	shader->aBindPoint         = VK_PIPELINE_BIND_POINT_COMPUTE;
	shader->apShaderModules    = shaderModules;
	shader->aShaderModuleCount = 1;

	// TODO: look into trying to make multiple pipelines at once
	VK_CheckResultF(
	  vkCreateComputePipelines( VK_GetDevice(), VK_NULL_HANDLE, 1, &pipelineInfo, NULL, &shader->aPipeline ),
	  "Failed to create graphics pipeline for shader: \"%s\"", srPipelineCreate.apName );

	VK_SetObjectNameEx( VK_OBJECT_TYPE_PIPELINE, (u64)shader->aPipeline, srPipelineCreate.apName, "Compute Pipeline" );

	return true;
}


bool VK_CreateGraphicsPipeline( ChHandle_t& srHandle, GraphicsPipelineCreate_t& srGraphicsCreate )
{
	VkPipelineLayout* layout = gPipelineLayouts.Get( srGraphicsCreate.aPipelineLayout );
	if ( layout == nullptr )
	{
		Log_ErrorF( gLC_Render, "VK_CreateGraphicsPipeline(): Pipeline Layout not found: \"%s\"\n", srGraphicsCreate.apName );
		return false;
	}
	
	VkRenderPass renderPass = VK_GetRenderPass( srGraphicsCreate.aRenderPass );
	if ( renderPass == VK_NULL_HANDLE )
	{
		Log_ErrorF( gLC_Render, "VK_CreateGraphicsPipeline(): RenderPass not found: \"%s\"\n", srGraphicsCreate.apName );
		return false;
	}

	std::vector< VkPipelineShaderStageCreateInfo > shaderStages;
	shaderStages.resize( srGraphicsCreate.aShaderModules.size() );
	VkShaderModule* vkShaderModules = nullptr;
	if ( !VK_GetShaderStageCreateInfo( shaderStages.data(), srGraphicsCreate.aShaderModules.data(), srGraphicsCreate.aShaderModules.size(), vkShaderModules ) )
	{
		Log_ErrorF( gLC_Render, "VK_CreateGraphicsPipeline(): Failed to create shader stage info: \"%s\"\n", srGraphicsCreate.apName );
		return false;
	}

	ChVector< VkVertexInputBindingDescription >   bindingDescriptions( srGraphicsCreate.aVertexBindings.size() );
	ChVector< VkVertexInputAttributeDescription > attributeDescriptions( srGraphicsCreate.aVertexAttributes.size() );

	for ( size_t i = 0; i < srGraphicsCreate.aVertexBindings.size(); i++ )
	{
		auto& vkBinding   = bindingDescriptions[ i ];
		auto& binding     = srGraphicsCreate.aVertexBindings[ i ];

		vkBinding.binding   = binding.aBinding;
		vkBinding.stride    = binding.aStride;
		vkBinding.inputRate = binding.aIsInstanced ? VK_VERTEX_INPUT_RATE_INSTANCE : VK_VERTEX_INPUT_RATE_VERTEX;
	}

	for ( size_t i = 0; i < srGraphicsCreate.aVertexAttributes.size(); i++ )
	{
		auto& vkAtrib = attributeDescriptions[ i ];
		auto& attrib     = srGraphicsCreate.aVertexAttributes[ i ];

		vkAtrib.location = attrib.aLocation;
		vkAtrib.binding  = attrib.aBinding;
		vkAtrib.format   = VK_ToVkFormat( attrib.aFormat );
		vkAtrib.offset   = attrib.aOfset;
	}

	VkPipelineVertexInputStateCreateInfo vertexInput{ VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO };
	vertexInput.vertexBindingDescriptionCount   = bindingDescriptions.size();
	vertexInput.pVertexBindingDescriptions      = bindingDescriptions.data();
	vertexInput.vertexAttributeDescriptionCount = attributeDescriptions.size();
	vertexInput.pVertexAttributeDescriptions    = attributeDescriptions.data();

	VkPipelineInputAssemblyStateCreateInfo inputAssembly{ VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO };
	inputAssembly.primitiveRestartEnable = VK_FALSE;

	switch ( srGraphicsCreate.aPrimTopology )
	{
		default:
		case EPrimTopology_Tri:
			inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
			break;

		case EPrimTopology_Line:
			inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
			break;

		case EPrimTopology_Point:
			inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
			break;
	}

	// TODO: AAAAAAAAAAAAAAAA WHY DO I NEED TO DO THIS
	// placeholder for like 85 years, all shaders use dynamic viewport and scissor anyway
	VkViewport  viewport{
		 .x        = 0.0f,
		 .y        = 0,
		 .width    = 1280.f,
		 .height   = 720.f,
		 .minDepth = 0.0f,
		 .maxDepth = 1.0f
	};

	VkRect2D scissor{
		.offset = { 0, 0 },
		.extent = { 1280, 720 },
	};

	VkPipelineViewportStateCreateInfo viewportInfo{
		.sType         = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
		.viewportCount = 1,
		.pViewports    = &viewport,
		.scissorCount  = 1,
		.pScissors     = &scissor
	};

	// create rasterizer
	VkPipelineRasterizationStateCreateInfo rasterizer{ VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO };
	rasterizer.depthClampEnable        = VK_FALSE;
	rasterizer.rasterizerDiscardEnable = VK_FALSE;
	rasterizer.lineWidth               = 1.f;
	rasterizer.polygonMode             = srGraphicsCreate.aLineMode ? VK_POLYGON_MODE_LINE : VK_POLYGON_MODE_FILL;
	rasterizer.frontFace               = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	rasterizer.depthBiasEnable         = srGraphicsCreate.aDepthBiasEnable;
	rasterizer.depthBiasConstantFactor = 0.0f;  // Optional
	rasterizer.depthBiasClamp          = 0.0f;  // Optional
	rasterizer.depthBiasSlopeFactor    = 0.0f;  // Optional

	switch ( srGraphicsCreate.aCullMode )
	{
		default:
		case ECullMode_None:
			rasterizer.cullMode = VK_CULL_MODE_NONE;
			break;

		case ECullMode_Front:
			rasterizer.cullMode = VK_CULL_MODE_FRONT_BIT;
			break;

		case ECullMode_Back:
			rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
			break;
	}

	// get renderpass info to see if it uses msaa or not
	RenderPassInfoVK*                    renderPassInfo = VK_GetRenderPassInfo( renderPass );

	// Performs anti-aliasing
	VkPipelineMultisampleStateCreateInfo multisampling{ VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO };
	multisampling.sampleShadingEnable   = r_msaa_textures;
	multisampling.rasterizationSamples  = renderPassInfo->aUsesMSAA ? VK_GetMSAASamples() : VK_SAMPLE_COUNT_1_BIT;
	multisampling.minSampleShading      = 1.0f;      // Optional
	multisampling.pSampleMask           = NULL;      // Optional
	multisampling.alphaToCoverageEnable = VK_FALSE;  // Optional
	multisampling.alphaToOneEnable      = VK_FALSE;  // Optional

	std::vector< VkPipelineColorBlendAttachmentState > colorBlendAttachments( srGraphicsCreate.aColorBlendAttachments.size() );

	for ( size_t i = 0; i < srGraphicsCreate.aColorBlendAttachments.size(); i++ )
	{
		// colorBlendAttachments[ i ].colorWriteMask      = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
		colorBlendAttachments[ i ].colorWriteMask      = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT;
		colorBlendAttachments[ i ].blendEnable         = srGraphicsCreate.aColorBlendAttachments[ i ].aBlendEnable;
		// colorBlendAttachments[ i ].srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
		colorBlendAttachments[ i ].srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
		colorBlendAttachments[ i ].dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
		colorBlendAttachments[ i ].colorBlendOp        = VK_BLEND_OP_ADD;
		colorBlendAttachments[ i ].srcAlphaBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
		colorBlendAttachments[ i ].dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
		colorBlendAttachments[ i ].alphaBlendOp        = VK_BLEND_OP_SUBTRACT;

		// colorBlendAttachments[ i ].srcColorBlendFactor = VK_BLEND_FACTOR_ZERO;
		// colorBlendAttachments[ i ].dstColorBlendFactor = VK_BLEND_FACTOR_SRC_COLOR;
		// colorBlendAttachments[ i ].colorBlendOp        = VK_BLEND_OP_ADD;
		// 
		// colorBlendAttachments[ i ].srcAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
		// colorBlendAttachments[ i ].dstAlphaBlendFactor = VK_BLEND_FACTOR_SRC_COLOR;
		// colorBlendAttachments[ i ].alphaBlendOp        = VK_BLEND_OP_ADD;
	}

	VkPipelineDepthStencilStateCreateInfo depthStencil{ VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO };
	depthStencil.depthTestEnable       = VK_TRUE;
	depthStencil.depthWriteEnable      = VK_TRUE;
	// depthStencil.depthCompareOp        = VK_COMPARE_OP_LESS;
	depthStencil.depthCompareOp        = VK_COMPARE_OP_LESS_OR_EQUAL;
	depthStencil.depthBoundsTestEnable = VK_FALSE;
	depthStencil.minDepthBounds        = 0.0f;  // Optional
	depthStencil.maxDepthBounds        = 1.0f;  // Optional
	depthStencil.stencilTestEnable     = VK_FALSE;
	depthStencil.front                 = {};  // Optional
	depthStencil.back                  = {};  // Optional

	VkPipelineColorBlendStateCreateInfo colorBlending{ VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO };
	colorBlending.logicOpEnable       = VK_FALSE;
	colorBlending.logicOp             = VK_LOGIC_OP_COPY;  // Optional
	colorBlending.attachmentCount     = static_cast< u32 >( colorBlendAttachments.size() );
	colorBlending.pAttachments        = colorBlendAttachments.data();
	colorBlending.blendConstants[ 0 ] = 0.0f;  // Optional
	colorBlending.blendConstants[ 1 ] = 0.0f;  // Optional
	colorBlending.blendConstants[ 2 ] = 0.0f;  // Optional
	colorBlending.blendConstants[ 3 ] = 0.0f;  // Optional

	ChVector< VkDynamicState > dynamicStates;

	if ( srGraphicsCreate.aDynamicState & EDynamicState_Viewport )
		dynamicStates.push_back( VK_DYNAMIC_STATE_VIEWPORT );

	if ( srGraphicsCreate.aDynamicState & EDynamicState_Scissor )
		dynamicStates.push_back( VK_DYNAMIC_STATE_SCISSOR );

	if ( srGraphicsCreate.aDynamicState & EDynamicState_LineWidth )
		dynamicStates.push_back( VK_DYNAMIC_STATE_LINE_WIDTH );

	if ( srGraphicsCreate.aDynamicState & EDynamicState_DepthBias )
		dynamicStates.push_back( VK_DYNAMIC_STATE_DEPTH_BIAS );

	if ( srGraphicsCreate.aDynamicState & EDynamicState_DepthBounds )
		dynamicStates.push_back( VK_DYNAMIC_STATE_DEPTH_BOUNDS );

	if ( srGraphicsCreate.aDynamicState & EDynamicState_BlendConstants )
		dynamicStates.push_back( VK_DYNAMIC_STATE_BLEND_CONSTANTS );

	if ( srGraphicsCreate.aDynamicState & EDynamicState_StencilCompareMask )
		dynamicStates.push_back( VK_DYNAMIC_STATE_STENCIL_COMPARE_MASK );

	if ( srGraphicsCreate.aDynamicState & EDynamicState_StencilWriteMask )
		dynamicStates.push_back( VK_DYNAMIC_STATE_STENCIL_WRITE_MASK );

	if ( srGraphicsCreate.aDynamicState & EDynamicState_StencilReference )
		dynamicStates.push_back( VK_DYNAMIC_STATE_STENCIL_REFERENCE );

	VkPipelineDynamicStateCreateInfo dynamicState{ VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO };
	dynamicState.dynamicStateCount              = dynamicStates.size();
	dynamicState.pDynamicStates                 = dynamicStates.data();

	//	Combine all the objects above into one parameter for graphics pipeline creation
	VkGraphicsPipelineCreateInfo pipelineInfo{ VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO };
	pipelineInfo.stageCount          = (u32)shaderStages.size();
	pipelineInfo.pStages             = shaderStages.data();
	pipelineInfo.pVertexInputState   = &vertexInput;
	pipelineInfo.pInputAssemblyState = &inputAssembly;
	pipelineInfo.pViewportState      = &viewportInfo;
	pipelineInfo.pRasterizationState = &rasterizer;
	pipelineInfo.pMultisampleState   = &multisampling;
	pipelineInfo.pDepthStencilState  = &depthStencil;
	pipelineInfo.pColorBlendState    = &colorBlending;
	pipelineInfo.pDynamicState       = &dynamicState;
	pipelineInfo.layout              = *layout;
	pipelineInfo.renderPass          = renderPass;
	pipelineInfo.subpass             = 0;
	pipelineInfo.basePipelineHandle  = VK_NULL_HANDLE;  // Optional, very important for later when making new pipelines. It is less expensive to reference an existing similar pipeline
	pipelineInfo.basePipelineIndex   = -1;              // Optional

	ShaderVK* shader                 = nullptr;

	if ( srHandle != CH_INVALID_HANDLE )
	{
		shader = gShaders.Get( srHandle );
		if ( !shader )
		{
			Log_ErrorF( gLC_Render, "VK_CreateGraphicsPipeline(): Shader not found for recreation: \"%s\"\n", srGraphicsCreate.apName );
			return false;
		}

		if ( shader->aShaderModuleCount )
		{
			// was this already destroyed?
			// for ( u32 i = 0; i < shader->aShaderModuleCount; i++ )
			// 	vkDestroyShaderModule( VK_GetDevice(), shader->apShaderModules[ i ], nullptr );

			ch_free( shader->apShaderModules );
			shader->apShaderModules    = nullptr;
			shader->aShaderModuleCount = 0;
		}
	}
	else
	{
		srHandle = gShaders.Create( &shader );
	}

	shader->aBindPoint         = VK_PIPELINE_BIND_POINT_GRAPHICS;
	shader->apShaderModules    = vkShaderModules;
	shader->aShaderModuleCount = srGraphicsCreate.aShaderModules.size();

	// TODO: look into trying to make multiple pipelines at once
	VK_CheckResultF(
	  vkCreateGraphicsPipelines( VK_GetDevice(), VK_NULL_HANDLE, 1, &pipelineInfo, NULL, &shader->aPipeline ),
	  "Failed to create graphics pipeline for shader: \"%s\"", srGraphicsCreate.apName );

	VK_SetObjectNameEx( VK_OBJECT_TYPE_PIPELINE, (u64)shader->aPipeline, srGraphicsCreate.apName, "Graphics Pipeline" );

	return true;
}


void VK_DestroyPipeline( Handle sPipeline )
{
	ShaderVK* shader = gShaders.Get( sPipeline );
	if ( !shader )
	{
		Log_Error( gLC_Render, "VK_DestroyPipeline(): Shader not found!\n" );
		return;
	}

	if ( shader->aShaderModuleCount )
	{
		for ( u32 i = 0; i < shader->aShaderModuleCount; i++ )
			vkDestroyShaderModule( VK_GetDevice(), shader->apShaderModules[ i ], nullptr );

		ch_free( shader->apShaderModules );
		shader->apShaderModules    = nullptr;
		shader->aShaderModuleCount = 0;
	}
	
	vkDestroyPipeline( VK_GetDevice(), shader->aPipeline, nullptr );

	gShaders.Remove( sPipeline );
}


void VK_DestroyPipelineLayout( Handle sPipeline )
{
	VkPipelineLayout* layout = gPipelineLayouts.Get( sPipeline );
	if ( layout == nullptr )
	{
		Log_Error( gLC_Render, "VK_DestroyPipelineLayout(): Pipeline Layout not found!\n" );
		return;
	}

	vkDestroyPipelineLayout( VK_GetDevice(), *layout, nullptr );
	gPipelineLayouts.Remove( sPipeline );
}


void VK_DestroyShaders()
{
	// destroy all shaders
	for ( u32 i = 0; i < gShaders.GetHandleCount(); i++ )
	{
		ShaderVK* shader = nullptr;
		if ( !gShaders.GetByIndex( i, &shader ) )
			continue;

		if ( shader->aPipeline )
			vkDestroyPipeline( VK_GetDevice(), shader->aPipeline, nullptr );
	}

	// destroy all shaders
	for ( u32 i = 0; i < gPipelineLayouts.GetHandleCount(); i++ )
	{
		VkPipelineLayout* layout = nullptr;
		if ( !gPipelineLayouts.GetByIndex( i, &layout ) )
			continue;

		vkDestroyPipelineLayout( VK_GetDevice(), *layout, nullptr );
	}

	gShaders.clear();
	gPipelineLayouts.clear();
}

