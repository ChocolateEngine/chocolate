#include "render.h"


constexpr const char* SHADER_PATH = "shaders/render3/guide_compute.comp.spv";


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


struct vk_shader_create_graphics_t
{
	const char*               name;
	vk_shader_module_create_t modules[ 2 ];  // vertex and fragment

	VkDynamicState            dynamic_state;

	// push constants

	// descriptor set bindings

	// material data (maybe use encoding pattern for this?)
	vk_shader_create_material_t material;
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


vk_shader_module_create_t g_compute_shader_info{
	VK_SHADER_STAGE_COMPUTE_BIT,
	SHADER_PATH,
	"main",  // NOTE: there could be multiple entry points
};


VkPipeline       g_pipeline_gradient;
VkPipelineLayout g_pipeline_gradient_layout;
VkShaderModule   g_shader_module_gradient;


static bool      vk_load_shader_module( vk_shader_module_create_t* module_creates, VkPipelineShaderStageCreateInfo* stage_create, VkShaderModule** shader_modules_ptr, u32 count )
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


bool vk_load_test_shader()
{
	// create the pipeline layout for this shader
	VkPipelineLayoutCreateInfo compute_layout{ VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };
	compute_layout.pSetLayouts    = &g_vk_desc_draw_image_layout;
	compute_layout.setLayoutCount = 1;

	if ( vk_check_e( vkCreatePipelineLayout( g_vk_device, &compute_layout, nullptr, &g_pipeline_gradient_layout ), "Failed to create pipeline layout" ) )
		return false;

	// load the shader
	VkPipelineShaderStageCreateInfo stage_create{ VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO };
	VkShaderModule*                 shader_modules = nullptr;

	if ( !vk_load_shader_module( &g_compute_shader_info, &stage_create, &shader_modules, 1 ) )
	{
		Log_Error( "Failed to create shader stage" );
		return false;
	}

	VkComputePipelineCreateInfo compute_pipeline{ VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO };
	compute_pipeline.layout = g_pipeline_gradient_layout;
	compute_pipeline.stage  = stage_create;

	if ( vk_check_e( vkCreateComputePipelines( g_vk_device, VK_NULL_HANDLE, 1, &compute_pipeline, nullptr, &g_pipeline_gradient ), "Failed to create compute pipeline" ) )
		return false;

	g_shader_module_gradient = shader_modules[ 0 ];

	ch_free( shader_modules );
	return true;
}


// for now, just load one test compute shader
bool vk_shaders_init()
{	
	return vk_load_test_shader();
}


void vk_shaders_shutdown()
{
	vkDestroyPipeline( g_vk_device, g_pipeline_gradient, nullptr );
	vkDestroyPipelineLayout( g_vk_device, g_pipeline_gradient_layout, nullptr );
	vkDestroyShaderModule( g_vk_device, g_shader_module_gradient, nullptr );
}

