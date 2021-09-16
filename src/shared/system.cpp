#include "../../inc/shared/system.h"

#include <SDL2/SDL.h>

void BaseSystem::ReadMessage(  )
{
	if ( !apMsgs )
		return;
	Message 	*pMsg;
	while ( true )
	{
		/* No more messages for this system.  */
		if ( !( pMsg = apMsgs->FetchMessage( aSystemType, aFlags ) ) )
			return;
		for ( auto&& cmd : aEngineCommands )
			/* If the message is used by the system.  */
			if ( cmd.msg == pMsg->msg )
				cmd.func( pMsg->args );
		/* Get rid of the spent message.  */
		apMsgs->Remove( apMsgs->aLastMsgIndex );
	}
}

void BaseSystem::ReadConsole(  )
{
#if 0
	if ( !apConsole || aConsoleCommands.empty(  ) )
		return;
	std::string 	command;
	while ( true )
	{
		if ( apConsole->Empty(  ) )
			return;
		command = apConsole->FetchCmd(  );
		/* Exhausted all commands in console.  */
		if ( command == "" )
			return;
		for ( const auto& cmd : aConsoleCommands )
		{
			/* Makes sure compound commands are parsed correctly .e.g bind in "bind up ent_create" could not be triggered by "bindyourmomlol".  */
			if ( cmd.str == command.substr( 0, cmd.str.length(  ) )
			     && ( command.size(  ) == cmd.str.size(  ) || command[ cmd.str.size(  ) ] == ' ' ) )
			{
				std::vector< std::string > args;
				int start, end = 0;
				/* Break the input into a vector of strings.  */
				while ( ( start = command.find_first_not_of( ' ', end ) ) != std::string::npos )
				{
					end = command.find( ' ', start );
					args.push_back( command.substr( start, end - start ) );
				}
				cmd.func( args );
				apConsole->DeleteCommand(  );
			}
		}
	}
#endif
}

void BaseSystem::AddUpdateFunction( std::function< void(  ) > sFunction )
{
	aFunctionList.push_back( sFunction );
}

void BaseSystem::AddFreeFunction( std::function< void(  ) > sFunction )
{
	aFreeQueue.push_back( sFunction );
}

void BaseSystem::ExecuteFunctions( FunctionList& srFunctionList )
{
	for ( const auto& func : srFunctionList )
		func(  );
}

void BaseSystem::InitCommands(  )
{
	
}

void BaseSystem::InitConsoleCommands(  )
{
	// really should move this to when loading a dll, before even calling this
	// maybe some DLLInit function
	apConsole->RegisterConVars(  );
}

void BaseSystem::InitSubsystems(  )
{
	
}

void BaseSystem::Update( float dt )
{
	ReadMessage(  );
	ReadConsole(  );
	ExecuteFunctions( aFunctionList );
}

void BaseSystem::AddFlag( int sFlags )
{
	aFlags |= sFlags;
}
	
void BaseSystem::RemoveFlag( int sFlags )
{
	
}
	
void BaseSystem::HandleSDLEvent( SDL_Event* e )
{
	
}

BaseSystem::BaseSystem(  )
{

}

void BaseSystem::Init(  )
{
	InitCommands(  );
	InitConsoleCommands(  );
	InitSubsystems(  );
}

void BaseSystem::SendMessages(  )
{
	
}

BaseSystem::~BaseSystem(  )
{
	/* Free all memory allocated by the system.  */
	ExecuteFunctions( aFreeQueue );
}
