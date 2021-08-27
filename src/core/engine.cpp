/*
engine.cpp ( Authored by p0lyh3dron )

Defines the methods declared in engine.h.  
*/
#include "../../inc/core/engine.h"
#include "../../inc/shared/platform.h"

template< typename T, typename... TArgs >
void Engine::AddSystem( const T *spSystem, TArgs... sSystems )
{
	if ( !spSystem )
		return;
	aSystems.push_back( ( BaseSystem* )spSystem );
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
		aSystems.push_back( pSys );
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

	this->apMsgs 		= &msgs;
	this->apConsole 	= &console;
	this->apCommandManager  = &commandManager;
	this->aActive 		= true;

        LoadObject( "bin/client" EXT_DLL, "game_init" );
	InitSystems(  );
	AddGameSystems(  );
	InitCommands(  );

	this->apMsgs->Add( ENGINE_C, ENGI_PING );
	
	while ( aActive )
	        UpdateSystems(  );
}

void Engine::InitSystems(  )
{
	AddSystem( new GraphicsSystem, new InputSystem, new AudioSystem, new GuiSystem );
	for ( const auto& pSys : aSystems )
	{
		pSys->apMsgs 		= this->apMsgs;
		pSys->apConsole 	= this->apConsole;
		pSys->apCommandManager	= this->apCommandManager;
	}
	for ( const auto& pSys : aSystems )
		pSys->Init(  );
	for ( const auto& pSys : aSystems )
		pSys->SendMessages(  );
}

void Engine::UpdateSystems(  )
{
	/* Update self.  */
	this->Update(  );

	for ( const auto& pSys : aSystems )
		pSys->Update(  );
}

Engine::Engine(  ) : BaseSystem(  )
{
	aSystemType = ENGINE_C;
}

Engine::~Engine(  )
{
	for ( const auto& pSys : aSystems )
		pSys->~BaseSystem(  );
}
