#include "render.h"


// TODO: test encoding pattern with this
enum e_shader_create_tags
{
	e_shader_create_tag_none = 0,
	e_shader_create_tag_material_vars,
	e_shader_create_tag_material_buffer,
};


// for later

// shader material var descriptor
struct vk_shader_material_var_desc_t
{
//	e_mat_var type;
	const char* name;
	const char* desc;

	union
	{
		const char* default_texture;  // Path To Default Texture
		float       default_float;
		int         default_int;
		glm::vec2   default_vec2;
		glm::vec3   default_vec3;
		glm::vec4   default_vec4;
	};

	u32 offset;
	u32 size;
};


struct vk_shader_create_material_buffer_t
{
	u32 buffer_size;
	u32 buffer_binding;
};


struct vk_shader_create_material_t
{
	// material vars
	vk_shader_material_var_desc_t* var;
	u32                            var_count;
	
	// if size is 0, no buffer is created
	u32                            buffer_size;
	u32                            buffer_binding;
};


struct vk_shader_create_compute_t
{
	const char*               name;
	vk_shader_module_create_t module;
};


static vk_shader_create_graphics_t* g_shader_create_graphics;
static u32                          g_shader_create_graphics_count;

static vk_shader_create_compute_t*  g_shader_create_compute;
static u32                          g_shader_create_compute_count;

static vk_shader_data_graphics_t*   g_shader_data_graphics;
static VkPipelineLayout*            g_shader_data_graphics_pipeline_layout;
static VkPipeline*                  g_shader_data_graphics_pipelines;
static ch_string*                   g_shader_data_graphics_names;
static u32                          g_shader_data_graphics_count = 0;


bool vk_load_shader_module( vk_shader_module_create_t* module_creates, VkPipelineShaderStageCreateInfo* stage_create, VkShaderModule** shader_modules_ptr, u32 count )
{
	*shader_modules_ptr            = ch_malloc< VkShaderModule >( count );

	VkShaderModule* shader_modules = *shader_modules_ptr;

	for ( u32 i = 0; i < count; i++ )
	{
		vk_shader_module_create_t& stage   = module_creates[ i ];

		ch_string_auto  absPath = FileSys_FindFile( stage.path );
		if ( !absPath.data )
		{
			Log_ErrorF( gLC_Render, "Failed to find shader: \"%s\"\n", stage.path );
			ch_free( shader_modules );
			return false;
		}

		ch_string_auto fileData = FileSys_ReadFile( absPath.data, absPath.size );

		if ( !fileData.data )
		{
			Log_ErrorF( gLC_Render, "Shader file is empty: \"%s\"\n", stage.path );
			ch_free( shader_modules );
			return false;
		}

		auto& stageInfo = stage_create[ i ];
		stageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		stageInfo.pName = stage.entry;
		stageInfo.stage = stage.stage;

		VkShaderModuleCreateInfo createInfo{ VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO };
		createInfo.codeSize = fileData.size;
		createInfo.pCode    = (u32*)fileData.data;

		if ( vk_check_e( vkCreateShaderModule( g_vk_device, &createInfo, NULL, &stageInfo.module ), "Failed to create shader module!" ) )
		{
			ch_free( shader_modules );
			return false;
		}

		shader_modules[ i ] = stageInfo.module;
	}

	return true;
}


bool vk_shaders_create_graphics_pipeline( VkPipelineLayout layout, vk_shader_create_graphics_t* graphics_create )
{
	return false;

#if 0
	std::vector< VkPipelineShaderStageCreateInfo > shaderStages;
	shaderStages.resize( srGraphicsCreate.aShaderModules.size() );
	VkShaderModule* vkShaderModules = nullptr;
	if ( !VK_GetShaderStageCreateInfo( shaderStages.data(), srGraphicsCreate.aShaderModules.data(), srGraphicsCreate.aShaderModules.size(), vkShaderModules ) )
	{
		Log_ErrorF( gLC_Render, "VK_CreateGraphicsPipeline(): Failed to create shader stage info: \"%s\"\n", srGraphicsCreate.apName );
		return false;
	}

	VkPipelineInputAssemblyStateCreateInfo inputAssembly{ VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO };
	inputAssembly.primitiveRestartEnable = VK_FALSE;
	inputAssembly.topology               = graphics_create->topology;

	// TODO: AAAAAAAAAAAAAAAA WHY DO I NEED TO DO THIS
	// placeholder for like 85 years, all shaders use dynamic viewport and scissor anyway
	VkViewport viewport{
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
	rasterizer.polygonMode             = graphics_create->polygon_mode;
	rasterizer.frontFace               = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	rasterizer.depthBiasEnable         = srGraphicsCreate.aDepthBiasEnable;
	rasterizer.depthBiasConstantFactor = 0.0f;  // Optional
	rasterizer.depthBiasClamp          = 0.0f;  // Optional
	rasterizer.depthBiasSlopeFactor    = 0.0f;  // Optional
	rasterizer.cullMode                = graphics_create->cull_mode;

	// performs anti-aliasing
	VkPipelineMultisampleStateCreateInfo multisampling{ VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO };
	multisampling.sampleShadingEnable   = VK_FALSE;
	multisampling.rasterizationSamples  = VK_SAMPLE_COUNT_1_BIT;
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

	VkPipelineDynamicStateCreateInfo dynamicState{ VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO };
	dynamicState.dynamicStateCount = graphics_create->dynamic_state_count;
	dynamicState.pDynamicStates    = graphics_create->dynamic_state;

	// Combine all the objects above into one parameter for graphics pipeline creation
	VkGraphicsPipelineCreateInfo pipelineInfo{ VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO };
	pipelineInfo.stageCount          = (u32)shaderStages.size();
	pipelineInfo.pStages             = shaderStages.data();
	pipelineInfo.pVertexInputState   = nullptr;
	pipelineInfo.pInputAssemblyState = &inputAssembly;
	pipelineInfo.pViewportState      = &viewportInfo;
	pipelineInfo.pRasterizationState = &rasterizer;
	pipelineInfo.pMultisampleState   = &multisampling;
	pipelineInfo.pDepthStencilState  = &depthStencil;
	pipelineInfo.pColorBlendState    = &colorBlending;
	pipelineInfo.pDynamicState       = &dynamicState;
	pipelineInfo.layout              = layout;
	pipelineInfo.renderPass          = renderPass;
	pipelineInfo.subpass             = 0;
	pipelineInfo.basePipelineHandle  = VK_NULL_HANDLE;  // Optional, very important for later when making new pipelines. It is less expensive to reference an existing similar pipeline
	pipelineInfo.basePipelineIndex   = -1;              // Optional

	// allocate new shader data
	vk_shader_data_graphics_t* new_data = ch_realloc( g_shader_data_graphics, g_shader_data_graphics_count + 1 );

	if ( !new_data )
	{
		Log_ErrorF( gLC_Render, "Failed to create graphics shader data for \"%s\"\n", srGraphicsCreate.apName );
		return false;
	}

	g_shader_data_graphics = new_data;

	// create the pipeline
	vk_shader_data_graphics_t& shader_data = g_shader_data_graphics[ g_shader_data_graphics_count ];

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
	vk_check_f(
	  vkCreateGraphicsPipelines( g_vk_device, VK_NULL_HANDLE, 1, &pipelineInfo, NULL, &shader->aPipeline ),
	  "Failed to create graphics pipeline for shader: \"%s\"", srGraphicsCreate.apName );

	vk_set_name_ex( VK_OBJECT_TYPE_PIPELINE, (u64)shader->aPipeline, graphics_create->name, "Graphics Pipeline" );

	return true;
#endif
}


bool vk_shaders_create_graphics( vk_shader_create_graphics_t* graphics_create )
{
	// create the pipeline layout for this shader
	VkPipelineLayoutCreateInfo layout_create{ VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };
	layout_create.pSetLayouts    = &g_vk_desc_layout_graphics;
	layout_create.setLayoutCount = 1;

	// does this shader use push constants?
	if ( graphics_create->fn_push_constants )
	{
		layout_create.pPushConstantRanges     = &graphics_create->push_constant_range;
		layout_create.pushConstantRangeCount = 1;
	}

	VkPipelineLayout layout;
	if ( vk_check_e( vkCreatePipelineLayout( g_vk_device, &layout_create, nullptr, &layout ), "Failed to create pipeline layout" ) )
		return false;

	// load the shader
	VkPipelineShaderStageCreateInfo stage_create{ VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO };
	VkShaderModule*                 shader_modules = nullptr;

	if ( !vk_load_shader_module( graphics_create->modules, &stage_create, &shader_modules, 2 ) )
	{
		Log_Error( "Failed to create shader stage" );
		return false;
	}

	if ( !vk_shaders_create_graphics_pipeline( layout, graphics_create ) )
	{
		Log_Error( "Failed to create graphics pipeline" );
		return false;
	}

	// g_shader_module_gradient = shader_modules[ 0 ];

	ch_free( shader_modules );

	g_shader_data_graphics_count++;

	return true;
}


bool vk_load_test_shader();
void vk_shader_destroy_test();


// for now, just load one test compute shader
bool vk_shaders_init()
{	
	if ( !vk_load_test_shader() )
		return false;

	// load graphics shaders
	for ( u32 i = 0; i < g_shader_create_graphics_count; i++ )
	{
		if ( !vk_shaders_create_graphics( &g_shader_create_graphics[ i ] ) )
			return false;
	}

	return true;
}


void vk_shaders_shutdown()
{
	vk_shader_destroy_test();

	for ( u32 i = 0; i < g_shader_create_graphics_count; i++ )
	{
	}
}


void vk_shaders_update_push_constants()
{
	// temp for test compute shader

}


void vk_shaders_register_graphics( vk_shader_create_graphics_t* shader_create )
{
	if ( !shader_create->name )
	{
		Log_Error( "Failed to register graphics shader: name is null" );
		return;
	}

	// g_shader_create_graphics
	vk_shader_create_graphics_t* new_data = ch_realloc< vk_shader_create_graphics_t >( g_shader_create_graphics, g_shader_create_graphics_count + 1 );

	if ( new_data )
	{
		g_shader_create_graphics = new_data;
		g_shader_create_graphics[ g_shader_create_graphics_count++ ] = *shader_create;
	}
	else
	{
		Log_ErrorF( "Failed to register graphics shader \"%s\"", shader_create->name );
	}
}

