/*
engine.cpp ( Authored by p0lyh3dron )

Defines the methods declared in engine.h.  
*/
#include "engine.h"

#ifdef _WIN32
	#include <direct.h>
#else
	#include <unistd.h>
#endif

#include <SDL2/SDL_loadso.h>
#include <chrono>

LOG_REGISTER_CHANNEL( Engine, LogColor::Default );

void Engine::Init( const std::vector< std::string >& srModulePaths )
{
	// Create some engine specific console commands.
	CON_COMMAND_LAMBDA( exit ) { aActive = false; });
	CON_COMMAND_LAMBDA( quit ) { aActive = false; });

	// Grab the systems from libraries.
	for ( auto& path : srModulePaths )
		LoadModule( path );

	// load saved cvar values and other stuff
	core_post_load();

	for ( BaseSystem* sys : systems->GetSystemList() )
		sys->Init();

	// Register the convars that are declared.
	Con_RegisterConVars();

	// Get systems that the engine needs to specificallykeep track of.
	apGuiSystem = GET_SYSTEM( BaseGuiSystem );
}

/* Note: SDL_LoadLibrary is a little broken on linux; it seems like SDL_GetError()
 * has a fixed length, so error reporting is a little flawed. For now, just temporarily
 * use dlopen() to get the full message.  */
BaseSystem* Engine::LoadModule( const std::string& srDlPath )
{
	Module handle = NULL;
	BaseSystem *( *cframework_get )() = 0;

	std::string path = FileSys_FindFile( srDlPath + EXT_DLL );

	if ( path == "" )
	{
		Log_WarnF( gEngineChannel, "No module named %s found in search paths\n", ( srDlPath + EXT_DLL ).c_str() );
		return nullptr;
	}

	handle = sys_load_library( path.c_str() );
	if ( !handle )
	{
		Log_FatalF( gEngineChannel, "Unable to load shared librarys: %s\n", sys_get_error() );
	}
	
    *( void** )( &cframework_get ) = sys_load_func( handle, "cframework_get" );
	if ( !cframework_get ) {
		Log_FatalF( gEngineChannel, "Unable to link library function: %s\n", sys_get_error() );
	}

	aDlHandles.push_back( handle );
	
	BaseSystem *pSys = cframework_get();

	if ( pSys == nullptr )
	{
		Log_FatalF( "Failed to Load Framework From DLL: %s\n", srDlPath.c_str() );
		return nullptr;
	}

	systems->Add( pSys );
    Log_MsgF( gEngineChannel, "Loaded Module: %s\n", srDlPath.c_str() );

	return pSys;
}

void Engine::UpdateSystems( float sDT )
{
	for ( const auto& pSys : systems->GetSystemList() )
	    pSys->Update( sDT );
}
