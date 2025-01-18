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

vk_shader_data_graphics_t*          g_shader_data_graphics;
VkPipelineLayout*                   g_shader_data_graphics_pipeline_layout;
VkPipeline*                         g_shader_data_graphics_pipelines;
ch_string*                          g_shader_data_graphics_names;
u32                                 g_shader_data_graphics_count = 0;


bool                                vk_load_shader_module( vk_shader_module_create_t* module_creates, VkPipelineShaderStageCreateInfo* stage_create, VkShaderModule* shader_modules, u32 count )
{
	for ( u32 i = 0; i < count; i++ )
	{
		vk_shader_module_create_t& stage   = module_creates[ i ];

		ch_string_auto             absPath = FileSys_FindFile( stage.path );
		if ( !absPath.data )
		{
			Log_ErrorF( gLC_Render, "Failed to find shader: \"%s\"\n", stage.path );
			return false;
		}

		ch_string_auto fileData = FileSys_ReadFile( absPath.data, absPath.size );

		if ( !fileData.data )
		{
			Log_ErrorF( gLC_Render, "Shader file is empty: \"%s\"\n", stage.path );
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
			return false;

		shader_modules[ i ] = stageInfo.module;
	}

	return true;
}


bool vk_shaders_create_graphics_pipeline( VkPipelineLayout layout, vk_shader_create_graphics_t* graphics_create, u32 index )
{
#if 0
	return false;
#else
	VkPipelineShaderStageCreateInfo shader_stages[ 2 ]{ VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO };
	vk_shader_data_graphics_t&      shader_data    = g_shader_data_graphics[ index ];

	if ( !vk_load_shader_module( graphics_create->modules, shader_stages, shader_data.modules, 2 ) )
	{
		Log_ErrorF( gLC_Render, "VK_CreateGraphicsPipeline(): Failed to create shader stage info: \"%s\"\n", graphics_create->name );
		return false;
	}

	VkPipelineInputAssemblyStateCreateInfo inputAssembly{ VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO };
	inputAssembly.primitiveRestartEnable = VK_FALSE;
	inputAssembly.topology               = graphics_create->topology;

	// Need to have default values for viewport and scissor despite them being dynamic state
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
	// rasterizer.frontFace               = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	rasterizer.frontFace               = VK_FRONT_FACE_CLOCKWISE;
	rasterizer.depthBiasEnable         = false;  // true for shadowmapping in render2
	rasterizer.depthBiasConstantFactor = 0.0f;   // Optional
	rasterizer.depthBiasClamp          = 0.0f;   // Optional
	rasterizer.depthBiasSlopeFactor    = 0.0f;   // Optional
	rasterizer.cullMode                = graphics_create->cull_mode;

	// performs anti-aliasing
	VkPipelineMultisampleStateCreateInfo multisampling{ VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO };
	multisampling.sampleShadingEnable   = r_msaa_enabled && r_msaa_textures ? VK_TRUE : VK_FALSE;
	multisampling.rasterizationSamples  = vk_msaa_get_samples( r_msaa_enabled );
	multisampling.minSampleShading      = r_msaa_textures_min;  // Optional
	multisampling.pSampleMask           = NULL;      // Optional
	multisampling.alphaToCoverageEnable = VK_FALSE;  // Optional
	multisampling.alphaToOneEnable      = VK_FALSE;  // Optional

	VkPipelineColorBlendAttachmentState color_blend_attach{};

	color_blend_attach.colorWriteMask      = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	// color_blend_attach.colorWriteMask      = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT;
	color_blend_attach.blendEnable         = VK_TRUE;  // was true in some shaders
	// color_blend_attach.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
	color_blend_attach.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
	color_blend_attach.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	color_blend_attach.colorBlendOp        = VK_BLEND_OP_ADD;
	color_blend_attach.srcAlphaBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
	color_blend_attach.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	color_blend_attach.alphaBlendOp        = VK_BLEND_OP_SUBTRACT;

	// colorBlendAttachments[ i ].srcColorBlendFactor = VK_BLEND_FACTOR_ZERO;
	// colorBlendAttachments[ i ].dstColorBlendFactor = VK_BLEND_FACTOR_SRC_COLOR;
	// colorBlendAttachments[ i ].colorBlendOp        = VK_BLEND_OP_ADD;
	//
	// colorBlendAttachments[ i ].srcAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	// colorBlendAttachments[ i ].dstAlphaBlendFactor = VK_BLEND_FACTOR_SRC_COLOR;
	// colorBlendAttachments[ i ].alphaBlendOp        = VK_BLEND_OP_ADD;

	// TODO: expose this for shadow mapping
	VkPipelineDepthStencilStateCreateInfo depthStencil{ VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO };
	depthStencil.depthTestEnable       = VK_TRUE;
	depthStencil.depthWriteEnable      = VK_TRUE;
	// depthStencil.depthCompareOp        = VK_COMPARE_OP_LESS;
	// depthStencil.depthCompareOp        = VK_COMPARE_OP_NEVER;
	// depthStencil.depthCompareOp        = VK_COMPARE_OP_GREATER_OR_EQUAL;
	depthStencil.depthCompareOp        = VK_COMPARE_OP_LESS_OR_EQUAL;
	depthStencil.depthBoundsTestEnable = VK_FALSE;
	depthStencil.minDepthBounds        = 0.0f;  // Optional
	depthStencil.maxDepthBounds        = 1.0f;  // Optional
	depthStencil.stencilTestEnable     = VK_FALSE;
	depthStencil.front                 = {};  // Optional
	depthStencil.back                  = {};  // Optional

//	depthStencil.depthTestEnable       = VK_FALSE;
//	depthStencil.depthWriteEnable      = VK_FALSE;
//	// depthStencil.depthCompareOp        = VK_COMPARE_OP_LESS;
//	// depthStencil.depthCompareOp        = VK_COMPARE_OP_LESS;
//	depthStencil.depthCompareOp        = VK_COMPARE_OP_NEVER;
//	depthStencil.depthBoundsTestEnable = VK_FALSE;
//	depthStencil.minDepthBounds        = 0.0f;  // Optional
//	depthStencil.maxDepthBounds        = 1.0f;  // Optional
//	depthStencil.stencilTestEnable     = VK_FALSE;
//	depthStencil.front                 = {};  // Optional
//	depthStencil.back                  = {};  // Optional

	VkPipelineColorBlendStateCreateInfo colorBlending{ VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO };
	colorBlending.logicOpEnable       = VK_FALSE;
	colorBlending.logicOp             = VK_LOGIC_OP_COPY;  // Optional
	colorBlending.attachmentCount     = 1;
	colorBlending.pAttachments        = &color_blend_attach;
	colorBlending.blendConstants[ 0 ] = 0.0f;  // Optional
	colorBlending.blendConstants[ 1 ] = 0.0f;  // Optional
	colorBlending.blendConstants[ 2 ] = 0.0f;  // Optional
	colorBlending.blendConstants[ 3 ] = 0.0f;  // Optional

	VkPipelineDynamicStateCreateInfo dynamicState{ VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO };
	dynamicState.dynamicStateCount = graphics_create->dynamic_state_count;
	dynamicState.pDynamicStates    = graphics_create->dynamic_state;

	// TEMP VERTEX BUFFER BINDING
#if 0
	VkVertexInputBindingDescription binding_description{};
	binding_description.binding   = 0;
	binding_description.stride    = sizeof( gpu_vertex_t );
	binding_description.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

	VkVertexInputAttributeDescription attrib_description[ 5 ]{};
	attrib_description[ 0 ].location = 0;
	attrib_description[ 0 ].format   = VK_FORMAT_R32G32B32_SFLOAT;
	attrib_description[ 0 ].offset   = offsetof( gpu_vertex_t, pos );

	attrib_description[ 1 ].location = 1;
	attrib_description[ 1 ].format   = VK_FORMAT_R32_SFLOAT;
	attrib_description[ 1 ].offset   = offsetof( gpu_vertex_t, uv_x );

	attrib_description[ 2 ].location = 2;
	attrib_description[ 2 ].format   = VK_FORMAT_R32G32B32_SFLOAT;
	attrib_description[ 2 ].offset   = offsetof( gpu_vertex_t, normal );

	attrib_description[ 3 ].location = 3;
	attrib_description[ 3 ].format   = VK_FORMAT_R32_SFLOAT;
	attrib_description[ 3 ].offset   = offsetof( gpu_vertex_t, uv_y );

	attrib_description[ 4 ].location = 4;
	attrib_description[ 4 ].format   = VK_FORMAT_R32G32B32A32_SFLOAT;
	attrib_description[ 4 ].offset   = offsetof( gpu_vertex_t, color );

	VkPipelineVertexInputStateCreateInfo vertex_input{ VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO };
	vertex_input.pVertexAttributeDescriptions    = attrib_description;
	vertex_input.vertexAttributeDescriptionCount = 5;
	vertex_input.pVertexBindingDescriptions      = &binding_description;
	vertex_input.vertexBindingDescriptionCount   = 1;
#else
	VkPipelineVertexInputStateCreateInfo vertex_input{ VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO };
#endif

	// Dynamic Rendering
	VkPipelineRenderingCreateInfo rendering_info{ VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO };
	rendering_info.colorAttachmentCount    = graphics_create->color_attach_count;
	rendering_info.pColorAttachmentFormats = graphics_create->color_attach;
	rendering_info.depthAttachmentFormat   = graphics_create->depth_attach;

	// Combine all the objects above into one parameter for graphics pipeline creation
	VkGraphicsPipelineCreateInfo pipelineInfo{ VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO };
	pipelineInfo.pNext               = &rendering_info;
	pipelineInfo.stageCount          = 2;
	pipelineInfo.pStages             = shader_stages;
	pipelineInfo.pVertexInputState   = &vertex_input;
	pipelineInfo.pInputAssemblyState = &inputAssembly;
	pipelineInfo.pViewportState      = &viewportInfo;
	pipelineInfo.pRasterizationState = &rasterizer;
	pipelineInfo.pMultisampleState   = &multisampling;
	pipelineInfo.pDepthStencilState  = &depthStencil;
	pipelineInfo.pColorBlendState    = &colorBlending;
	pipelineInfo.pDynamicState       = &dynamicState;
	pipelineInfo.layout              = layout;
	pipelineInfo.renderPass          = VK_NULL_HANDLE;
	pipelineInfo.subpass             = 0;
	pipelineInfo.basePipelineHandle  = VK_NULL_HANDLE;  // Optional, very important for later when making new pipelines. It is less expensive to reference an existing similar pipeline
	pipelineInfo.basePipelineIndex   = -1;              // Optional

	// TODO: look into trying to make multiple pipelines at once
	vk_check_f(
	  vkCreateGraphicsPipelines( g_vk_device, VK_NULL_HANDLE, 1, &pipelineInfo, NULL, &g_shader_data_graphics_pipelines[ index ] ),
	  "Failed to create graphics pipeline for shader: \"%s\"", graphics_create->name );

	vk_set_name_ex( VK_OBJECT_TYPE_PIPELINE, (u64)g_shader_data_graphics_pipelines[ index ], graphics_create->name, "Graphics Pipeline" );

	return true;
#endif
}


bool vk_shaders_create_graphics( vk_shader_create_graphics_t* graphics_create, u32 index )
{
	// create the pipeline layout for this shader
	VkPipelineLayoutCreateInfo layout_create{ VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };
	layout_create.pSetLayouts    = &g_vk_desc_layout_graphics;
	layout_create.setLayoutCount = 1;

	// does this shader use push constants?
	if ( graphics_create->fn_push_constants )
	{
		layout_create.pPushConstantRanges    = &graphics_create->push_constant_range;
		layout_create.pushConstantRangeCount = 1;
	}

	VkPipelineLayout layout;
	if ( vk_check_e( vkCreatePipelineLayout( g_vk_device, &layout_create, nullptr, &layout ), "Failed to create pipeline layout" ) )
		return false;

	// load the shader
	if ( !vk_shaders_create_graphics_pipeline( layout, graphics_create, index ) )
	{
		Log_Error( "Failed to create graphics pipeline" );
		return false;
	}

	g_shader_data_graphics_pipeline_layout[ index ] = layout;
	g_shader_data_graphics_names[ index ]           = ch_str_copy( graphics_create->name );

	return true;
}


bool vk_load_test_shader();
void vk_shader_destroy_test();


// for now, just load one test compute shader
bool vk_shaders_init()
{
	if ( !vk_load_test_shader() )
		return false;

	// preallocate these
	g_shader_data_graphics                 = ch_calloc< vk_shader_data_graphics_t >( g_shader_create_graphics_count );
	g_shader_data_graphics_pipeline_layout = ch_calloc< VkPipelineLayout >( g_shader_create_graphics_count );
	g_shader_data_graphics_pipelines       = ch_calloc< VkPipeline >( g_shader_create_graphics_count );
	g_shader_data_graphics_names           = ch_calloc< ch_string >( g_shader_create_graphics_count );

	// load graphics shaders
	for ( u32 i = 0; i < g_shader_create_graphics_count; i++ )
	{
		if ( !vk_shaders_create_graphics( &g_shader_create_graphics[ i ], i ) )
			return false;
	}

	return true;
}


void vk_shaders_shutdown()
{
	vk_shader_destroy_test();

	for ( u32 i = 0; i < g_shader_create_graphics_count; i++ )
	{
		vkDestroyShaderModule( g_vk_device, g_shader_data_graphics[ i ].modules[ 0 ], nullptr );
		vkDestroyShaderModule( g_vk_device, g_shader_data_graphics[ i ].modules[ 1 ], nullptr );

		vkDestroyPipeline( g_vk_device, g_shader_data_graphics_pipelines[ i ], nullptr );
		vkDestroyPipelineLayout( g_vk_device, g_shader_data_graphics_pipeline_layout[ i ], nullptr );
	}

	free( g_shader_data_graphics );
	free( g_shader_data_graphics_pipeline_layout );
	free( g_shader_data_graphics_pipelines );
	free( g_shader_data_graphics_names );
}


bool vk_shaders_rebuild()
{
	vk_queue_wait_graphics();
	vkDeviceWaitIdle( g_vk_device );

	for ( u32 i = 0; i < g_shader_create_graphics_count; i++ )
	{
		vkDestroyShaderModule( g_vk_device, g_shader_data_graphics[ i ].modules[ 0 ], nullptr );
		vkDestroyShaderModule( g_vk_device, g_shader_data_graphics[ i ].modules[ 1 ], nullptr );

		vkDestroyPipeline( g_vk_device, g_shader_data_graphics_pipelines[ i ], nullptr );
		vkDestroyPipelineLayout( g_vk_device, g_shader_data_graphics_pipeline_layout[ i ], nullptr );
	}

	// load graphics shaders
	for ( u32 i = 0; i < g_shader_create_graphics_count; i++ )
	{
		if ( !vk_shaders_create_graphics( &g_shader_create_graphics[ i ], i ) )
			return false;
	}

	return true;
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
		g_shader_create_graphics                                     = new_data;
		g_shader_create_graphics[ g_shader_create_graphics_count++ ] = *shader_create;
	}
	else
	{
		Log_ErrorF( "Failed to register graphics shader \"%s\"", shader_create->name );
	}
}


u32 vk_shaders_find( const char* shader_name )
{
	if ( !shader_name )
		return 0;

	for ( u32 i = 0; i < g_shader_create_graphics_count; i++ )
	{
		if ( ch_strcasecmp( g_shader_data_graphics_names[ i ].data, shader_name ) == 0 )
			return i + 1;
	}

	return 0;
}

