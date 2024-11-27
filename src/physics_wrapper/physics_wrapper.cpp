#include "core/commandline.h"


constexpr const char* g_module_min            = "ch_physics_min";
constexpr const char* g_module_avx2           = "ch_physics_avx2";

bool                  g_force_load_module_min = args_register( "Load the physics dll compiled with the minimum cpu features", "--phys-min" );
bool                  g_force_load_module_max = args_register( "Load the physics dll compiled with all cpu features it can use", "--phys-max" );


ModuleInterface_t* load_physics_module( const char* moduleName, u8& count )
{
	Log_MsgF( "Loading Physics Module: %s\n", moduleName );

	Module phys_module = Mod_Load( moduleName );
	count              = 0;

	if ( !phys_module )
		return nullptr;

	ModuleInterface_t* ( *ch_get_interfaces )( u8& srCount ) = nullptr;
	if ( !( *(void**)( &ch_get_interfaces ) = sys_load_func( phys_module, "ch_get_interfaces" ) ) )
	{
		Log_ErrorF( "Unable to load \"ch_get_interfaces\" function from library: %s - %s\n", moduleName, sys_get_error() );
		return nullptr;
	}

	return ch_get_interfaces( count );
}


extern "C"
{
	DLL_EXPORT ModuleInterface_t* ch_get_interfaces( u8& srCount )
	{
		if ( g_force_load_module_min )
		{
			return load_physics_module( g_module_min, srCount );
		}

		if ( g_force_load_module_max )
		{
			return load_physics_module( g_module_avx2, srCount );
		}

		// check what cpu features we have
		bool use_avx2 = SDL_HasAVX2();

		// the avx2 dll also uses F16C, and min is not compiled with that
		// so we need to check for that too

		cpu_info_t cpu_info = sys_get_cpu_info();

		if ( cpu_info.features & ( ECPU_Feature_AVX2 | ECPU_Feature_F16C ) )
			return load_physics_module( g_module_avx2, srCount );
		
		return load_physics_module( g_module_min, srCount );
	}
}

