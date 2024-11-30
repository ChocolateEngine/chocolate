#include "render.h"


constexpr const char* STANDARD_SHADER_PATH_V = "shaders/render3/standard.vert.spv";
constexpr const char* STANDARD_SHADER_PATH_F = "shaders/render3/standard.frag.spv";


struct r_standard_push_data_t
{
	glm::mat4 mvp;
};


void r_standard_push_constants( VkCommandBuffer cmd, VkPipelineLayout layout, const vk_shader_push_data_t& push_data )
{

}


VkDynamicState g_standard_dynamic_states[] = {
	VK_DYNAMIC_STATE_VIEWPORT,
	VK_DYNAMIC_STATE_SCISSOR,
};


vk_shader_create_graphics_t g_shader_standard_create
{
	.name    = "standard",
	.modules = {
	  { VK_SHADER_STAGE_VERTEX_BIT, STANDARD_SHADER_PATH_V, "main" },
	  { VK_SHADER_STAGE_FRAGMENT_BIT, STANDARD_SHADER_PATH_F, "main" },
	},

	.topology 	         = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
	.cull_mode 	         = VK_CULL_MODE_BACK_BIT,

	.dynamic_state       = g_standard_dynamic_states,
	.dynamic_state_count = ARR_SIZE( g_standard_dynamic_states ),

	.fn_push_constants   = r_standard_push_constants,
	.push_constant_range = {
	  .stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
	  .offset     = 0,
	  .size       = sizeof( r_standard_push_data_t ),
	}
};


CH_SHADER_REGISTER_GRAPHICS( g_shader_standard_create );

