#include "core/platform.h"
#include "core/log.h"
#include "core/resources.hh"
#include "util.h"

#include "render/irender.h"
#include "render_vk.h"


struct ShaderVK
{
	VkPipeline          aPipeline       = nullptr;
	VkPipelineLayout    aPipelineLayout = nullptr;
	VkPipelineBindPoint aBindPoint;
};


static ResourceList< ShaderVK >         gShaders;
static ResourceList< VkPipelineLayout > gPipelineLayouts;


void VK_BindDescSets()
{
}


void VK_BindShader( VkCommandBuffer c, Handle handle )
{
	ShaderVK* shader = gShaders.Get( handle );
	if ( !shader )
	{
		Log_Warn( gLC_Render, "VK_BindShader(): Shader not found!\n" );
		return;
	}

	vkCmdBindPipeline( c, shader->aBindPoint, shader->aPipeline );
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


Handle VK_CreatePipelineLayout( PipelineLayoutCreate_t& srPipelineCreate )
{
	std::vector< VkDescriptorSetLayout > layouts;

	if ( srPipelineCreate.aLayouts & EDescriptorLayout_Image )
		layouts.push_back( VK_GetImageLayout() );

	if ( srPipelineCreate.aLayouts & EDescriptorLayout_ImageStorage )
		layouts.push_back( VK_GetImageStorageLayout() );

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
	// Handle handle = gPipelineLayouts.Add( &pipelineLayout );
	Handle handle = gPipelineLayouts.Add( pipelineLayout );

	return handle;
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


Handle VK_CreateGraphicsPipeline( GraphicsPipelineCreate_t& srGraphicsCreate )
{
	std::vector< VkPipelineShaderStageCreateInfo > shaderStages;
	VK_GetShaderStageCreateInfo( srGraphicsCreate.aShaderModules, shaderStages );

	std::vector< VkVertexInputBindingDescription > bindingDescriptions;
	std::vector< VkVertexInputAttributeDescription > attributeDescriptions;

	for ( const auto& binding : srGraphicsCreate.aVertexBindings )
	{
		bindingDescriptions.emplace_back(
			binding.aBinding,
			binding.aStride,
			binding.aIsInstanced ? VK_VERTEX_INPUT_RATE_INSTANCE : VK_VERTEX_INPUT_RATE_VERTEX
		);
	}

	for ( const auto& attrib : srGraphicsCreate.aVertexAttributes )
	{
		attributeDescriptions.emplace_back(
			attrib.aLocation,
			attrib.aBinding,
			VK_ToVkFormat( attrib.aFormat ),
			attrib.aOfset
		);
	}

	VkPipelineVertexInputStateCreateInfo vertexInput{ VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO };
	vertexInput.vertexBindingDescriptionCount   = (u32)bindingDescriptions.size();
	vertexInput.pVertexBindingDescriptions      = bindingDescriptions.data();
	vertexInput.vertexAttributeDescriptionCount = (u32)attributeDescriptions.size();
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
	rasterizer.depthClampEnable        = VK_FALSE;
	rasterizer.rasterizerDiscardEnable = VK_FALSE;
	rasterizer.lineWidth               = 1.0f;
	rasterizer.polygonMode             = VK_POLYGON_MODE_FILL;
	rasterizer.frontFace               = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	rasterizer.depthBiasEnable         = VK_FALSE;
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

	//	Performs anti-aliasing
	VkPipelineMultisampleStateCreateInfo multisampling{ VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO };
	multisampling.sampleShadingEnable   = VK_FALSE;
	multisampling.rasterizationSamples  = VK_GetMSAASamples();
	multisampling.minSampleShading      = 1.0f;      // Optional
	multisampling.pSampleMask           = NULL;      // Optional
	multisampling.alphaToCoverageEnable = VK_FALSE;  // Optional
	multisampling.alphaToOneEnable      = VK_FALSE;  // Optional

	VkPipelineColorBlendAttachmentState colorBlendAttachment{};
	colorBlendAttachment.colorWriteMask      = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	colorBlendAttachment.blendEnable         = VK_TRUE;
	colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
	colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	colorBlendAttachment.colorBlendOp        = VK_BLEND_OP_ADD;
	colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	colorBlendAttachment.alphaBlendOp        = VK_BLEND_OP_ADD;

	VkPipelineDepthStencilStateCreateInfo depthStencil{ VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO };
	depthStencil.depthTestEnable       = VK_TRUE;
	depthStencil.depthWriteEnable      = VK_TRUE;
	depthStencil.depthCompareOp        = VK_COMPARE_OP_LESS;
	depthStencil.depthBoundsTestEnable = VK_FALSE;
	depthStencil.minDepthBounds        = 0.0f;  // Optional
	depthStencil.maxDepthBounds        = 1.0f;  // Optional
	depthStencil.stencilTestEnable     = VK_FALSE;
	depthStencil.front                 = {};  // Optional
	depthStencil.back                  = {};  // Optional

	VkPipelineColorBlendStateCreateInfo colorBlending{ VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO };
	colorBlending.logicOpEnable       = VK_FALSE;
	colorBlending.logicOp             = VK_LOGIC_OP_COPY;  // Optional
	colorBlending.attachmentCount     = 1;
	colorBlending.pAttachments        = &colorBlendAttachment;
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
	pipelineInfo.layout              = *gPipelineLayouts.Get( srGraphicsCreate.aPipelineLayout );
	pipelineInfo.renderPass          = VK_GetRenderPass();
	pipelineInfo.subpass             = 0;
	pipelineInfo.basePipelineHandle  = VK_NULL_HANDLE;  // Optional, very important for later when making new pipelines. It is less expensive to reference an existing similar pipeline
	pipelineInfo.basePipelineIndex   = -1;              // Optional

	ShaderVK* shader   = nullptr;
	Handle    handle   = gShaders.Create( &shader );
	shader->aBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;

	// TODO: look into trying to make multiple pipelines at once
	VK_CheckResult( vkCreateGraphicsPipelines( VK_GetDevice(), VK_NULL_HANDLE, 1, &pipelineInfo, NULL, &shader->aPipeline ), "Failed to create graphics pipeline!" );

	return handle;
}


// um

#if 0
void VK_BindImageShader()
{
	VkViewport viewport{};
	viewport.x = 0;
	viewport.y = 0;
	viewport.width  = VK_GetSwapExtent().width;
	viewport.height = VK_GetSwapExtent().height;
	viewport.minDepth = 0.f;
	viewport.maxDepth = 1.f;

	vkCmdSetViewport( VK_GetCommandBuffer(), 0, 1, &viewport );

	VkRect2D scissor{
		.offset = { 0, 0 },
		.extent = VK_GetSwapExtent(),
	};

	vkCmdSetScissor( VK_GetCommandBuffer(), 0, 1, &scissor );

	// bind pipeline and descriptor sets
	vkCmdBindPipeline( VK_GetCommandBuffer(), VK_PIPELINE_BIND_POINT_GRAPHICS, gPipeline );

	// bind vertex buffer
	VkBuffer     aBuffers[] = { gVertexBuffer };
	VkDeviceSize aOffsets[] = { 0 };
	vkCmdBindVertexBuffers( VK_GetCommandBuffer(), 0, 1, aBuffers, aOffsets );

	VkDescriptorSet sets[] = {
		VK_GetImageSets()[ VK_GetCommandIndex() ],
		VK_GetImageStorage()[ VK_GetCommandIndex() ],
	};

	vkCmdBindDescriptorSets( VK_GetCommandBuffer(), VK_PIPELINE_BIND_POINT_GRAPHICS, gPipelineLayout, 0, 1, sets, 0, nullptr );
}


void VK_DrawImageShader()
{
	for ( const auto& shaderDraw : gDrawQueue )
	{
		auto&     drawInfo = shaderDraw.aDrawInfo;

		// update push constant
		ImgPush_t push{};
		push.aWindowScale.x = VK_GetSwapExtent().width;
		push.aWindowScale.y = VK_GetSwapExtent().height;

		push.aImageScale = push.aScale;

		// idfk
		push.aBicubicScale.x  = 4;
		push.aBicubicScale.y  = 4;

		push.aScale.x = 2.0f / VK_GetSwapExtent().width;
		push.aScale.y = 2.0f / VK_GetSwapExtent().height;
		push.aTranslate.x = ( drawInfo.aX - ( VK_GetSwapExtent().width / 2.f ) ) * push.aScale.x;
		push.aTranslate.y = ( drawInfo.aY - ( VK_GetSwapExtent().height / 2.f ) ) * push.aScale.y;

		push.aScale.x *= drawInfo.aWidth;
		push.aScale.y *= drawInfo.aHeight;
		
		// useless? just use the textureSize() function?
		push.aDrawSize.x    = shaderDraw.aDrawInfo.aWidth;
		push.aDrawSize.y    = shaderDraw.aDrawInfo.aHeight;
		push.aTextureSize.x = shaderDraw.apInfo->aWidth;
		push.aTextureSize.y = shaderDraw.apInfo->aHeight;

		push.aFilterType = shaderDraw.aDrawInfo.aFilter;

		// printf( "SCALE:  %.6f x %.6f   TRANSLATE: %.6f x %.6f   OFFSET: %.6f x %.6f\n",
		// 	push.aScale.x, push.aScale.y,
		// 	push.aTranslate.x, push.aTranslate.y,
		// 	drawInfo.aX, drawInfo.aY
		// );

		push.aTexIndex = shaderDraw.apTexture->aIndex;
		push.aRotation = shaderDraw.aDrawInfo.aRotation;

		// maybe check if the compute shader has an output for this? idk

		vkCmdPushConstants( VK_GetCommandBuffer(), gPipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof( ImgPush_t ), &push );

		vkCmdDraw( VK_GetCommandBuffer(), gVertices.size(), 1, 0, 0 );
	}
}


void VK_CreateImageMesh()
{
	// gVertices.emplace_back( vec2{ -1.0f, -1.0f }, vec2{ 0.0f, 0.0f } );  // Bottom Left
	// gVertices.emplace_back( vec2{ 1.0f, -1.0f }, vec2{ 1.0f, 0.0f } );   // Bottom Right
	// gVertices.emplace_back( vec2{ 1.0f, 1.0f }, vec2{ 1.0f, 1.0f } );    // Top Right
	// 
	// gVertices.emplace_back( vec2{ -1.0f, -1.0f }, vec2{ 0.0f, 0.0f } );  // Bottom Left
	// gVertices.emplace_back( vec2{ -1.0f, 1.0f }, vec2{ 0.0f, 1.0f } );   // Top Left
	// gVertices.emplace_back( vec2{ 1.0f, 1.0f }, vec2{ 1.0f, 1.0f } );    // Top Right

	gVertices.emplace_back( vec2{ 0.0f, 0.0f }, vec2{ 0.0f, 0.0f } );  // Bottom Left
	gVertices.emplace_back( vec2{ 1.0f, 0.0f }, vec2{ 1.0f, 0.0f } );   // Bottom Right
	gVertices.emplace_back( vec2{ 1.0f, 1.0f }, vec2{ 1.0f, 1.0f } );    // Top Right

	gVertices.emplace_back( vec2{ 0.0f, 0.0f }, vec2{ 0.0f, 0.0f } );  // Bottom Left
	gVertices.emplace_back( vec2{ 0.0f, 1.0f }, vec2{ 0.0f, 1.0f } );   // Top Left
	gVertices.emplace_back( vec2{ 1.0f, 1.0f }, vec2{ 1.0f, 1.0f } );    // Top Right

	// create a vertex buffer for the mesh
	VkDeviceSize   bufferSize = sizeof( ImgVert_t ) * gVertices.size();

	VkBuffer       stagingBuffer;
	VkDeviceMemory stagingBufferMemory;

	VK_CreateBuffer(
	  stagingBuffer,
	  stagingBufferMemory,
	  bufferSize,
	  VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
	  VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT );

	VK_memcpy( stagingBufferMemory, bufferSize, gVertices.data() );

	VK_CreateBuffer(
	  gVertexBuffer,
	  gVertexBufferMem,
	  bufferSize,
	  VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
	  VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT );

	// copy buffer from cpu to the gpu
	VkBufferCopy copyRegion{
		.srcOffset = 0,
		.dstOffset = 0,
		.size      = bufferSize,
	};

	VK_SingleCommand( [ & ]( VkCommandBuffer c )
	                  { vkCmdCopyBuffer( c, stagingBuffer, gVertexBuffer, 1, &copyRegion ); } );

	// free staging buffer
	VK_DestroyBuffer( stagingBuffer, stagingBufferMemory );
}


void VK_CreateImagePipeline()
{
	gLayouts[ 0 ] = VK_GetImageLayout();
	gLayouts[ 1 ] = VK_GetImageStorageLayout();

	VkPipelineLayoutCreateInfo pipelineCreateInfo{
		.sType          = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
		.setLayoutCount = ARR_SIZE( gLayouts ),
		.pSetLayouts    = gLayouts
	};

	VkPushConstantRange pushConstantRange{
		.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
		.offset     = 0,
		.size       = sizeof( ImgPush_t )
	};

	pipelineCreateInfo.pushConstantRangeCount = 1;
	pipelineCreateInfo.pPushConstantRanges    = &pushConstantRange;

	VK_CheckResult( vkCreatePipelineLayout( VK_GetDevice(), &pipelineCreateInfo, NULL, &gPipelineLayout ), "Failed to create pipeline layout" );

	VkPipelineShaderStageCreateInfo shaderStages[ 2 ]{
		{ .sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
		  .stage  = VK_SHADER_STAGE_VERTEX_BIT,
		  .module = gShaderModules[ 0 ],
		  .pName  = "main" },

		{ .sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
		  .stage  = VK_SHADER_STAGE_FRAGMENT_BIT,
		  .module = gShaderModules[ 1 ],
		  .pName  = "main" }
	};

	VkVertexInputBindingDescription bindingDescriptions[] = 
	{
		{ .binding   = 0,
		  .stride    = sizeof( ImgVert_t ),
		  .inputRate = VK_VERTEX_INPUT_RATE_VERTEX },
	};

	VkVertexInputAttributeDescription attributeDescriptions[] = 
	{
		{ .location = 0,
		  .binding  = 0,
		  .format   = VK_FORMAT_R32G32_SFLOAT,
		  .offset   = offsetof( ImgVert_t, aPos ) },

		{ .location = 1,
		  .binding  = 0,
		  .format   = VK_FORMAT_R32G32_SFLOAT,
		  .offset   = offsetof( ImgVert_t, aUV ) },
	};

	VkPipelineVertexInputStateCreateInfo vertexInputInfo{
		.sType                           = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
		.vertexBindingDescriptionCount   = ARR_SIZE( bindingDescriptions ),
		.pVertexBindingDescriptions      = bindingDescriptions,
		.vertexAttributeDescriptionCount = ARR_SIZE( attributeDescriptions ),
		.pVertexAttributeDescriptions    = attributeDescriptions
	};

	VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo{
		.sType                  = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
		.topology               = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
		.primitiveRestartEnable = VK_FALSE
	};

	const auto& swapExtent = VK_GetSwapExtent();

	VkViewport viewport{
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
	VkPipelineRasterizationStateCreateInfo rasterizer{};  //	Turns  primitives into fragments, aka, pixels for the framebuffer
	rasterizer.sType                   = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizer.depthClampEnable        = VK_FALSE;
	rasterizer.rasterizerDiscardEnable = VK_FALSE;
	rasterizer.polygonMode             = VK_POLYGON_MODE_FILL;  //	Fill with fragments, can optionally use VK_POLYGON_MODE_LINE for a wireframe
	rasterizer.lineWidth               = 1.0f;
	// rasterizer.cullMode 			= ( sFlags & NO_CULLING ) ? VK_CULL_MODE_NONE : VK_CULL_MODE_BACK_BIT;	//	FIX FOR MAKING 2D SPRITES WORK!!! WOOOOO!!!!
	// rasterizer.cullMode 			= VK_CULL_MODE_BACK_BIT;
	rasterizer.cullMode                = VK_CULL_MODE_NONE;
	rasterizer.frontFace               = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	rasterizer.depthBiasEnable         = VK_FALSE;
	rasterizer.depthBiasConstantFactor = 0.0f;  // Optional
	rasterizer.depthBiasClamp          = 0.0f;  // Optional
	rasterizer.depthBiasSlopeFactor    = 0.0f;  // Optional

	VkPipelineMultisampleStateCreateInfo multisampling{};  //	Performs anti-aliasing
	multisampling.sType                 = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampling.sampleShadingEnable   = VK_FALSE;
	multisampling.rasterizationSamples  = VK_GetMSAASamples();
	multisampling.minSampleShading      = 1.0f;      // Optional
	multisampling.pSampleMask           = NULL;      // Optional
	multisampling.alphaToCoverageEnable = VK_FALSE;  // Optional
	multisampling.alphaToOneEnable      = VK_FALSE;  // Optional

	VkPipelineColorBlendAttachmentState colorBlendAttachment{};
	colorBlendAttachment.colorWriteMask      = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	colorBlendAttachment.blendEnable         = VK_TRUE;
	colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
	colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	colorBlendAttachment.colorBlendOp        = VK_BLEND_OP_ADD;
	colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	colorBlendAttachment.alphaBlendOp        = VK_BLEND_OP_ADD;

	VkPipelineDepthStencilStateCreateInfo depthStencil{};
	depthStencil.sType                 = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	//depthStencil.depthTestEnable 		= ( sFlags & NO_DEPTH ) ? VK_FALSE : VK_TRUE;
	//depthStencil.depthWriteEnable		= ( sFlags & NO_DEPTH ) ? VK_FALSE : VK_TRUE;
	depthStencil.depthTestEnable       = VK_TRUE;
	depthStencil.depthWriteEnable      = VK_TRUE;
	depthStencil.depthCompareOp        = VK_COMPARE_OP_LESS;
	depthStencil.depthBoundsTestEnable = VK_FALSE;
	depthStencil.minDepthBounds        = 0.0f;  // Optional
	depthStencil.maxDepthBounds        = 1.0f;  // Optional
	depthStencil.stencilTestEnable     = VK_FALSE;
	depthStencil.front                 = {};  // Optional
	depthStencil.back                  = {};  // Optional

	VkPipelineColorBlendStateCreateInfo colorBlending{};
	colorBlending.sType               = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlending.logicOpEnable       = VK_FALSE;
	colorBlending.logicOp             = VK_LOGIC_OP_COPY;  // Optional
	colorBlending.attachmentCount     = 1;
	colorBlending.pAttachments        = &colorBlendAttachment;
	colorBlending.blendConstants[ 0 ] = 0.0f;  // Optional
	colorBlending.blendConstants[ 1 ] = 0.0f;  // Optional
	colorBlending.blendConstants[ 2 ] = 0.0f;  // Optional
	colorBlending.blendConstants[ 3 ] = 0.0f;  // Optional

	VkDynamicState                   dynamicStates[]{ VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
	
	VkPipelineDynamicStateCreateInfo dynamicState{};
	dynamicState.sType             = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamicState.dynamicStateCount = ARR_SIZE( dynamicStates );
	dynamicState.pDynamicStates    = dynamicStates;

	VkGraphicsPipelineCreateInfo pipelineInfo{};  //	Combine all the objects above into one parameter for graphics pipeline creation
	pipelineInfo.sType               = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineInfo.stageCount          = ARR_SIZE( shaderStages );
	pipelineInfo.pStages             = shaderStages;
	pipelineInfo.pVertexInputState   = &vertexInputInfo;
	pipelineInfo.pInputAssemblyState = &inputAssemblyInfo;
	pipelineInfo.pViewportState      = &viewportInfo;
	pipelineInfo.pRasterizationState = &rasterizer;
	pipelineInfo.pMultisampleState   = &multisampling;
	pipelineInfo.pDepthStencilState  = &depthStencil;
	pipelineInfo.pColorBlendState    = &colorBlending;
	pipelineInfo.pDynamicState       = &dynamicState;
	pipelineInfo.layout              = gPipelineLayout;
	pipelineInfo.renderPass          = VK_GetRenderPass();
	pipelineInfo.subpass             = 0;
	pipelineInfo.basePipelineHandle  = VK_NULL_HANDLE;  // Optional, very important for later when making new pipelines. It is less expensive to reference an existing similar pipeline
	pipelineInfo.basePipelineIndex   = -1;              // Optional

	VK_CheckResult( vkCreateGraphicsPipelines( VK_GetDevice(), VK_NULL_HANDLE, 1, &pipelineInfo, NULL, &gPipeline ), "Failed to create graphics pipeline!" );
}


void VK_CreateImageShader()
{
	gShaderModules[ 0 ] = VK_CreateShaderModule( gImageShaderVert, sizeof( gImageShaderVert ) );
	gShaderModules[ 1 ] = VK_CreateShaderModule( gImageShaderFrag, sizeof( gImageShaderFrag ) );

	VK_CreateImagePipeline();
	VK_CreateImageMesh();
}


void VK_DestroyImageShader()
{
	if ( gShaderModules[ 0 ] )
		vkDestroyShaderModule( VK_GetDevice(), gShaderModules[ 0 ], nullptr );

	if ( gShaderModules[ 1 ] )
		vkDestroyShaderModule( VK_GetDevice(), gShaderModules[ 1 ], nullptr );

	gShaderModules[ 0 ] = nullptr;
	gShaderModules[ 1 ] = nullptr;

	if ( gVertexBuffer )
		VK_DestroyBuffer( gVertexBuffer, gVertexBufferMem );

	gVertexBuffer = nullptr;
	gVertexBufferMem = nullptr;

	if ( gPipelineLayout )
		vkDestroyPipelineLayout( VK_GetDevice(), gPipelineLayout, nullptr );

	if ( gPipeline )
		vkDestroyPipeline( VK_GetDevice(), gPipeline, nullptr );

	gPipeline = nullptr;
	gPipelineLayout = nullptr;
}
#endif
