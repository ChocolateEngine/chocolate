/*
engine.cpp ( Authored by p0lyh3dron )

Defines the methods declared in engine.h.  
*/
#include "engine.h"
#include "core/platform.h"
#include "core/console.h"
#include "core/filesystem.h"
#include "core/commandline.h"
#include "core/log.h"
#include "util.h"

#ifdef _WIN32
	#include <direct.h>
#else
	#include <unistd.h>
#endif

#include <SDL2/SDL_loadso.h>
#include <chrono>

/* Note: SDL_LoadLibrary is a little broken on linux; it seems like SDL_GetError()
 * has a fixed length, so error reporting is a little flawed. For now, just temporarily
 * use dlopen() to get the full message.  */
void Engine::LoadModule( const std::string& srDlPath )
{
	Module handle = NULL;
	BaseSystem *( *cframework_get )() = 0;

	std::string path = filesys->FindFile( srDlPath + EXT_DLL );

	if ( path == "" ) {
		LogWarn( "No module named %s found in search paths\n", ( srDlPath + EXT_DLL ).c_str() );
		return;
	}

	handle = sys_load_library( path.c_str() );
	if ( !handle )
	{
		LogFatal( "Unable to load shared librarys: %s\n", sys_get_error() );
	}
	
    *( void** )( &cframework_get ) = sys_load_func( handle, "cframework_get" );
	if ( !cframework_get ) {
		LogFatal( "Unable to link library function: %s\n", sys_get_error() );
	}

	aDlHandles.push_back( handle );
	
	BaseSystem *pSys = cframework_get();
	pSys->Init();
	systems->Add( pSys );
    LogMsg( "Loaded Module: %s\n", srDlPath.c_str() );
}

void Engine::InitSystems()
{
	LoadModule( "input" );
	if ( cmdline->Find( "-g2" ) )
	{
		LoadModule( "graphics2" );
	}
	else
	{
		LoadModule( "graphics" );
	}
	LoadModule( "aduio" );
}

void Engine::UpdateSystems( float sDT )
{
	for ( const auto& pSys : systems->GetSystemList() ) {
	    pSys->Update( sDT );
	}
}
