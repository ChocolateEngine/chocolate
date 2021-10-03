#include "../../inc/shared/console.h"
#include "../../inc/shared/util.h"

#include <stdarg.h>

Console* console = nullptr;


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
	aDefaultValueFloat = ToDouble( defaultValue, 0.f );
	aFunc = callback;

	SetValue( defaultValue );
}

void ConVar::Init( const std::string& name, float defaultValue, ConVarFunc callback )
{
	aName = name;
	aDefaultValue = ToString( defaultValue );
	aDefaultValueFloat = defaultValue;
	aFunc = callback;

	SetValue( aDefaultValue );
}

std::string ConVar::GetPrintMessage(  )
{
	return aName + " = " + GetValue() + " (" + aDefaultValue + " default)\n";
}


void ConVar::SetValue( const std::string& value )
{
	aValue = value;
	aValueFloat = ToDouble( value, aValueFloat );
}

void ConVar::SetValue( float value )
{
	aValue = ToString( value );
	aValueFloat = value;
}


void ConVar::Reset(  )
{
	Init( aName, aDefaultValue, aFunc );
}


const std::string& ConVar::GetValue(  )
{
	return aValue;
}

float ConVar::GetFloat(  )
{
	return aValueFloat;
}

bool ConVar::GetBool(  )
{
	return aValueFloat == 1.f;
}


// amazing, have to put these here or i get some ConVarRef undefined error
// also not const due to the function call
bool    ConVar::operator<=( ConVarRef& other )              { return aValueFloat <= other.GetFloat(); }
bool    ConVar::operator>=( ConVarRef& other )              { return aValueFloat >= other.GetFloat(); }
bool    ConVar::operator==( ConVarRef& other )              { return aValueFloat == other.GetFloat(); }
bool    ConVar::operator!=( ConVarRef& other )              { return aValueFloat != other.GetFloat(); }

float   ConVar::operator*( ConVarRef& other )               { return aValueFloat * other.GetFloat(); }
float   ConVar::operator/( ConVarRef& other )               { return aValueFloat / other.GetFloat(); }
float   ConVar::operator+( ConVarRef& other )               { return aValueFloat + other.GetFloat(); }
float   ConVar::operator-( ConVarRef& other )               { return aValueFloat - other.GetFloat(); }


// ================================================================================


void ConVarRef::Init(  )
{
	if ( console == nullptr )
		return;

	// if this is created later, just search for the convar
	SetReference( console->GetConVar( aName ) );
}

void ConVarRef::SetReference( ConVar* ref )
{
	apRef = ref;
	aValid = apRef != nullptr;
}

// should never be called on a ConVarRef really
std::string ConVarRef::GetPrintMessage(  )
{
	return apRef ? apRef->GetPrintMessage(  ) : "Invalid ConVarRef: " + aName + "\n";
}

void ConVarRef::SetValue( const std::string& value )
{
	if ( apRef ) SetValue( value );
}

void ConVarRef::SetValue( float value )
{
	if ( apRef ) SetValue( value );
}

const std::string& ConVarRef::GetValue(  )
{
	static std::string empty = "";
	return apRef ? apRef->aValue : empty;
}

float ConVarRef::GetFloat(  )
{
	return apRef ? apRef->aValueFloat : 0.f;
}

bool ConVarRef::GetBool(  )
{
	return apRef ? apRef->GetBool(  ) : false;
}


// uhhh
//bool operator<=( ConVar& left, ConVarRef& other )              { return left.GetFloat() <= other.GetFloat(); }
//bool operator>=( ConVar& left, ConVarRef& other )              { return left.GetFloat() >= other.GetFloat(); }
//bool operator==( ConVar& left, ConVarRef& other )              { return left.GetFloat() == other.GetFloat(); }


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

	std::vector< ConVarRef* > cvarRefList;

	current = ConVarBase::spConVarBases;
	while ( current )
	{
		next = current->apNext;

		if ( ConVarRef* cvarRef = dynamic_cast< ConVarRef* >(current) )
		{
			cvarRefList.push_back( cvarRef );
		}
		else
		{
			if ( !vec_contains( aConVarList, current ) )
				aConVarList.push_back( current );
		}

		current = next;
	}

	// Now link all cvar references
	for ( ConVarRef* cvarRef: cvarRefList )
	{
		cvarRef->SetReference( GetConVar( cvarRef->GetName() ) );
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

const std::string& Console::GetConsoleHistory(  )
{
	return aConsoleHistory;
}

const std::vector< std::string >& Console::GetCommandHistory(  )
{
	return aCommandHistory;
}

void Console::SetTextBuffer( const std::string& str, bool recalculateList )
{
	aTextBuffer = str;

	if ( recalculateList )
		CalculateAutoCompleteList( aTextBuffer );
}

const std::string& Console::GetTextBuffer(  )
{
	return aTextBuffer;
}


ConVar* Console::GetConVar( const std::string& name )
{
	for ( const auto& cvar : aConVarList )
	{
		if ( ConVar* convar = dynamic_cast<ConVar*>(cvar) )
		{
			if ( convar->aName == name )
				return convar;
		}
	}

	return nullptr;
}


std::string Console::GetConVarValue( const std::string& name )
{
	if ( ConVar* convar = GetConVar(name) )
		return convar->aValue;
	
	return "";
}


float Console::GetConVarFloat( const std::string& name )
{
	if ( ConVar* convar = GetConVar(name) )
		return convar->aValueFloat;
	
	return 0.f;
}


void Console::PrintAllConVars(  )
{
	std::vector< std::string > ConVarMsgs;
	std::vector< std::string > ConCommandMsgs;

	for ( const auto& cvar : aConVarList )
	{
		if ( dynamic_cast<ConVar*>(cvar) )
			ConVarMsgs.push_back( cvar->GetPrintMessage() );

		else if ( dynamic_cast<ConCommand*>(cvar) )
			ConCommandMsgs.push_back( cvar->GetPrintMessage() );
	}

	Print( "\nConVars:\n--------------------------------------\n" );
	for ( const auto& msg : ConVarMsgs )
		Print( msg.c_str() );

	Print( "\nConCommands:\n--------------------------------------\n" );
	for ( const auto& msg : ConCommandMsgs )
		Print( msg.c_str() );

	Print( "--------------------------------------\n" );
}


void Console::Print( const char* format, ... )
{
	VSTRING( std::string buffer, format );
	printf( buffer.c_str() );
	aConsoleHistory += buffer;
	va_end( args );
}


void Console::Print( Msg type, const char* format, ... )
{
	VSTRING( std::string buffer, format );
	printf( buffer.c_str() );
	aConsoleHistory += buffer;
	va_end( args );
}


void Console::CalculateAutoCompleteList( const std::string& textBuffer )
{
	aAutoCompleteList.clear();

	if ( textBuffer.empty() )
		return;

	for ( const auto& cvar : aConVarList )
	{
		if ( cvar->aName.starts_with( textBuffer ) )
		{
			aAutoCompleteList.push_back( cvar->aName );
		}
	}
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
	console = this;
}

Console::~Console(  )
{
}
