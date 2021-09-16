#include "../../inc/shared/console.h"


Console* g_console = nullptr;

// concommands to register after static initalization
std::vector< ConCommand* > g_registerConCommands;



void ConCommand::Init( const std::string& name, ConCommandFunc func )
{
	aName = name;
	aFunc = func;

	g_registerConCommands.push_back( this );
}

// ================================================================================

bool Console::Empty(  )
{
	if ( aQueue.empty() )
		aCmdIndex = -1;

	return aQueue.empty(  ) ? true : false;
}

void Console::Add( const String &srCmd )
{
	aQueue.push_back( srCmd );
}

void Console::RegisterConVars(  )
{
	static bool registered = false;

	if ( registered )
		return;

	for ( auto const& cmd : g_registerConCommands )
	{
		AddConCommand( cmd );
	}

	registered = true;
}

void Console::AddConCommand( ConCommand* cmd )
{
	aConCommandList.push_back( cmd );
}

void Console::DeleteCommand(  )
{
	aQueue.erase( aQueue.begin(  ) + aCmdIndex );
}

void Console::Clear(  )
{
	aQueue.clear(  );
}

void Console::Update(  )
{
	std::string command;

	while ( true )
	{
		if ( Empty(  ) )
			return;

		command = FetchCmd(  );

		/* Exhausted all commands in console.  */
		if ( command == "" )
			return;

		for ( const auto& cmd : aConCommandList )
		{
			/* Makes sure compound commands are parsed correctly .e.g bind in "bind up ent_create" could not be triggered by "bindyourmomlol".  */
			if ( cmd->aName == command.substr( 0, cmd->aName.length(  ) )
				&& ( command.size(  ) == cmd->aName.size(  ) || command[ cmd->aName.size(  ) ] == ' ' ) )
			{
				std::vector< std::string > args;
				int start, end = 0;
				/* Break the input into a vector of strings.  */
				while ( ( start = command.find_first_not_of( ' ', end ) ) != std::string::npos )
				{
					end = command.find( ' ', start );
					args.push_back( command.substr( start, end - start ) );
				}
				cmd->aFunc( args );
				DeleteCommand(  );
			}
		}
	}
}


std::string Console::FetchCmd(  )
{
	aCmdIndex++;
	if ( Empty(  ) || aCmdIndex >= aQueue.size(  ) )
	{
		aCmdIndex = 0;
		return "";
	}
	return aQueue[ aCmdIndex ];
}

Console::Console(  )
{
}

Console::~Console(  )
{
}
