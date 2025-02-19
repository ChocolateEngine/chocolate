#include "graphics_data.h"

#include <unordered_set>


// Materials updated this frame
ChVector< ch_material_h >            g_materials_dirty;


// stored pointers to material var names
std::unordered_map< u64, ch_string > g_material_var_names;


#if 0
// store list of material var strings, so we don't have 1000 duplicated strings
// also stored as a u16 since i don't think we would have over 65k variable names
ch_string*                g_material_var_name;
u16                       g_material_var_name_count;

// std::unordered_map< ch_string, ch_material_h > g_material_var_names;


ch_string* GraphicsData::material_get_var_names( u16 var_name_index )
{
	return g_material_var_name;
}


u16 GraphicsData::material_get_var_names_count()
{
	return g_material_var_name_count;
}


ch_string GraphicsData::material_get_var_name( u16 var_name_index )
{
	if ( var_name_index > g_material_var_name_count )
		return {};

	return g_material_var_name[ var_name_index ];
}


u16 GraphicsData::material_get_var_name_index( const char* name, size_t len )
{
	for ( u16 i = 0; i < g_material_var_name_count; i++ )
		if ( ch_str_equals( g_material_var_name[ i ], name, len ) )
			return i;

	return UINT16_MAX;
}


static u16 material_add_var_name( const char* name, size_t len )
{
	if ( util_array_append( g_material_var_name, 1 ) )
		return UINT16_MAX;

	g_material_var_name[ g_material_var_name_count ] = ch_str_copy( name, len );
	return g_material_var_name_count++;
}
#endif


// ----------------------------------------------------------------------------------


ch_string GraphicsData::material_parse_name( const char* path, size_t len )
{
	if ( len == 0 )
		return {};

	size_t path_no_ext_len = len;

	// Remove .cmt from the extension, no need for this
	if ( ch_str_ends_with( path, len, ".cmt", 4 ) )
		path_no_ext_len -= 4;

	// Normalize Path Separators
	return FileSys_CleanPath( path, path_no_ext_len );
}


ch_material_h GraphicsData::material_load( const char* path )
{
	Log_Error( gLC_GraphicsData, "Material Loading from file is not implemented yet\n" );
	return {};
}


ch_material_h GraphicsData::material_create( const char* name, const char* shader_name )
{
	ch_material_h handle{};
	material_t*   material = nullptr;

	if ( !name || !shader_name )
	{
		Log_Error( gLC_GraphicsData, "Failed to create material - No Name or Shader specified\n" );
		return {};
	}

	if ( !g_materials.create( handle, &material ) )
	{
		Log_Error( gLC_GraphicsData, "Failed to create material\n" );
		return {};
	}

	if ( !material_set_string( handle, "shader", shader_name ) )
	{
		Log_Error( gLC_GraphicsData, "Failed to create material\n" );
		g_materials.ref_decrement( handle );
		return {};
	}

	ch_string final_name           = material_parse_name( name, strlen( name ) );
	//g_material_names[ final_name ] = handle;

	return handle;
}


void GraphicsData::material_free( ch_material_h handle )
{
	material_t* material = g_materials.get( handle );

	if ( !material )
		return;

	if ( g_materials.ref_decrement_delay( handle ) > 0 )
		return;

	// free the strings in the material vars
	for ( size_t i = 0; i < material->count; i++ )
	{
		if ( material->type[ i ] == e_mat_var_string )
		{
			ch_str_free( material->var[ i ].val_string );
		}
	}

	free( material->name );
	free( material->var );
	free( material->type );

	g_materials.ref_decrement_finish( handle );
}


material_t* GraphicsData::material_get_data( ch_material_h handle )
{
	return g_materials.get( handle );
}


// ----------------------------------------------------------------------------------
// Modified Materials


size_t GraphicsData::material_get_dirty_count()
{
	return g_materials_dirty.size();
}


ch_material_h GraphicsData::material_get_dirty( size_t index )
{
	if ( index > g_materials_dirty.size() )
		return {};

	return g_materials_dirty[ index ];
}


void GraphicsData::material_mark_dirty( ch_material_h handle )
{
	if ( g_materials_dirty.index( handle ) == UINT32_MAX )
	//	g_materials_dirty.push_back( handle );
		return;
}


// ----------------------------------------------------------------------------------
// Memory Allocation


bool material_data_resize( material_t* material, size_t count )
{
	auto new_var = ch_realloc( material->var, count );
	auto new_name = ch_realloc( material->name, count );
	auto new_type = ch_realloc( material->type, count );

	if ( !new_var || !new_name || !new_type )
	{
		Log_Error( gLC_GraphicsData, "Failed to reallocate data for material\n" );
		return false;
	}

	material->var  = new_var;
	material->name = new_name;
	material->type = new_type;

	return true;
}


bool material_data_extend( material_t* material, size_t count )
{
	if ( util_array_extend( material->var, material->count, count ) )
	{
		Log_Error( gLC_GraphicsData, "Failed to allocate more data for material\n" );
		return false;
	}

	if ( util_array_extend( material->name, material->count, count ) )
	{
		Log_Error( gLC_GraphicsData, "Failed to allocate more data for material\n" );
		return false;
	}

	if ( util_array_extend( material->type, material->count, count ) )
	{
		Log_Error( gLC_GraphicsData, "Failed to allocate more data for material\n" );
		return false;
	}

	return true;
}


bool material_data_reserve( material_t* material, size_t count )
{
	if ( count <= material->count )
		return true;

	return material_data_extend( material, count - material->count );
}


// ----------------------------------------------------------------------------------
// Variable Assignment


#if 0
u16 material_handle_var_name_index( const char* var_name )
{
	size_t var_name_len   = strlen( var_name );
	u16    var_name_index = graphics_data.material_get_var_name_index( var_name, var_name_len );

	if ( var_name_index == UINT16_MAX )
		var_name_index = material_add_var_name( var_name, var_name_len );

	return var_name_index;
}
#endif


static ch_string material_get_var_name_ptr( const char* var_name )
{
	size_t var_name_len = strlen( var_name );
	u64    hash         = ch_str_hash( var_name, var_name_len );
	auto   find         = g_material_var_names.find( hash );

	if ( find == g_material_var_names.end() )
	{
		ch_string new_string = ch_str_copy( var_name, var_name_len );
		g_material_var_names[ hash ] = new_string;
		return new_string;
	}

	return find->second;
}


#if 0
ch_string material_handle_var_name( const char* var_name )
{
	size_t var_name_len = strlen( var_name );
	u64    hash         = ch_str_hash( var_name, var_name_len );
	auto   find         = g_material_var_names.find( hash );

	if ( find == g_material_var_names.end() )
	{
		ch_string new_string         = ch_str_copy( var_name, var_name_len );
		g_material_var_names[ hash ] = new_string;
		return new_string;
	}

	return *find;
}
#endif


bool GraphicsData::material_set_string( ch_material_h handle, const char* var_name, const char* value )
{
	material_t* material = g_materials.get( handle );

	if ( !material )
	{
		Log_Error( gLC_GraphicsData, "material_set_string: Failed to find material\n" );
		return false;
	}

	ch_string var_name_ptr = material_get_var_name_ptr( var_name );

//	#error get existing material var index

	if ( !material_data_extend( material, 1 ) )
	{
		Log_Error( gLC_GraphicsData, "material_set_string: Failed to allocate data for string var on material\n" );
		return false;
	}

	material->name[ material->count ]           = var_name_ptr;
	material->var[ material->count ].val_string = ch_str_copy( value );
	material->type[ material->count ]           = e_mat_var_string;

	material->count++;
	material_mark_dirty( handle );
	return true;
}


bool GraphicsData::material_set_float( ch_material_h handle, const char* var_name, float value )
{
	material_t* material = g_materials.get( handle );

	if ( !material )
	{
		Log_Error( gLC_GraphicsData, "material_set_float: Failed to find material\n" );
		return false;
	}

	ch_string var_name_ptr = material_get_var_name_ptr( var_name );

	if ( !material_data_extend( material, 1 ) )
	{
		Log_Error( gLC_GraphicsData, "Failed to allocate data for float var on material\n" );
		return false;
	}

	material->name[ material->count ]          = var_name_ptr;
	material->var[ material->count ].val_float = value;
	material->type[ material->count ]          = e_mat_var_float;

	material->count++;
	material_mark_dirty( handle );
	return true;
}


// ----------------------------------------------------------------------------------
// Variable Getting


size_t material_get_var_index( material_t* material, const char* var_name, e_mat_var type )
{
	size_t var_name_len   = strlen( var_name );

	// find the index in the var name array
	for ( size_t i = 0; i < material->count; i++ )
	{
		if ( !ch_str_equals( material->name[ i ], var_name, var_name_len ) )
			continue;

		if ( material->type[ i ] != type )
		{
			Log_ErrorF( gLC_GraphicsData, "Material variable \"%s\" is not the correct type\n", var_name );
			return SIZE_MAX;
		}

		return i;
	}

	return SIZE_MAX;
}


ch_string GraphicsData::material_get_string( ch_material_h handle, const char* var_name, const char* fallback )
{
	material_t* material = g_materials.get( handle );

	if ( !material )
	{
		Log_Error( gLC_GraphicsData, "material_get_string: Failed to find material\n" );
		return ch_string( (char*)fallback, strlen( fallback ) );
	}

	size_t var_index = material_get_var_index( material, var_name, e_mat_var_string );

	if ( var_index == UINT16_MAX )
	{
		Log_ErrorF( gLC_GraphicsData, "material_get_string: Failed to find variable \"%s\"\n", var_name );
		return ch_string( (char*)fallback, strlen( fallback ) );
	}

	return material->var[ var_index ].val_string;
}


float GraphicsData::material_get_float( ch_material_h handle, const char* var_name, float fallback )
{
	material_t* material = g_materials.get( handle );

	if ( !material )
	{
		Log_Error( gLC_GraphicsData, "material_get_float: Failed to find material\n" );
		return fallback;
	}

	size_t var_index = material_get_var_index( material, var_name, e_mat_var_float );

	if ( var_index == UINT16_MAX )
	{
		Log_ErrorF( gLC_GraphicsData, "material_get_float: Failed to find variable \"%s\"\n", var_name );
		return fallback;
	}

	return material->var[ var_index ].val_float;
}


// material_get_var_array();