#include "render.h"


ch_handle_ref_list_32< vk_material_h, vk_material_t > g_materials;

std::unordered_map< ch_material_h, vk_material_h >    g_material_map;
std::unordered_map< ch_string, r_texture_h >          g_texture_paths;


vk_material_h vk_material_create( ch_material_h base_handle )
{
	// Search for this material
	// if not found, copy it to a vk_material_t
	// Then upload any textures found in it based on the shader

	// kind of a lot to do while actively loading a model,
	// surely we can technically do all at the same time with job systems or something?

	auto it = g_material_map.find( base_handle );
	if ( it != g_material_map.end() )
	{
		// increment ref
		return it->second;
	}

	// create a new material
	vk_material_h  handle{};
	vk_material_t* material{};
	if ( !g_materials.create( handle, &material ) )
	{
		Log_Error( "Failed to create vulkan material\n" );
		return {};
	}

	// copy material data
	vk_shaders_material_update( handle );

	g_material_map[ base_handle ] = handle;

	return handle;
}


void vk_material_free()
{
	// decrement ref and free it if 0
}


void vk_material_update()
{
	// check graphics api for dirty materials, and update the vk materials if any changed this frame
}


vk_material_h vk_material_find( ch_material_h base_handle )
{
	auto it = g_material_map.find( base_handle );
	if ( it != g_material_map.end() )
	{
		return it->second;
	}

	return {};
}

