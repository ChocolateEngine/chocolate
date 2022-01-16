/*
engine.cpp ( Authored by p0lyh3dron )

Defines the methods declared in engine.h.  
*/
#include "engine.h"
#include "core/platform.h"
#include "core/console.h"
#include "util.h"

#ifdef _WIN32
	#include <direct.h>
#else
	#include <unistd.h>
#endif

#include <SDL2/SDL_loadso.h>
#include <chrono>

CONVAR( en_max_frametime, 0.2 );
CONVAR( en_timescale, 1 );
CONVAR( en_fps_max, 0 );

void Engine::AddGameSystems(  )
{
	std::vector< BaseSystem* > gameSystems;
	game_init( gameSystems );

	/* Mark systems external, and synchronize communication.  */
	for ( const auto& pSys : gameSystems )
		systems->Add( pSys );

	for ( const auto& pSys : gameSystems )
		pSys->Init(  );
}

void Engine::LoadObject( const std::string& srDlPath, const std::string& srEntry )
{
	Module handle = NULL;

	handle = SDL_LoadObject( srDlPath.c_str(  ) );
	if ( !handle )
	{
		Print( "Error: %s\n", SDL_GetError() );
		throw std::runtime_error( "Unable to load shared librarys!" );
	}

	*( void** )( &game_init ) = SDL_LoadFunction( handle, srEntry.c_str(  ) );
	if ( !game_init )
	{
		Print( "Error: %s\n", SDL_GetError() );
		SDL_UnloadObject( handle );
		throw std::runtime_error( "Unable to link library's entry point!" );
	}
	aDlHandles.push_back( handle );
}

void Engine::EngineMain(  )
{
	CON_COMMAND_LAMBDA( exit ) { aActive = false; });
	CON_COMMAND_LAMBDA( quit ) { aActive = false; });	
	aActive 		= true;

	LoadObject( "bin/client" EXT_DLL, "game_init" );
	InitSystems(  );
	AddGameSystems(  );
	
	while ( aActive )
		UpdateSystems(  );
}

void Engine::Update( float sDT )
{
	/* Ball.  */
}

void Engine::LoadModule( const std::string& srDlPath )
{
	Module handle = NULL;

	handle = SDL_LoadObject( (aExePath + srDlPath + EXT_DLL).c_str(  ) );
	if ( !handle )
	{
		Print( "Error: %s\n", SDL_GetError() );
		throw std::runtime_error( "Unable to load shared librarys!" );
	}
	else
	{
		Print( "Loaded Module: %s\n", srDlPath.c_str() );
	}
}

void Engine::InitSystems(  )
{
	systems->Add( new InputSystem );

	LoadModule( "bin/graphics" );
	LoadModule( "bin/aduio" );

	console->RegisterConVars(  );

	for ( const auto& pSys : systems->GetSystemList() )
		pSys->Init(  );
}

void Engine::UpdateSystems(  )
{
	static auto startTime = std::chrono::high_resolution_clock::now(  );

	auto currentTime = std::chrono::high_resolution_clock::now();
	float time = std::chrono::duration< float, std::chrono::seconds::period >( currentTime - startTime ).count(  );
	
	// don't let the time go too crazy, usually happens when in a breakpoint
	time = glm::min( time, en_max_frametime.GetFloat() );

	// rendering is actually half the framerate for some reason, odd
	if ( en_fps_max.GetFloat() > 0.f )
	{
		// HACK: fps is doubled for some reason, so multiple by 2
		float maxFps = glm::clamp( en_fps_max.GetFloat() * 2.f, 20.f, 5000.f );

		// check if we still have more than 2ms till next frame and if so, wait for "1ms"
		float minFrameTime = 1.0f / maxFps;
		if ( (minFrameTime - time) > (2.0f/1000.f))
			SDL_Delay(1);

		// framerate is above max
		if (time < minFrameTime)
			return;
	}

	console->Update(  );

	static BaseGraphicsSystem* graphics = GET_SYSTEM( BaseGraphicsSystem );

	/* Update self.  */
	Update( time );

	for ( const auto& pSys : systems->GetSystemList() )
	{
		if ( pSys != graphics )
			pSys->Update( time );
	}

	graphics->Update( time );

	startTime = currentTime;
}

Engine::Engine(  )
{
}

Engine::~Engine(  )
{
	for ( const auto& pSys : systems->GetSystemList() )
		delete pSys;

	for ( const auto& handle : aDlHandles )
		SDL_UnloadObject( (Module)handle );
}
