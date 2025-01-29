#include "render.h"


constexpr const char* STANDARD_SHADER_PATH_V = "shaders/render3/standard.vert.spv";
constexpr const char* STANDARD_SHADER_PATH_F = "shaders/render3/standard.frag.spv";


// struct r_standard_push_data_t
// {
// 	glm::mat4 mvp;
// };


void r_standard_push_constants( VkCommandBuffer cmd, VkPipelineLayout layout, const vk_shader_push_data_t& push_data )
{
}


static VkDynamicState g_standard_dynamic_states[] = {
	VK_DYNAMIC_STATE_VIEWPORT,
	VK_DYNAMIC_STATE_SCISSOR,
};


static VkFormat g_standard_color_formats[] = {
	g_draw_format,
};


static vk_shader_create_graphics_t g_shader_standard_create
{
	.name    = "standard",
	.modules = {
	  { VK_SHADER_STAGE_VERTEX_BIT, STANDARD_SHADER_PATH_V, "main" },
	  { VK_SHADER_STAGE_FRAGMENT_BIT, STANDARD_SHADER_PATH_F, "main" },
	},

	.topology            = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
	.cull_mode           = VK_CULL_MODE_NONE,
	.polygon_mode        = VK_POLYGON_MODE_FILL,

	.dynamic_state       = g_standard_dynamic_states,
	.dynamic_state_count = ARR_SIZE( g_standard_dynamic_states ),

	.color_attach_count  = 1,
	.color_attach        = g_standard_color_formats,
	// .depth_attach        = VK_FORMAT_UNDEFINED, //VK_FORMAT_D32_SFLOAT,
	.depth_attach        = VK_FORMAT_D32_SFLOAT,

	.fn_push_constants   = r_standard_push_constants,
	.push_constant_range = {
	  .stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
	  .offset     = 0,
	  .size       = sizeof( gpu_push_t ),
	},
};


CH_SHADER_REGISTER_GRAPHICS( g_shader_standard_create );
