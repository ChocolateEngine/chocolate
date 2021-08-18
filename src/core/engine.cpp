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
		pSys->apMsgs = this->apMsgs;
		pSys->apConsole = apConsole;
		aSystems.push_back( pSys );
	}
}

void Engine::InitCommands(  )
{
	msg_s msg;
	msg.type 	= ENGINE_C;
	
	msg.msg 	= ENGI_PING;
	msg.func 	= [ & ]( std::vector< std::any > args ){ printf( "Engine: Ping!\n" ); };
	aEngineCommands.push_back( msg );

	msg.msg 	= ENGI_EXIT;
	msg.func 	= [ & ]( std::vector< std::any > args ){ aActive = false; };
	aEngineCommands.push_back( msg );
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
	static msgs_c 		msgs;
	static console_c 	console;

	this->apMsgs 		= &msgs;
	this->apConsole 	= &console;

        LoadObject( "bin/client" EXT_DLL, "game_init" );
	InitSystems(  );
	AddGameSystems(  );

	this->apMsgs->add( ENGINE_C, ENGI_PING );
	
	while ( aActive )
	        UpdateSystems(  );
}

void Engine::InitSystems(  )
{
	AddSystem( new graphics_c,
		   new input_c,
		   new AudioSystem,
		   new gui_c );
	
	for ( const auto& pSys : aSystems )
	{
		pSys->apMsgs 		= this->apMsgs;
		pSys->apConsole 	= this->apConsole;
		pSys->InitSubsystems(  );
	}
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
