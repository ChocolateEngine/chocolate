#include "../../inc/shared/console.h"
#include "../../inc/shared/util.h"

#include <stdarg.h>

Console* g_console = nullptr;


// ================================================================================


// convars to register after static initalization
ConVarBase* ConVarBase::spConVarBases = nullptr;


ConVarBase::ConVarBase( const std::string& name )
{
	aName = name;

	apNext = spConVarBases;
	spConVarBases = this;
}

const std::string& ConVarBase::GetName(  )
{
	return aName;
}

ConVarBase* ConVarBase::GetNext(  )
{
	return apNext;
}


// ================================================================================


void ConCommand::Init( const std::string& name, ConVarFunc func )
{
	aName = name;
	aFunc = func;
}

std::string ConCommand::GetPrintMessage(  )
{
	return aName + "\n";
}

// ================================================================================


void ConVar::Init( const std::string& name, const std::string& defaultValue, ConVarFunc callback )
{
	aName = name;
	aDefaultValue = defaultValue;
	aFunc = callback;

	SetValue( defaultValue );
}

std::string ConVar::GetPrintMessage(  )
{
	return aName + " = " + GetValue() + " (" + aDefaultValue + " default)\n";
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

	ConVarBase *current;
	ConVarBase *next;

	current = ConVarBase::spConVarBases;
	while ( current )
	{
		next = current->apNext;

		if ( !vec_contains( aConVarList, current ) )
			aConVarList.push_back( current );

		current = next;
	}

	registered = true;

	ConVarBase::spConVarBases = nullptr;
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
	CalculateAutoCompleteList(  );
}

const std::string& Console::GetTextBuffer( )
{
	return aTextBuffer;
}


void Console::PrintAllConVars( )
{
	Print( "\nConVars:\n--------------------------------------\n" );

	for ( const auto& cvar : aConVarList )
	{
		Print( cvar->GetPrintMessage().c_str() );
	}

	/*Print( "\nConCommands:\n--------------------------------------\n" );

	for ( const auto& cmd : aConCommandList )
	{
		Print( cmd->GetPrintMessage().c_str() );
	}*/

	Print( "--------------------------------------\n" );
}


void Console::Print( const char* str, ... )
{
	char buffer[8192];
	va_list args;

	va_start( args, str );

	vsnprintf( buffer, 8192, str, args );

	printf( buffer );
	aConsoleHistory += buffer;
	va_end( args );
}


void Console::CalculateAutoCompleteList( )
{
	aAutoCompleteList.clear();

	if ( aTextBuffer.empty() )
		return;

	for ( const auto& cvar : aConVarList )
	{
		if ( cvar->aName.starts_with( aTextBuffer ) )
		{
			aAutoCompleteList.push_back( cvar->aName );
		}
	}

	/*for ( const auto& cmd : aConCommandList )
	{
		if ( cmd->aName.starts_with( aTextBuffer ) )
		{
			aAutoCompleteList.push_back( cmd->aName );
		}
	}*/
}


const std::vector< std::string >& Console::GetAutoCompleteList( )
{
	return aAutoCompleteList;
}


void Console::Update(  )
{
	// HACK HACK HACK
	// only needed for when creating cvars in functions
	RegisterConVars(  );

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
				if ( ConVar* convar = dynamic_cast<ConVar*>(cvar) )
				{
					commandCalled = true;

					if ( !args.empty() )
					{
						convar->SetValue( args[0] );

						// TODO: convar callbacks should have different parameters
						if ( convar->aFunc )
							convar->aFunc( args );
					}
					else
					{
						Print( convar->GetPrintMessage().c_str() );
					}
				}
				else if ( ConCommand* cmd = dynamic_cast<ConCommand*>(cvar) )
				{
					commandCalled = true;
					cmd->aFunc( args );
				}

				DeleteCommand(  );
				break;
			}
		}

		if ( commandCalled )
			continue;

		// command wasn't used?
		Print( "Command \"%s\" is undefined\n", commandName.c_str() );

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
	g_console = this;
}

Console::~Console(  )
{
}
