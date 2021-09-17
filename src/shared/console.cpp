#include "../../inc/shared/console.h"
#include "../../inc/shared/util.h"


Console* g_console = nullptr;

// concommands to register after static initalization
std::vector< ConCommand* > g_registerConCommands;
std::vector< ConVar* > g_registerConVars;


void ConCommand::Init( const std::string& name, ConVarFunc func )
{
	aName = name;
	aFunc = func;

	g_registerConCommands.push_back( this );
}

// ================================================================================


void ConVar::Init( const std::string& name, const std::string& defaultValue )
{
	aName = name;
	aDefaultValue = defaultValue;

	SetValue( defaultValue );

	g_registerConVars.push_back( this );
}


void ConVar::Init( const std::string& name, const std::string& defaultValue, ConVarFunc callback )
{
	aName = name;
	aDefaultValue = defaultValue;
	aFunc = callback;

	SetValue( defaultValue );

	g_registerConVars.push_back( this );
}


static float ToFloat( const std::string& value, float prev )
{
	if ( value.empty() )
		return prev;

	char* end;
	double result = 0;

	result = strtod( value.c_str(), &end );

	if ( end == value.c_str() )
	{
		return prev;
	}

	return result;
}



void ConVar::SetValue( const std::string& value )
{
	aValue = value;

	aValueFloat = ToFloat( value, aValueFloat );
}


const std::string& ConVar::GetValue(  )
{
	return aValue;
}

float ConVar::GetFloat(  )
{
	return aValueFloat;
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

	if ( aCommandHistory.empty() || aCommandHistory.back() != srCmd )
		aCommandHistory.push_back( srCmd );

	AddToHistory( srCmd );
}

void Console::RegisterConVars(  )
{
	static bool registered = false;

	//if ( registered )
	//	return;

	// TODO: make both of these share a base class, and use dynamic_cast to check which one it is, smh my head
	for ( auto const& cmd : g_registerConCommands )
	{
		AddConCommand( cmd );
	}

	for ( auto const& cmd : g_registerConVars )
	{
		AddConVar( cmd );
	}

	registered = true;
}


void Console::AddConCommand( ConCommand* cmd )
{
	if ( !vec_contains( aConCommandList, cmd ) )
		aConCommandList.push_back( cmd );
}

void Console::AddConVar( ConVar* cmd )
{
	if ( !vec_contains( aConVarList, cmd ) )
		aConVarList.push_back( cmd );
}

void Console::DeleteCommand(  )
{
	aQueue.erase( aQueue.begin(  ) + aCmdIndex );
}

void Console::Clear(  )
{
	aQueue.clear(  );
}

void Console::AddToHistory( const std::string& str )
{
	aConsoleHistory += "] " + str + "\n";
}

const std::string& Console::GetConsoleHistory( )
{
	return aConsoleHistory;
}

Console::StringList Console::GetCommandHistory( )
{
	return aCommandHistory;
}

void Console::SetTextBuffer( const std::string& str )
{
	aTextBuffer = str;
}

const std::string& Console::GetTextBuffer( )
{
	return aTextBuffer;
}


void Console::Update(  )
{
	static size_t regConCmds = g_registerConCommands.size();
	static size_t regCVars = g_registerConVars.size();

	// probably can't do this only on the first call here due to potentially loading other dlls with convars
	// after this has already called
	if ( regConCmds != g_registerConCommands.size() || regCVars != g_registerConVars.size() )
	{
		// HACK HACK HACK
		// only needed for when creating cvars in functions
		RegisterConVars(  );

		regConCmds = g_registerConCommands.size();
		regCVars = g_registerConVars.size();
	}

	std::string command;

	while ( true )
	{
		if ( Empty(  ) )
			return;

		command = FetchCmd(  );

		/* Exhausted all commands in console.  */
		if ( command == "" )
			return;

		std::string commandName;
		std::vector< std::string > args;
		int start, end = 0;

		/* Break the input into a vector of strings.  */
		while ( ( start = command.find_first_not_of( ' ', end ) ) != std::string::npos )
		{
			end = command.find( ' ', start );

			if ( start == 0 )
				commandName = command.substr( start, end - start );
			else
				args.push_back( command.substr( start, end - start ) );
		}

		bool commandCalled = false;

		for ( const auto& cvar : aConVarList )
		{
			if ( cvar->aName == commandName )
			{
				commandCalled = true;
				cvar->SetValue( args[0] );

				// TODO: convar callbacks should have different parameters
				if ( cvar->aFunc )
					cvar->aFunc( args );

				DeleteCommand(  );
				break;
			}
		}

		if ( commandCalled )
			continue;

		for ( const auto& cmd : aConCommandList )
		{
			if ( cmd->aName == commandName )
			{
				commandCalled = true;
				cmd->aFunc( args );
				DeleteCommand(  );
				break;
			}
		}

		if ( commandCalled )
			continue;

		// command wasn't used?
		std::string warningMsg = "Command \"" + commandName + "\" is undefined\n";

		// blech
		printf( warningMsg.c_str() );
		aConsoleHistory += warningMsg;

		DeleteCommand(  );
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
