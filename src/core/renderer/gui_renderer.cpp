#include "../../../inc/core/renderer/gui_renderer.h"

#include "../../../inc/imgui/imgui.h"

void gui_renderer_c::init
	(  )
{
	unsigned char* fontData;
	int texWidth, texHeight;
	ImGui::CreateContext(  );
	ImGui::GetIO(  ).Fonts->GetTexDataAsRGBA32( &fontData, &texWidth, &texHeight );
	allocator.init_image( 800, 800, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, fontImage, fontMemory );
	allocator.init_image_view( fontView, fontImage, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_ASPECT_COLOR_BIT );
	device->init_texture_sampler( sampler, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE );
	allocator.init_desc_pool( descriptorPool, { { { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1 } } } );
	allocator.init_desc_set_layout( descriptorSetLayout, { { allocator.layout_binding( VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 0 ) } } );
	allocator.init_desc_set( allocator.allocate_info( descriptorPool, &descriptorSetLayout, 1 ), allocator.image_info( sampler, fontView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL ), descriptorSet! );
}
