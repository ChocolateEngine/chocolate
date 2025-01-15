#include "graphics_data.h"


// Ref Counters
std::unordered_map< ch_model_h, u32 >          g_model_ref;
// std::unordered_map< ch_material_h, u32 >       g_materials_ref;

// Path to Handle
std::unordered_map< ch_string, ch_model_h >    g_model_paths;
std::unordered_map< ch_string, ch_material_h > g_materials_paths;

// Handle Arrays
// ch_handle_array_32_t< ch_model_h, model_t >    g_model_handles;

// Maps handle to index of array for models (not size_t, as ch_model_h is a 32 bit handle
std::unordered_map< ch_model_h, model_t >      g_model_handles;
// ChVector< model_t >                            g_models;


class GraphicsData : public IGraphicsData
{
   public:
	// --------------------------------------------------------------------------------------------
	// General
	// --------------------------------------------------------------------------------------------


	// Creates a new frustum
	frustum_t frustum_create( const glm::mat4& proj_view ) override
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
	// Materials
	// --------------------------------------------------------------------------------------------

	// --------------------------------------------------------------------------------------------
	// Models
	// --------------------------------------------------------------------------------------------

	ch_model_h model_load( const char* path ) override
	{
		if ( path == nullptr )
		{
			Log_Error( "Path is null\n" );
			return CH_INVALID_HANDLE;
		}

		if ( !ch_str_ends_with( path, ".obj", 4 ) )
		{
			Log_ErrorF( "We only support OBJ models\n" );
			return CH_INVALID_HANDLE;
		}

		// Normalize Path Separators
		ch_string clean_path = FileSys_CleanPath( path );

		auto      path_it    = g_model_paths.find( clean_path );
		if ( path_it != g_model_paths.end() )
		{
			// This Model is already loaded, increase the ref count of it, and return the existing handle
			auto ref_it = g_model_ref.find( path_it->second );
			if ( ref_it == g_model_ref.end() )
			{
				Log_ErrorF( "Failed to find stored ref count for model \"%s\"\n", clean_path.data );
				g_model_ref[ path_it->second ] = 1;
			}
			else
			{
				ref_it->second++;
			}

			ch_str_free( clean_path );
			return path_it->second;
		}

		ch_string_auto full_path = FileSys_FindFile( clean_path.data, clean_path.size );

		if ( !full_path.data )
		{
			Log_ErrorF( "Failed to find model \"%s\"\n", clean_path.data );
			ch_str_free( clean_path );
			return CH_INVALID_HANDLE;
		}

		// Now try to load the model

		// create a new handle
		ch_model_h handle = util_create_handle_u32();
		model_t&   model  = g_model_handles[ handle ];

		if ( !model_load_obj( clean_path.data, full_path.data, model ) )
		{
			Log_ErrorF( "Failed to load model \"%s\"\n", full_path.data );
			ch_str_free( clean_path );
			//g_model_handles.erase( handle );
			return CH_INVALID_HANDLE;
		}

		// Store the model path and set the ref count to 1
		g_model_paths[ clean_path ] = handle;
		g_model_ref[ handle ]       = 1;

		return handle;
	}

	ch_model_h* model_load( const char** paths, size_t count = 1 ) override
	{
		if ( paths == nullptr )
			return nullptr;

		ch_model_h* handles = ch_calloc< ch_model_h >( count );

		for ( size_t i = 0; i < count; i++ )
			handles[ i ] = model_load( paths[ i ] );

		return handles;
	}

	void model_free( ch_model_h handle )
	{
	}

	model_t* model_get( ch_model_h handle ) override
	{
		auto search = g_model_handles.find( handle );

		if ( search == g_model_handles.end() )
			return nullptr;

		return &search->second;
	}

	const char* model_get_path( ch_model_h handle ) override
	{
		// TODO: replace this hash map system with the generation handle system
		return nullptr;

		//auto search = g_model_paths.find( handle );
		//
		//if ( search == g_model_handles.end() )
		//	return nullptr;
		//
		//return &search->second;
	}

	// const u16 model_get_material_count( ch_model_h handle ) override
	// {
	// 	return 0;
	// }
	// 
	// const ch_material_h* model_get_material_array( ch_model_h handle ) override
	// {
	// 	return nullptr;
	// }
};


GraphicsData g_graphics_data;


static ModuleInterface_t g_interfaces[] = {
	{ &g_graphics_data, CH_GRAPHICS_DATA, CH_GRAPHICS_DATA_VER }
};


extern "C"
{
	DLL_EXPORT ModuleInterface_t* ch_get_interfaces( u8& count )
	{
		count = 1;
		return g_interfaces;
	}
}

