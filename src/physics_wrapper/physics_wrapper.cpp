#include "core/commandline.h"


constexpr const char* gModuleMIN          = "ch_physics_min";
constexpr const char* gModuleAVX2         = "ch_physics_avx2";

bool                  gForceLoadMinModule = Args_Register( "Load the physics dll compiled with the minimum cpu features", "-phys-min" );
bool                  gForceLoadMaxModule = Args_Register( "Load the physics dll compiled with all cpu features it can use", "-phys-max" );


ModuleInterface_t* LoadPhysicsModule( const char* moduleName, size_t& count )
{
	Log_MsgF( "Loading Physics Module: %s\n", moduleName );

	Module phys_module = Mod_Load( moduleName );
	count              = 0;

	if ( !phys_module )
		return nullptr;

	ModuleInterface_t* ( *getInterfacesF )( size_t& srCount ) = nullptr;
	if ( !( *(void**)( &getInterfacesF ) = sys_load_func( phys_module, "cframework_GetInterfaces" ) ) )
	{
		Log_ErrorF( "Unable to load \"cframework_GetInterfaces\" function from library: %s - %s\n", moduleName, sys_get_error() );
		return nullptr;
	}

	return getInterfacesF( count );
}


extern "C"
{
	DLL_EXPORT ModuleInterface_t* cframework_GetInterfaces( size_t& srCount )
	{
		if ( gForceLoadMinModule )
		{
			return LoadPhysicsModule( gModuleMIN, srCount );
		}

		if ( gForceLoadMaxModule )
		{
			return LoadPhysicsModule( gModuleAVX2, srCount );
		}

		// check what cpu features we have
		cpu_info_t cpu_info = Sys_GetCpuInfo();

		if ( cpu_info.features & ECPU_Feature_AVX2 )
			return LoadPhysicsModule( gModuleAVX2, srCount );
		
		return LoadPhysicsModule( gModuleMIN, srCount );
	}
}

