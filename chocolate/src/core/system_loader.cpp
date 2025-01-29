#include "core/core.h"

#include <set>

LOG_CHANNEL_REGISTER( Module, ELogColor_Default );


struct LoadedSystem_t
{
	ISystem*  apSystem;
	Module    apModule;
	ch_string name;
	size_t    apVer;
	bool      aRequired = true;
	bool      aRunning  = false;
};


struct InterfacesFromModule_t
{
	Module             module;
	ModuleInterface_t* interfaces;
	size_t             count;
};


static std::unordered_map< const char*, Module > gModuleHandles;

static ModuleInterface_t*                        gInterfaces            = nullptr;
static u32                                       gInterfacesCount       = 0;

// stupid, used only in Mod_Free, where we need to know what interfaces are in what modules
// or don't do this? what if we just free the module and have the app free the interfaces?
static InterfacesFromModule_t*                   gInterfacesFromModule;

static LoadedSystem_t*                           gLoadedSystems         = nullptr;
static u32                                       gLoadedSystemsCount    = 0;

const size_t                                     CH_PLAT_FOLDER_SEP_LEN = strlen( CH_PLAT_FOLDER_SEP );


bool Mod_InitLoadedSystem( LoadedSystem_t& appModule )
{
	CH_ASSERT( appModule.apSystem );

	if ( !appModule.apSystem )
		return false;

	if ( appModule.apSystem->Init() )
	{
		appModule.aRunning = true;
		return true;
	}

	if ( appModule.aRequired )
		Log_ErrorF( "Failed to Init Required System: %s\n", appModule.name.data );
	else
		Log_ErrorF( "Failed to Init Optional System: %s\n", appModule.name.data );

	return false;
}


bool Mod_InitSystems()
{
	// load saved cvar values and other stuff
	core_post_load();

	for ( u32 i = 0; i < gLoadedSystemsCount; i++ )
	{
		LoadedSystem_t& appModule = gLoadedSystems[ i ];

		CH_ASSERT( appModule.apSystem );

		// If we failed to init this system and it's required, then stop
		if ( !Mod_InitLoadedSystem( appModule ) && appModule.aRequired )
		{
			return false;
		}
	}

	return true;
}


void Mod_Shutdown()
{
	if ( gLoadedSystemsCount == 0 || gLoadedSystems == nullptr )
		return;

	// Go through the list in reverse, in order from what started up last to what started first
	u32 i = gLoadedSystemsCount;
	do
	{
		i--;

		LoadedSystem_t& appModule = gLoadedSystems[ i ];

		if ( appModule.name.data )
			ch_str_free( appModule.name.data );

		if ( !appModule.apSystem )
			continue;

		if ( !appModule.aRunning )
			continue;

		appModule.apSystem->Shutdown();
		appModule.aRunning = false;
	}
	while ( i > 0 );

	if ( gLoadedSystems )
		ch_free( gLoadedSystems );

	if ( gInterfaces )
		ch_free( gInterfaces );

	if ( gInterfacesFromModule )
		ch_free( gInterfacesFromModule );

	gLoadedSystems        = nullptr;
	gLoadedSystemsCount   = 0;

	gInterfaces           = nullptr;
	gInterfacesFromModule = nullptr;
	gInterfacesCount      = 0;

	// Free all the modules we loaded
	for ( const auto& [ name, module ] : gModuleHandles )
	{
		sys_close_library( module );
	}
}


// Load the dll, get the interfaces, and store them
Module Mod_Load( const char* spPath )
{
	auto it = gModuleHandles.find( spPath );
	if ( it != gModuleHandles.end() )
		return it->second;  // already loaded

	// TODO: improve this, what if it has a path before it
	const char*    strings[] = { CH_PLAT_FOLDER_SEP, spPath, EXT_DLL };
	const size_t   lengths[] = { CH_PLAT_FOLDER_SEP_LEN, strlen( spPath ), EXT_DLL_LEN };
	ch_string_auto pathExt   = ch_str_join( 3, strings, lengths );

	ch_string_auto path      = FileSys_FindFileEx( CH_STR_UNROLL( pathExt ), ESearchPathType_Binary );

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

	// ------------------------------------------
	// Find all interfaces in this module

	ModuleInterface_t* ( *getInterfacesF )( u8 & srCount ) = nullptr;
	if ( !( *(void**)( &getInterfacesF ) = sys_load_func( handle, "ch_get_interfaces" ) ) )
	{
		Log_ErrorF( gLC_Module, "Unable to load \"ch_get_interfaces\" function from library: %s - %s\n", pathExt.data, sys_get_error() );
		return nullptr;
	}

	u8                 count      = 0;
	ModuleInterface_t* interfaces = getInterfacesF( count );

	if ( interfaces == nullptr || count == 0 )
	{
		Log_ErrorF( "Failed to Load Interfaces From Library: %s\n", pathExt.data );
		return nullptr;
	}

	Log_DevF( gLC_Module, 1, "Loading Module: %s\n", pathExt.data );

	// realloc interfaces
	ModuleInterface_t* newData = ch_realloc< ModuleInterface_t >( gInterfaces, gInterfacesCount + count );

	if ( !newData )
	{
		Log_ErrorF( gLC_Module, "Failed to realloc interfaces\n" );
		return nullptr;
	}

	gInterfaces = newData;

	for ( size_t i = 0; i < count; i++ )
	{
		// add interface
		gInterfaces[ gInterfacesCount + i ] = interfaces[ i ];
		Log_DevF( gLC_Module, 1, "    Found Interface: %s - Version %zd\n", interfaces[ i ].apName, interfaces[ i ].aVersion );
	}

	// add interfaces from this module for freeing later if needed
	gInterfacesFromModule                                = ch_realloc< InterfacesFromModule_t >( gInterfacesFromModule, gInterfacesCount + 1 );
	gInterfacesFromModule[ gInterfacesCount ].module     = handle;
	gInterfacesFromModule[ gInterfacesCount ].interfaces = interfaces;
	gInterfacesFromModule[ gInterfacesCount ].count      = count;

	gInterfacesCount += count;

	return handle;
}


void Mod_Free( const char* spPath )
{
	auto it = gModuleHandles.find( spPath );
	if ( it == gModuleHandles.end() )
		return;  // Module was never loaded
	
	// Shutdown Systems loaded with this module
	for ( u32 i = 0; i < gLoadedSystemsCount; i++ )
	{
		LoadedSystem_t& loadedSystem = gLoadedSystems[ i ];

		if ( loadedSystem.apModule == it->second )
		{
			loadedSystem.apSystem->Shutdown();
			loadedSystem.aRunning = false;
		}

	}
	
	// find this module in gInterfacesModules so we can remove this system from gInterfaces
	// wait no this wont work smh, because of multiple interface in one module, this would just remove the first one only
	for ( u32 i = 0; i < gInterfacesCount; i++ )
	{
		if ( gInterfacesFromModule[ i ].module != it->second )
			continue;

		// shift everything back 1
		for ( u16 shift_i = i; shift_i < gInterfacesCount - 1; shift_i++ )
		{
			gInterfacesFromModule[ shift_i ] = gInterfacesFromModule[ shift_i + 1 ];
			gInterfaces[ shift_i ]           = gInterfaces[ shift_i + 1 ];
		}

		gInterfacesCount--;
		break;
	}

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
	void* system = Mod_GetSystem( srModule.apInterfaceName, srModule.apInterfaceVer );
	if ( system == nullptr )
	{
		Log_ErrorF( gLC_Module, "Failed to load system from module: %s - %s\n", srModule.apModuleName, srModule.apInterfaceName );
		return EModLoadError_LoadInterface;
	}

	*srModule.apSystem = static_cast< ISystem* >( system );
	// gSystems.push_back( static_cast< ISystem* >( system ) );

	LoadedSystem_t loadedModule{};
	loadedModule.apModule   = module;
	loadedModule.apSystem   = static_cast< ISystem* >( system );
	loadedModule.aRequired  = srModule.aRequired;
	loadedModule.apVer      = srModule.apInterfaceVer;
	loadedModule.name      = ch_str_copy( srModule.apInterfaceName );

	// realloc loaded systems
	LoadedSystem_t* newData = ch_realloc< LoadedSystem_t >( gLoadedSystems, gLoadedSystemsCount + 1 );

	if ( !newData )
	{
		Log_ErrorF( gLC_Module, "Failed to realloc loaded systems\n" );
		ch_str_free( loadedModule.name.data );
		return EModLoadError_LoadModule;
	}

	gLoadedSystems = newData;
	gLoadedSystems[ gLoadedSystemsCount ] = loadedModule;
	gLoadedSystemsCount++;

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
	size_t nameLen = strlen( srModule.apInterfaceName );

	// Dumb, find the AppLoadedModule_t struct we made so can initialize it
	for ( u32 i = 0; i < gLoadedSystemsCount; i++ )
	{
		LoadedSystem_t& loadedSystem = gLoadedSystems[ i ];

		if ( ch_str_equals( loadedSystem.name, srModule.apInterfaceName, nameLen ) )
		{
			return Mod_InitLoadedSystem( loadedSystem );
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

	size_t nameLen = strlen( srModule.apInterfaceName );

	// Dumb, find the AppLoadedModule_t struct we made so can initialize it
	for ( u32 i = 0; i < gLoadedSystemsCount; i++ )
	{
		LoadedSystem_t& loadedSystem = gLoadedSystems[ i ];

		if ( ch_str_equals( loadedSystem.name, srModule.apInterfaceName, nameLen ) )
		{
			Mod_InitLoadedSystem( loadedSystem );
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


void* Mod_GetSystem( const char* spName, size_t sVersion )
{
	if ( spName == nullptr )
	{
		Log_ErrorF( gLC_Module, "System Search is nullptr\n" );
		return nullptr;
	}

	u64 searchNameLen = strlen( spName );

	for ( u32 i = 0; i < gInterfacesCount; i++ )
	{
		ModuleInterface_t& interface = gInterfaces[ i ];

		if ( !ch_str_equals( interface.apName, spName, searchNameLen ) )
		{
			continue;
		}

		if ( interface.aVersion != sVersion )
		{
			Log_ErrorF( gLC_Module, "Found system \"%s\" but version is different: App Version: %zd - Module Version: %zd\n", spName, sVersion, interface.aVersion );
			continue;
		}

		return interface.apInterface;
	}

	Log_ErrorF( gLC_Module, "Failed to find system: %s\n", spName );
	return nullptr;
}

