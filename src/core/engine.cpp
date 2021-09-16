/*
engine.cpp ( Authored by p0lyh3dron )

Defines the methods declared in engine.h.  
*/
#include "../../inc/core/engine.h"
#include "../../inc/shared/platform.h"

#include <chrono>

// im sorry
extern Console* g_console;

template< typename T, typename... TArgs >
void Engine::AddSystem( const T *spSystem, TArgs... sSystems )
{
	if ( !spSystem )
		return;
	aSystems.push_back( ( BaseSystem* )spSystem );
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
		aSystems.push_back( pSys );
		apSystemManager->Add( pSys );
	}
	for ( const auto& pSys : gameSystems )
		pSys->Init(  );
	for ( const auto& pSys : gameSystems )
		pSys->SendMessages(  );
}

void Engine::InitCommands(  )
{
	apCommandManager->Add( Engine::Commands::PING, Command<  >( [ & ](  ){ printf( "Ping!\n" ); } ) );
	apCommandManager->Add( Engine::Commands::EXIT, Command<  >( [ & ](  ){ aActive = false; } ) );
}

void Engine::InitConsoleCommands(  )
{
	
}

void Engine::LoadObject( const std::string& srDlPath, const std::string& srEntry )
{
	Module handle = NULL;

	handle = LOAD_LIBRARY( srDlPath.c_str(  ) );
	if ( !handle )
	{
		fprintf( stderr, "Error: %s\n", GET_ERROR() );
		throw std::runtime_error( "Unable to load shared librarys!" );
	}

	*( void** )( &game_init ) = LOAD_FUNC( handle, srEntry.c_str(  ) );
	if ( !game_init )
	{
		fprintf( stderr, "Error: %s\n", GET_ERROR() );
		CLOSE_LIBRARY( handle );
		throw std::runtime_error( "Unable to link library's entry point!" );
	}
	aDlHandles.push_back( handle );
	AddFreeFunction( [ = ](  ){ CLOSE_LIBRARY( ( Module )handle ); } );
}

void Engine::EngineMain(  )
{
	static Messages         msgs;
	static Console 		console;
	static CommandManager   commandManager;
	static SystemManager   systemManager;

	apMsgs 		= &msgs;
	apConsole 	= &console;
	apCommandManager  = &commandManager;
	apSystemManager  = &systemManager;
	aActive 		= true;

	LoadObject( "bin/client" EXT_DLL, "game_init" );
	InitSystems(  );
	AddGameSystems(  );
	InitCommands(  );

	apMsgs->Add( ENGINE_C, ENGI_PING );
	//apCommandManager->Execute( GraphicsSystem::Commands::INIT_SPRITE, "/home/karl/Pictures/tidd.png", NULL );
	
	while ( aActive )
		UpdateSystems(  );
}

void Engine::InitSystems(  )
{
	AddSystem( new GraphicsSystem, new InputSystem, new AudioSystem, new GuiSystem );

	for ( const auto& pSys : aSystems )
	{
		pSys->apMsgs 		= apMsgs;
		pSys->apConsole 	= apConsole;
		pSys->apCommandManager	= apCommandManager;
		pSys->apSystemManager	= apSystemManager;
	}
	for ( const auto& pSys : aSystems )
		pSys->Init(  );
	for ( const auto& pSys : aSystems )
		pSys->SendMessages(  );
}

void Engine::UpdateSystems(  )
{
	static auto startTime = std::chrono::high_resolution_clock::now(  );

	auto currentTime = std::chrono::high_resolution_clock::now();
	float time = std::chrono::duration< float, std::chrono::seconds::period >( currentTime - startTime ).count(  );

	apConsole->Update(  );

	/* Update self.  */
	Update( time );

	for ( const auto& pSys : aSystems )
		pSys->Update( time );

	startTime = currentTime;
}

Engine::Engine(  ) : BaseSystem(  )
{
	aSystemType = ENGINE_C;
}

Engine::~Engine(  )
{
	for ( const auto& pSys : aSystems )
		//pSys->~BaseSystem(  );
		delete pSys;

	for ( const auto& handle : aDlHandles )
		CLOSE_LIBRARY( (Module)handle );
}
