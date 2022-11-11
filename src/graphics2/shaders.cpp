#include "core/platform.h"
#include "core/log.h"
#include "core/resources.hh"
#include "util.h"

#include "render/irender.h"
#include "render_vk.h"


struct ShaderVK
{
	VkPipeline          aPipeline       = nullptr;
	// VkPipelineLayout    aPipelineLayout = nullptr;
	VkPipelineBindPoint aBindPoint;
};


static ResourceList< ShaderVK >         gShaders;
static ResourceList< VkPipelineLayout > gPipelineLayouts;


CONVAR( r_sampled_textures, 0 );
CONVAR( r_line_thickness, 2 );


void VK_BindDescSets()
{
}


bool VK_BindShader( VkCommandBuffer c, Handle handle )
{
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
		return nullptr;
	}

	return *layout;
}


VkShaderModule VK_CreateShaderModule( u32* spByteCode, u32 sSize )
{
	VkShaderModuleCreateInfo createInfo{ VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO };
	createInfo.codeSize = sSize;
	createInfo.pCode    = spByteCode;

	VkShaderModule shaderModule;
	VK_CheckResult( vkCreateShaderModule( VK_GetDevice(), &createInfo, NULL, &shaderModule ), "Failed to create shader module!" );

	return shaderModule;
}


void VK_DestroyShaderModule( VkShaderModule shaderModule )
{
	if ( shaderModule )
		vkDestroyShaderModule( VK_GetDevice(), shaderModule, nullptr );
}


bool VK_CreatePipelineLayout( Handle& srHandle, PipelineLayoutCreate_t& srPipelineCreate )
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


void VK_GetShaderStageCreateInfo( const std::vector< ShaderModule_t >& shaderModules, std::vector< VkPipelineShaderStageCreateInfo >& shaderStages )
{
	for ( const auto& stage : shaderModules )
	{
		auto  fileData   = FileSys_ReadFile( stage.aModulePath );

		auto& stageInfo  = shaderStages.emplace_back( VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO );
		stageInfo.pName  = stage.apEntry;
		stageInfo.module = VK_CreateShaderModule( (u32*)fileData.data(), fileData.size() );

		if ( stage.aStage & ShaderStage_Vertex )
			stageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;

		else if ( stage.aStage & ShaderStage_Fragment )
			stageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;

		else if ( stage.aStage & ShaderStage_Compute )
			stageInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
	}
}


// UNFINISHED !!!!!
#if 0
Handle VK_CreateComputePipeline( ComputePipelineCreate_t& srPipelineCreate )
{
	std::vector< VkPipelineShaderStageCreateInfo > shaderStages;
	VK_GetShaderStageCreateInfo( srGraphicsCreate.aShaderModules, shaderStages );

	VkPipelineLayout pipelineLayout = *gPipelineLayouts.Get( srGraphicsCreate.aPipelineLayout );
}
#endif


bool VK_CreateGraphicsPipeline( Handle& srHandle, GraphicsPipelineCreate_t& srGraphicsCreate )
{
	VkPipelineLayout* layout = gPipelineLayouts.Get( srGraphicsCreate.aPipelineLayout );
	if ( layout == nullptr )
	{
		Log_Error( gLC_Render, "VK_CreateGraphicsPipeline(): Pipeline Layout not found!\n" );
		return InvalidHandle;
	}
	
	VkRenderPass renderPass = VK_GetRenderPass( srGraphicsCreate.aRenderPass );
	if ( renderPass == nullptr )
	{
		Log_Error( gLC_Render, "VK_CreateGraphicsPipeline(): RenderPass not found!\n" );
		return InvalidHandle;
	}

	std::vector< VkPipelineShaderStageCreateInfo > shaderStages;
	VK_GetShaderStageCreateInfo( srGraphicsCreate.aShaderModules, shaderStages );

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

	const auto& swapExtent = VK_GetSwapExtent();

	VkViewport  viewport{
		 .x        = 0.0f,
		 .y        = 0,
		 .width    = (float)swapExtent.width,
		 .height   = (float)swapExtent.height,
		 .minDepth = 0.0f,
		 .maxDepth = 1.0f
	};

	VkRect2D scissor{
		.offset = { 0, 0 },
		.extent = swapExtent,
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
	rasterizer.depthClampEnable        = VK_TRUE;
	rasterizer.rasterizerDiscardEnable = VK_FALSE;
	rasterizer.lineWidth               = r_line_thickness;
	rasterizer.polygonMode             = VK_POLYGON_MODE_FILL;
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
	multisampling.sampleShadingEnable   = r_sampled_textures;
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

	std::vector< VkDynamicState > dynamicStates;

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
	dynamicState.dynamicStateCount              = (u32)dynamicStates.size();
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

	if ( srHandle != InvalidHandle )
	{
		shader = gShaders.Get( srHandle );
		if ( !shader )
		{
			Log_Error( gLC_Render, "VK_CreateGraphicsPipeline(): Shader not found for recreation!\n" );
			return false;
		}
	}
	else
	{
		srHandle = gShaders.Create( &shader );
	}

	shader->aBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;

	// TODO: look into trying to make multiple pipelines at once
	VK_CheckResult( vkCreateGraphicsPipelines( VK_GetDevice(), VK_NULL_HANDLE, 1, &pipelineInfo, NULL, &shader->aPipeline ), "Failed to create graphics pipeline!" );

#ifdef _DEBUG
	if ( srGraphicsCreate.apName && pfnSetDebugUtilsObjectName )
	{
		// add a debug label onto it
		const VkDebugUtilsObjectNameInfoEXT objectInfo = {
			VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,  // sType
			NULL,                                                // pNext
			VK_OBJECT_TYPE_PIPELINE,                             // objectType
			(uint64_t)shader->aPipeline,                         // objectHandle
			srGraphicsCreate.apName,                             // pObjectName
		};

		pfnSetDebugUtilsObjectName( VK_GetDevice(), &objectInfo );
	}
#endif

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

