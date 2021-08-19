#include "../../inc/shared/system.h"

void BaseSystem::ReadMessage(  )
{
	if ( !apMsgs )
		return;
	msg_s 	*pMsg;
	while ( true )
	{
		/* No more messages for this system.  */
		if ( !( pMsg = apMsgs->fetch_msg( aSystemType, aFlags ) ) )
			return;
		for ( auto&& cmd : aEngineCommands )
			/* If the message is used by the system.  */
			if ( cmd.msg == pMsg->msg )
				cmd.func( pMsg->args );
		/* Get rid of the spent message.  */
		apMsgs->remove( apMsgs->lastMsgIndex );
	}
}

void BaseSystem::ReadConsole(  )
{
	if ( !apConsole || aConsoleCommands.empty(  ) )
		return;
	std::string 	command;
	while ( true )
	{
		if ( apConsole->empty(  ) )
			return;
		command = apConsole->fetch_cmd(  );
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
				apConsole->delete_command(  );
			}
		}
	}
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
	
}

void BaseSystem::Update(  )
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

BaseSystem::BaseSystem(  )
{
        InitCommands(  );
	InitConsoleCommands(  );
	InitSubsystems(  );
}

void BaseSystem::InitSubsystems(  )
{
	
}

BaseSystem::~BaseSystem(  )
{
	/* Free all memory allocated by the system.  */
	ExecuteFunctions( aFreeQueue );
}
