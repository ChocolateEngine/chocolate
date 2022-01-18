/*
engine.cpp ( Authored by p0lyh3dron )

Defines the methods declared in engine.h.  
*/
#include "engine.h"
#include "core/platform.h"
#include "core/console.h"
#include "core/filesystem.h"
#include "util.h"

#ifdef _WIN32
	#include <direct.h>
#else
	#include <unistd.h>
#endif

#include <SDL2/SDL_loadso.h>
#include <chrono>

void Engine::LoadModule( const std::string& srDlPath )
{
	Module handle = NULL;
	BaseSystem *( *cframework_get )() = 0;

	std::string path = filesys->FindFile( srDlPath + EXT_DLL );

	if ( path == "" ) {
		Print( "No module named %s found in search paths\n", ( srDlPath + EXT_DLL ).c_str() );
		return;
	}

	handle = SDL_LoadObject( path.c_str() );
	if ( !handle )
	{
		Print( "Error: %s\n", SDL_GetError() );
		throw std::runtime_error( "Unable to load shared librarys!" );
	}
        *( void** )( &cframework_get ) = SDL_LoadFunction( handle, "cframework_get" );
	if ( !cframework_get ) {
		Print( "Error: %s\n", SDL_GetError() );
		throw std::runtime_error( "Unable to link library function!" );
	}

	aDlHandles.push_back( handle );
	
	BaseSystem *pSys = cframework_get();
	pSys->Init();
	systems->Add( pSys );
      	Print( "Loaded Module: %s\n", srDlPath.c_str() );
}

void Engine::InitSystems(  )
{
	LoadModule( "input" );
	LoadModule( "graphics" );
	LoadModule( "aduio" );
}

void Engine::UpdateSystems( float sDT )
{
	for ( const auto& pSys : systems->GetSystemList() ) {
	        pSys->Update( sDT );
	}
}
