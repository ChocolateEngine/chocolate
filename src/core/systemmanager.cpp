#include "core/core.h"
#include "system.h"

#include <set>

LOG_REGISTER_CHANNEL2( Module, LogColor::Default );

// static std::vector< ISystem* > gSystems;

// struct ModuleSystem_t
// {
// 	ModuleInterface_t* apInterface = nullptr;
// 	ISystem*           apSystem    = nullptr;
// 	Module             aModule     = nullptr;
// };

struct LoadedSystem_t
{
	ISystem*    apSystem;
	Module      apModule;
	const char* apName = nullptr;
	size_t      apVer;
	bool        aRequired = true;
	bool        aRunning  = false;
};

static std::unordered_map< const char*, Module >        gModuleHandles;
static std::unordered_map< ModuleInterface_t*, Module > gInterfaces;
static std::vector< LoadedSystem_t >                    gLoadedSystems;
// static std::unordered_map< ISystem*, Module >           gSystems;

const u64 CH_PLAT_FOLDER_SEP_LEN = strlen( CH_PLAT_FOLDER_SEP );

bool Mod_InitSystem( LoadedSystem_t& appModule )
{
	CH_ASSERT( appModule.apSystem );

	if ( !appModule.apSystem )
		return false;

	if ( appModule.apSystem->Init() )
		return true;

	if ( appModule.aRequired )
		Log_ErrorF( "Failed to Init Required System: %s\n", appModule.apName );
	else
		Log_ErrorF( "Failed to Init Optional System: %s\n", appModule.apName );

	return false;
}


bool Mod_InitSystems()
{
	// load saved cvar values and other stuff
	core_post_load();

	for ( LoadedSystem_t& appModule : gLoadedSystems )
	{
		CH_ASSERT( appModule.apSystem );

		if ( !appModule.apSystem )
			continue;

		if ( appModule.apSystem->Init() )
		{
			appModule.aRunning = true;
			continue;
		}

		if ( appModule.aRequired )
		{
			Log_ErrorF( "Failed to Init Required System: %s\n", appModule.apName );
			return false;
		}
		else
		{
			Log_ErrorF( "Failed to Init Optional System: %s\n", appModule.apName );
		}
	}

	return true;
}


void Mod_Shutdown()
{
	if ( gLoadedSystems.empty() )
		return;

	// Go through the list in reverse, in order from what started up last to what started first
	for ( size_t i = gLoadedSystems.size() - 1; i > 0; i-- )
	{
		LoadedSystem_t& appModule = gLoadedSystems[ i ];

		if ( !appModule.apSystem )
			continue;

		if ( !appModule.aRunning )
			continue;

		appModule.apSystem->Shutdown();
		appModule.aRunning = false;
	}

	for ( const auto& [ name, module ] : gModuleHandles )
	{
		sys_close_library( module );
	}
}


Module Mod_Load( const char* spPath )
{
	auto it = gModuleHandles.find( spPath );
	if ( it != gModuleHandles.end() )
		return it->second;  // already loaded

	// TODO: improve this, what if it has a path before it
	const char*    strings[] = { CH_PLAT_FOLDER_SEP, spPath, EXT_DLL };
	const u64      lengths[] = { CH_PLAT_FOLDER_SEP_LEN, strlen( spPath ), EXT_DLL_LEN };
	ch_string_auto pathExt   = ch_str_concat( 3, strings, lengths );

	ch_string_auto path      = FileSys_FindFileEx( pathExt.data, pathExt.size, ESearchPathType_Binary );

	if ( !path.data )
	{
		Log_WarnF( gLC_Module, "No module named %s found in search paths\n", pathExt.data );
		return nullptr;
	}

	Module handle = sys_load_library( path.data );
	if ( !handle )
	{
		Log_ErrorF( gLC_Module, "Unable to load library: %s - %s\n", pathExt.data, sys_get_error() );
		return nullptr;
	}

	gModuleHandles[ spPath ] = handle;

	ModuleInterface_t* ( *getInterfacesF )( size_t & srCount ) = nullptr;
	if ( !( *(void**)( &getInterfacesF ) = sys_load_func( handle, "cframework_GetInterfaces" ) ) )
	{
		Log_ErrorF( gLC_Module, "Unable to load \"cframework_GetInterfaces\" function from library: %s - %s\n", pathExt.data, sys_get_error() );
		return nullptr;
	}

	size_t             count      = 0;
	ModuleInterface_t* interfaces = getInterfacesF( count );

	if ( interfaces == nullptr || count == 0 )
	{
		Log_ErrorF( "Failed to Load Interfaces From Library: %s\n", pathExt.data );
		return nullptr;
	}

	Log_DevF( gLC_Module, 1, "Loading Module: %s\n", pathExt.data );
	for ( size_t i = 0; i < count; i++ )
	{
		gInterfaces[ &interfaces[ i ] ] = handle;
		Log_DevF( gLC_Module, 1, "    Found Interface: %s - Version %zd\n", interfaces[ i ].apName, interfaces[ i ].aVersion );
	}

	return handle;
}


void Mod_Free( const char* spPath )
{
	auto it = gModuleHandles.find( spPath );
	if ( it == gModuleHandles.end() )
		return;  // Module was never loaded
	
	// Shutdown Systems loaded with this module
	
	sys_close_library( it->second );
	gModuleHandles.erase( it );
}


EModLoadError Mod_LoadSystem( AppModule_t& srModule )
{
	Module module = Mod_Load( srModule.apModuleName );
	if ( !module )
	{
		Log_ErrorF( gLC_Module, "Failed to load module: %s\n", srModule.apModuleName );
		return EModLoadError_LoadModule;
	}

	// Add the System we want from the Module
	void* system = Mod_GetInterface( srModule.apInterfaceName, srModule.apInterfaceVer );
	if ( system == nullptr )
	{
		Log_ErrorF( gLC_Module, "Failed to load system from module: %s - %s\n", srModule.apModuleName, srModule.apInterfaceName );
		return EModLoadError_LoadInterface;
	}

	*srModule.apSystem = static_cast< ISystem* >( system );
	// gSystems.push_back( static_cast< ISystem* >( system ) );

	LoadedSystem_t loadedModule{};
	loadedModule.apModule        = module;
	loadedModule.apSystem        = static_cast< ISystem* >( system );
	loadedModule.aRequired       = srModule.aRequired;
	loadedModule.apVer           = srModule.apInterfaceVer;

	// Copy the interface name
	size_t len                   = strlen( srModule.apInterfaceName );
	char*  name                  = new char[ len + 1 ];
	strncpy( name, srModule.apInterfaceName, len );
	name[ len ] = '\0';

	loadedModule.apName = name;

	gLoadedSystems.push_back( loadedModule );

	return EModLoadError_Success;
}


bool Mod_AddSystems( AppModule_t* spModules, size_t sCount )
{
	// Grab the systems from libraries.
	std::set< std::string > failed;

	for ( size_t i = 0; i < sCount; i++ )
	{
		EModLoadError err = Mod_LoadSystem( spModules[ i ] );

		if ( err == EModLoadError_LoadModule )
		{
			if ( spModules[ i ].aRequired )
				failed.emplace( spModules[ i ].apModuleName );
		}
		else if ( err == EModLoadError_LoadInterface )
		{
			if ( spModules[ i ].aRequired )
				failed.emplace( vstring( "%s - %s Version %zd", spModules[ i ].apModuleName, spModules[ i ].apInterfaceName, spModules[ i ].apInterfaceVer ) );
		}
	}

	if ( failed.size() )
	{
		std::string msg;

		for ( const auto& str : failed )
		{
			msg += "\n\t";
			msg += str;
		}

		Log_FatalF( gLC_Module, "Failed to load required modules or systems:%s\n", msg.c_str() );
		Mod_Shutdown();
		return false;
	}

	return true;
}


bool Mod_InitSystem( AppModule_t& srModule )
{
	// Dumb, find the AppLoadedModule_t struct we made so can initialize it
	for ( LoadedSystem_t& loadedSystem : gLoadedSystems )
	{
		if ( strcmp( loadedSystem.apName, srModule.apInterfaceName ) == 0 )
		{
			return Mod_InitSystem( loadedSystem );
		}
	}

	Log_ErrorF( gLC_Module, "Failed to find system to init: \"%s\"", srModule.apInterfaceName );
	return false;
}


EModLoadError Mod_LoadAndInitSystem( AppModule_t& srModule )
{
	EModLoadError err = Mod_LoadSystem( srModule );

	if ( err != EModLoadError_Success )
		return err;

	// Dumb, find the AppLoadedModule_t struct we made so can initialize it
	for ( LoadedSystem_t& loadedSystem : gLoadedSystems )
	{
		if ( strcmp( loadedSystem.apName, srModule.apInterfaceName ) == 0 )
		{
			Mod_InitSystem( loadedSystem );
			return EModLoadError_Success;
		}
	}

	return EModLoadError_InitInterface;
}


// void Mod_AddLoadedSystem( const char* spModule, ISystem* spSystem )
// {
// 	if ( !spSystem )
// 		return;
// 
// 	auto it = gModuleHandles.find( spPath );
// 	if ( it == gModuleHandles.end() )
// 		return;  // already loaded
// 
// 	gSystems.push_back( spSystem );
// }


void* Mod_GetInterface( const char* spName, size_t sVersion )
{
	if ( spName == nullptr )
	{
		Log_ErrorF( gLC_Module, "Interface Search is nullptr\n" );
		return nullptr;
	}

	for ( const auto& [ interface, module ] : gInterfaces )
	{
		if ( strcmp( interface->apName, spName ) != 0 )
		{
			continue;
		}

		if ( interface->aVersion != sVersion )
		{
			Log_ErrorF( gLC_Module, "Found Interface \"%s\" but version is different: App Version: %zd - Module Version: %zd\n", spName, sVersion, interface->aVersion );
			continue;
		}

		return interface->apInterface;
	}

	Log_ErrorF( gLC_Module, "Failed to find interface: %s\n", spName );
	return nullptr;
}

