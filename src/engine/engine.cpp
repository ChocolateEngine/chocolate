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

/* Note: SDL_LoadLibrary is a little broken on linux; it seems like SDL_GetError()
 * has a fixed length, so error reporting is a little flawed. For now, just temporarily
 * use dlopen() to get the full message.  */
BaseSystem* Engine::LoadModule( const std::string& srDlPath )
{
	Module handle = NULL;
	BaseSystem *( *cframework_get )() = 0;

	std::string path = filesys->FindFile( srDlPath + EXT_DLL );

	if ( path == "" )
	{
		LogWarn( gEngineChannel, "No module named %s found in search paths\n", ( srDlPath + EXT_DLL ).c_str() );
		return nullptr;
	}

	handle = sys_load_library( path.c_str() );
	if ( !handle )
	{
		LogFatal( gEngineChannel, "Unable to load shared librarys: %s\n", sys_get_error() );
	}
	
    *( void** )( &cframework_get ) = sys_load_func( handle, "cframework_get" );
	if ( !cframework_get ) {
		LogFatal( gEngineChannel, "Unable to link library function: %s\n", sys_get_error() );
	}

	aDlHandles.push_back( handle );
	
	BaseSystem *pSys = cframework_get();

	if ( pSys == nullptr )
	{
		LogFatal( "Failed to Load Framework From DLL: %s\n", srDlPath );
		return nullptr;
	}

	systems->Add( pSys );
    LogMsg( gEngineChannel, "Loaded Module: %s\n", srDlPath.c_str() );

	return pSys;
}

void Engine::InitSystems()
{
	BaseSystem* input = LoadModule( "input" );
	BaseSystem* graphics = nullptr;

	if ( cmdline->Find( "-g2" ) )
	{
		graphics = LoadModule( "graphics2" );
	}
	else
	{
		graphics = LoadModule( "graphics" );
	}

	BaseSystem* aduio =LoadModule( "aduio" );

	core_post_load();

	input->Init();
	graphics->Init();
	aduio->Init();
}

void Engine::UpdateSystems( float sDT )
{
	for ( const auto& pSys : systems->GetSystemList() ) {
	    pSys->Update( sDT );
	}
}
