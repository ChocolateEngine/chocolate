#include "core/core.h"
#include "system.h"

#include <set>

LOG_REGISTER_CHANNEL2( Module, LogColor::Default );

static std::vector< ISystem* > gSystems;

static std::unordered_map< const char*, Module > gModuleHandles;
static std::vector< ModuleInterface_t* >         gInterfaces;


bool Mod_InitSystems()
{
	// load saved cvar values and other stuff
	core_post_load();

	for ( ISystem* sys : gSystems )
	{
		if ( sys->Init() )
			continue;

		// dumb
		for ( const auto& interface : gInterfaces )
		{
			if ( sys == interface->apInterface )
			{
				Log_ErrorF( "Failed to Init System: %s\n", interface->apName );
				return false;
			}
		}

		Log_Error( "Failed to Init System!\n" );
		return false;
	}

	// Register the convars that are declared.
	Con_RegisterConVars();

	return true;
}


void Mod_Shutdown()
{
	for ( const auto& [ name, module ] : gModuleHandles )
	{
		sys_close_library( module );
	}
}


bool Mod_Load( const char* spPath )
{
	auto it = gModuleHandles.find( spPath );
	if ( it != gModuleHandles.end() )
		return true;  // already loaded

	std::string pathExt = spPath;
	pathExt += EXT_DLL;

	std::string path = FileSys_FindFile( pathExt, true );

	if ( path.empty() )
	{
		Log_WarnF( gLC_Module, "No module named %s found in search paths\n", pathExt.c_str() );
		return false;
	}

	Module handle = sys_load_library( path.c_str() );
	if ( !handle )
	{
		Log_ErrorF( gLC_Module, "Unable to load library: %s\n", pathExt.c_str(), sys_get_error() );
		return false;
	}

	gModuleHandles[ spPath ] = handle;

	ModuleInterface_t* ( *getInterfacesF )( size_t & srCount ) = nullptr;
	if ( !( *(void**)( &getInterfacesF ) = sys_load_func( handle, "cframework_GetInterfaces" ) ) )
	{
		Log_ErrorF( gLC_Module, "Unable to load \"cframework_GetInterfaces\" function from library: %s - %s\n", pathExt.c_str(), sys_get_error() );
		return false;
	}

	size_t             count      = 0;
	ModuleInterface_t* interfaces = getInterfacesF( count );

	if ( interfaces == nullptr || count == 0 )
	{
		Log_ErrorF( "Failed to Load Interfaces From Library: %s\n", pathExt.c_str() );
		return false;
	}

	Log_DevF( gLC_Module, 1, "Loading Module: %s\n", pathExt.c_str() );
	for ( size_t i = 0; i < count; i++ )
	{
		gInterfaces.push_back( &interfaces[ i ] );
		Log_DevF( gLC_Module, 1, "    Found Interface: %s - Version %zd\n", interfaces[ i ].apName, interfaces[ i ].aHash );
	}

	return true;
}


bool Mod_AddSystems( AppModules_t* spModules, size_t sCount )
{
	// Grab the systems from libraries.
	std::set< const char* > failed;

	for ( size_t i = 0; i < sCount; i++ )
	{
		if ( !Mod_Load( spModules[ i ].apModuleName ) )
		{
			failed.emplace( spModules[ i ].apModuleName );
			Log_ErrorF( gLC_Module, "Failed to load module: %s\n", spModules[ i ].apModuleName );
			continue;
		}

		// add system we want from module
		void* system = Mod_GetInterface( spModules[ i ].apInterfaceName, spModules[ i ].apInterfaceHash );
		if ( system == nullptr )
		{
			Log_ErrorF( gLC_Module, "Failed to load system from module: %s - %s\n", spModules[ i ].apModuleName, spModules[ i ].apInterfaceName );
			continue;
		}

		*spModules[ i ].apSystem = system;
		gSystems.push_back( static_cast< ISystem* >( system ) );
	}

	if ( failed.size() )
	{
		std::string msg;

		for ( const auto& str : failed )
		{
			msg += "\n\t";
			msg += str;
		}

		Log_FatalF( gLC_Module, "Failed to load modules:%s\n", msg.c_str() );
		Mod_Shutdown();
		return false;
	}

	return true;
}


void Mod_AddLoadedSystem( ISystem* spSystem )
{
	if ( !spSystem )
		return;

	gSystems.push_back( spSystem );
}


void* Mod_GetInterface( const char* spName, size_t sHash )
{
	for ( const auto& interface : gInterfaces )
	{
		if ( strcmp( interface->apName, spName ) != 0 )
		{
			continue;
		}

		if ( interface->aHash != sHash )
		{
			Log_ErrorF( gLC_Module, "Found Interface \"%s\" but hash is different: App Hash: %zd - Module Hash: %zd\n", spName, sHash, interface->aHash );
			continue;
		}

		return interface->apInterface;
	}

	Log_ErrorF( gLC_Module, "Failed to find interface: %s\n", spName );
	return nullptr;
}

