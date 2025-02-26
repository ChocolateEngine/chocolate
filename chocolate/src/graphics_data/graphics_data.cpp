#include "graphics_data.h"

LOG_CHANNEL_REGISTER( GraphicsData, ELogColor_Purple );

// Path to Handle
std::unordered_map< ch_string, ch_model_h >        g_model_paths;
std::unordered_map< ch_string, ch_material_h >     g_material_paths;
std::unordered_map< ch_string, ch_material_h >     g_material_names;

// Handle Arrays
// ch_handle_array_32_t< ch_model_h, model_t >    g_model_handles;

ch_handle_ref_list_32< ch_model_h, model_t >       g_models;
ch_handle_ref_list_32< ch_material_h, material_t > g_materials;


// --------------------------------------------------------------------------------------------
// General
// --------------------------------------------------------------------------------------------


// Creates a new frustum
frustum_t GraphicsData::frustum_create( const glm::mat4& proj_view ) 
{
	PROF_SCOPE();

	frustum_t frustum;

	glm::mat4 m   = glm::transpose( proj_view );
	glm::mat4 inv = glm::inverse( proj_view );

	frustum.planes[ e_frustum_left ]   = m[ 3 ] + m[ 0 ];
	frustum.planes[ e_frustum_right ]  = m[ 3 ] - m[ 0 ];
	frustum.planes[ e_frustum_bottom ] = m[ 3 ] + m[ 1 ];
	frustum.planes[ e_frustum_top ]    = m[ 3 ] - m[ 1 ];
	frustum.planes[ e_frustum_near ]   = m[ 3 ] + m[ 2 ];
	frustum.planes[ e_frustum_far ]    = m[ 3 ] - m[ 2 ];

	// Calculate Frustum Points
	for ( int i = 0; i < 8; i++ )
	{
		glm::vec4 ff          = inv * gFrustumFaceData[ i ];
		frustum.points[ i ].x = ff.x / ff.w;
		frustum.points[ i ].y = ff.y / ff.w;
		frustum.points[ i ].z = ff.z / ff.w;
	}

	return frustum;
}


// --------------------------------------------------------------------------------------------
// Models
// --------------------------------------------------------------------------------------------


ch_model_h GraphicsData::model_load( const char* path ) 
{
	if ( path == nullptr )
	{
		Log_Error( "Path is null\n" );
		return {};
	}

	if ( !ch_str_ends_with( path, ".obj", 4 ) )
	{
		Log_ErrorF( "We only support OBJ models\n" );
		return {};
	}

	// Normalize Path Separators
	ch_string clean_path = FileSys_CleanPath( path );
	auto      path_it    = g_model_paths.find( clean_path );

	if ( path_it != g_model_paths.end() )
	{
		// This Model is already loaded, increase the ref count of it, and return the existing handle
		if ( g_models.handle_valid( path_it->second ) )
		{
			g_models.ref_increment( path_it->second );
			ch_str_free( clean_path );
			return path_it->second;
		}
	}

	ch_string_auto full_path = FileSys_FindFile( clean_path.data, clean_path.size );

	if ( !full_path.data )
	{
		// try appending models to it, a hack that won't work if it starts with ".." or points elsewhere usually
		if ( FileSys_IsRelative( clean_path.data ) )
		{
			char new_path[ 512 ]{};
			strcat( new_path, "models" );
			strcat( new_path, SEP );
			strcat( new_path, clean_path.data );

			full_path = FileSys_FindFile( new_path );
		}
	}

	if ( !full_path.data )
	{
		Log_ErrorF( "Failed to find model \"%s\"\n", clean_path.data );
		ch_str_free( clean_path );
		return {};
	}

	// Now try to load the model

	// create a new handle
	ch_model_h handle{};
	model_t*   model  = nullptr;

	if ( !g_models.create( handle, &model ) )
	{
		Log_ErrorF( "Failed to create model handle for \"%s\"\n", clean_path.data );
		return {};
	}

	if ( !model_load_obj( clean_path.data, full_path.data, *model ) )
	{
		Log_ErrorF( "Failed to load model \"%s\"\n", full_path.data );
		ch_str_free( clean_path );
		//g_model_handles.erase( handle );
		return {};
	}

	// Store the model path and set the ref count to 1
	g_model_paths[ clean_path ] = handle;

	return handle;
}


ch_model_h* GraphicsData::model_load( const char** paths, size_t count ) 
{
	if ( paths == nullptr )
		return nullptr;

	ch_model_h* handles = ch_calloc< ch_model_h >( count );

	for ( size_t i = 0; i < count; i++ )
		handles[ i ] = model_load( paths[ i ] );

	return handles;
}


void GraphicsData::model_free( ch_model_h handle )
{
	g_models.ref_decrement( handle );
}


model_t* GraphicsData::model_get( ch_model_h handle ) 
{
	return g_models.get( handle );
}


ch_string GraphicsData::model_get_path( ch_model_h handle ) 
{
	for ( const auto& [ path, saved_handle ] : g_model_paths )
	{
		if ( saved_handle == handle )
			return path;
	}

	return {};

	// TODO: replace this hash map system with the generation handle system
	//  return nullptr;

	//auto search = g_model_paths.find( handle );
	//
	//if ( search == g_model_handles.end() )
	//	return nullptr;
	//
	//return &search->second;
}


// const u16 GraphicsData::model_get_material_count( ch_model_h handle ) 
// {
// 	return 0;
// }
// 
// const ch_material_h* GraphicsData::model_get_material_array( ch_model_h handle ) 
// {
// 	return nullptr;
// }


GraphicsData             graphics_data;


static ModuleInterface_t g_interfaces[] = {
	{ &graphics_data, CH_GRAPHICS_DATA, CH_GRAPHICS_DATA_VER }
};


extern "C"
{
	DLL_EXPORT ModuleInterface_t* ch_get_interfaces( u8& count )
	{
		count = 1;
		return g_interfaces;
	}
}

