#include "render.h"


constexpr const char*     TEST_SHADER_PATH = "shaders/render3/guide_compute.comp.spv";


VkPipeline                g_pipeline_gradient;
VkPipelineLayout          g_pipeline_gradient_layout;
VkShaderModule            g_shader_module_gradient;


vk_shader_module_create_t g_compute_shader_info{
	VK_SHADER_STAGE_COMPUTE_BIT,
	TEST_SHADER_PATH,
	"main",  // NOTE: there could be multiple entry points
};


bool vk_load_shader_module( vk_shader_module_create_t* module_creates, VkPipelineShaderStageCreateInfo* stage_create, VkShaderModule** shader_modules_ptr, u32 count );


bool vk_load_test_shader()
{
	// create the pipeline layout for this shader
	VkPushConstantRange push_constant{};
	push_constant.offset     = 0;
	push_constant.size       = sizeof( test_compute_push_t );
	push_constant.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

	VkPipelineLayoutCreateInfo compute_layout{ VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };
	compute_layout.pSetLayouts            = &g_vk_desc_draw_image_layout;
	compute_layout.setLayoutCount         = 1;
	compute_layout.pPushConstantRanges    = &push_constant;
	compute_layout.pushConstantRangeCount = 1;

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


void vk_shader_destroy_test()
{
	vkDestroyPipeline( g_vk_device, g_pipeline_gradient, nullptr );
	vkDestroyPipelineLayout( g_vk_device, g_pipeline_gradient_layout, nullptr );
	vkDestroyShaderModule( g_vk_device, g_shader_module_gradient, nullptr );
}

