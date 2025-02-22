#include "render.h"


ch_handle_ref_list_32< vk_material_h, vk_material_t > g_materials;

// index of ch_material_h points to an entry here
ChVector< vk_material_h >                             g_material_to_vk;


vk_material_h vk_material_create( ch_material_h base_handle )
{
	// Search for this material
	// if not found, copy it to a vk_material_t
	// Then upload any textures found in it based on the shader

	// kind of a lot to do while actively loading a model,
	// surely we can technically do all at the same time with job systems or something?

	if ( base_handle.index < g_material_to_vk.size() )
	{
		if ( g_materials.handle_valid( g_material_to_vk[ base_handle.index ] ) )
		{
			// material found, increase ref count since we want to create a new one
			graphics_data->material_ref_increment( base_handle );
			g_materials.ref_increment( g_material_to_vk[ base_handle.index ] );
			return g_material_to_vk[ base_handle.index ];
		}
	}

	// create new material
	vk_material_h  handle{};
	vk_material_t* material{};
	if ( !g_materials.create( handle, &material ) )
	{
		Log_Error( "Failed to create vulkan material\n" );
		return {};
	}

	// u32 new_size = std::max( g_material_to_vk.size(), graphics_data->material_get_count() );

	g_material_to_vk.resize( graphics_data->material_get_capacity() );
	g_material_to_vk[ base_handle.index ] = handle;

	// set the shader to invalid
	material->shader                      = UINT32_MAX;

	// copy material data
	vk_shaders_material_update( base_handle, handle );

	return handle;
}


void vk_material_free( vk_material_h base_handle )
{
	// decrement ref and free it if 0
	Log_Warn( gLC_Render, "VULKAN MATERIAL FREEING NOT IMPLEMENTED YET !!!!\n" );
}


void vk_material_update()
{
	// check graphics api for dirty materials, and update the vk materials if any changed this frame
	for ( size_t i = 0; i < graphics_data->material_get_dirty_count(); i++ )
	{
		ch_material_h dirty_mat_base = graphics_data->material_get_dirty( i );

		if ( dirty_mat_base.index >= g_material_to_vk.size() )
			continue;

		if ( !g_materials.handle_valid( g_material_to_vk[ dirty_mat_base.index ] ) )
			continue;

		vk_shaders_material_update( dirty_mat_base, g_material_to_vk[ dirty_mat_base.index ] );
	}
}


vk_material_h vk_material_find( ch_material_h base_handle )
{
	if ( base_handle.index >= g_material_to_vk.size() )
	{
		Log_Error( gLC_Render, "Vulkan material not found\n" );
		return {};
	}

	if ( !g_materials.handle_valid( g_material_to_vk[ base_handle.index ] ) )
	{
		Log_Error( gLC_Render, "Vulkan material not found\n" );
		return {};
	}

	return g_material_to_vk[ base_handle.index ];
}


vk_material_t* vk_material_get( ch_material_h base_handle )
{
	if ( base_handle.index >= g_material_to_vk.size() )
	{
		Log_Error( gLC_Render, "Vulkan material not found\n" );
		return nullptr;
	}

	if ( !g_materials.handle_valid( g_material_to_vk[ base_handle.index ] ) )
	{
		Log_Error( gLC_Render, "Vulkan material not found\n" );
		return nullptr;
	}

	return g_materials.get( g_material_to_vk[ base_handle.index ] );
}


vk_material_t* vk_material_get( vk_material_h handle )
{
	return g_materials.get( handle );
}

