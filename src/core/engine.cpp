/*
engine.cpp ( Authored by p0lyh3dron )

Defines the methods declared in engine.h.  
*/
#include "../../inc/core/engine.h"
#include "../../inc/shared/platform.h"

#include <chrono>

CONVAR( en_max_frametime, 0.2 );
CONVAR( en_timescale, 1 );
CONVAR( en_fps_max, 120 );

template< typename T, typename... TArgs >
void Engine::AddSystem( const T *spSystem, TArgs... sSystems )
{
	if ( !spSystem )
		return;
	apSystemManager->Add( ( BaseSystem* )spSystem );
	AddSystem( sSystems... );
}

void Engine::AddGameSystems(  )
{
	std::vector< BaseSystem* > gameSystems;
	game_init( gameSystems );
	/* Mark systems external, and synchronize communication.  */
	for ( const auto& pSys : gameSystems )
	{
		pSys->AddFlag( EXTERNAL_SYSTEM );
		pSys->apMsgs 		= this->apMsgs;
		pSys->apConsole 	= this->apConsole;
		pSys->apCommandManager 	= this->apCommandManager;
		pSys->apSystemManager 	= this->apSystemManager;
		//aSystems.push_back( pSys );
		apSystemManager->Add( pSys );
	}
	for ( const auto& pSys : gameSystems )
		pSys->Init(  );
	for ( const auto& pSys : gameSystems )
		pSys->SendMessages(  );
}

void Engine::InitCommands(  )
{
	apCommandManager->Add( Engine::Commands::PING, Command<  >( [ & ](  ){ apConsole->Print( "Ping!\n" ); } ) );
	apCommandManager->Add( Engine::Commands::EXIT, Command<  >( [ & ](  ){ aActive = false; } ) );
}

void Engine::InitConsoleCommands(  )
{
	CON_COMMAND_LAMBDA( help )
	{
		apConsole->PrintAllConVars();
	});

	CON_COMMAND_LAMBDA( exit ) { aActive = false; });
	CON_COMMAND_LAMBDA( quit ) { aActive = false; });

	BaseSystem::InitConsoleCommands(  );
}

void Engine::LoadObject( const std::string& srDlPath, const std::string& srEntry )
{
	Module handle = NULL;

	handle = LOAD_LIBRARY( srDlPath.c_str(  ) );
	if ( !handle )
	{
		// fprintf( stderr, "Error: %s\n", GET_ERROR() );
		apConsole->Print( "Error: %s\n", GET_ERROR() );
		throw std::runtime_error( "Unable to load shared librarys!" );
	}

	*( void** )( &game_init ) = LOAD_FUNC( handle, srEntry.c_str(  ) );
	if ( !game_init )
	{
		//fprintf( stderr, "Error: %s\n", GET_ERROR() );
		apConsole->Print( "Error: %s\n", GET_ERROR() );
		CLOSE_LIBRARY( handle );
		throw std::runtime_error( "Unable to link library's entry point!" );
	}
	aDlHandles.push_back( handle );
	AddFreeFunction( [ = ](  ){ CLOSE_LIBRARY( ( Module )handle ); } );
}

void Engine::EngineMain(  )
{
	static Messages         			msgs;
	static Console 					console;
	static CommandManager			   	commandManager;
	static SystemManager   				systemManager;

	apMsgs 		= &msgs;
	apConsole 	= &console;
	apCommandManager  = &commandManager;
	apSystemManager  = &systemManager;
	aActive 		= true;

	LoadObject( "bin/client" EXT_DLL, "game_init" );
	InitSystems(  );
	AddGameSystems(  );
	InitCommands(  );
	InitConsoleCommands(  );

	apMsgs->Add( ENGINE_C, ENGI_PING );
	//apCommandManager->Execute( GraphicsSystem::Commands::INIT_SPRITE, "/home/karl/Pictures/tidd.png", NULL );
	
	while ( aActive )
		UpdateSystems(  );
}

void Engine::InitSystems(  )
{
	AddSystem( new GraphicsSystem, new InputSystem, new AudioSystem, new GuiSystem );

	for ( const auto& pSys : apSystemManager->GetSystemList() )
	{
		pSys->apMsgs 		= apMsgs;
		pSys->apConsole 	= apConsole;
		pSys->apCommandManager	= apCommandManager;
		pSys->apSystemManager	= apSystemManager;
	}

	for ( const auto& pSys : apSystemManager->GetSystemList() )
		pSys->Init(  );

	for ( const auto& pSys : apSystemManager->GetSystemList() )
		pSys->SendMessages(  );
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
		float maxFps = glm::clamp( en_fps_max.GetFloat(), 20.f, 5000.f );

		// check if we still have more than 2ms till next frame and if so, wait for "1ms"
		float minFrameTime = 1.0f / maxFps;
		if ( (minFrameTime - time) > (2.0f/1000.f))
			SDL_Delay(1);

		// framerate is above max
		if (time < minFrameTime)
			return;
	}

	apConsole->Update(  );

	static GraphicsSystem* graphics = GET_SYSTEM( GraphicsSystem );

	/* Update self.  */
	Update( time );

	for ( const auto& pSys : apSystemManager->GetSystemList() )
	{
		if ( pSys != graphics )
			pSys->Update( time );
	}

	graphics->Update( time );

	startTime = currentTime;
}

Engine::Engine(  ) : BaseSystem(  )
{
	aSystemType = ENGINE_C;
}

Engine::~Engine(  )
{
	for ( const auto& pSys : apSystemManager->GetSystemList() )
		delete pSys;

	for ( const auto& handle : aDlHandles )
		CLOSE_LIBRARY( (Module)handle );
}
