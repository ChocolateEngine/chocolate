#include "core/console.h"
#include "util.h"

#include <stdarg.h>
#include <iostream>
#include <fstream>

DLL_EXPORT Console* console = nullptr;


void Print( const char* format, ... )
{
	VSTRING( std::string buffer, format );
	if ( console )
		console->Print( buffer.c_str() );
	else
		printf( buffer.c_str() );
}


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
	return aValueFloat >= 1.f;
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


// built in ConCommands

CONCMD( exec )
{
	if ( args.size() == 0 )
	{
		Print( "No Path Specified for exec!\n" );
		return;
	}

	std::string path = "cfg/" + args[0] + ".cfg";

	if ( !FileExists( path ) )
	{
		Print( "Warning: File does not exist: \"%s\"\n", path );
		return;
	}

	std::ifstream fileStream = std::ifstream(path, std::ios::in | std::ios::binary | std::ios::ate);

	int fileLen = fileStream.tellg();
	fileStream.seekg(0, fileStream.beg);

	char* buf = new char[fileLen];
	fileStream.read(buf, fileLen);
	fileStream.close();

	int ch = 0;
	bool inComment = false;
	std::string line;

	// parse it
	while ( ch < fileLen )
	{
#ifdef _WIN32
		if ( buf[ch] == '\r' )
		{
			ch++;
			continue;
		}
		else
#endif
		if ( buf[ch] == '\n' || buf[ch] == ';' )
		{
			if ( line != "" )
			{
				if ( line == "exec " + args[0] )
					Print( "[Console] Warning: cfg file trying to exec itself and cause infinite recursion\n" );
				else
					console->RunCommand( line );

				line = "";
			}

			inComment = false;
		}
		// check for a comment
		else if ( buf[ch] == '/' && ch + 1 < fileLen && buf[ch + 1] == '/' )
		{
			inComment = true;
			ch++;
		}
		else if ( !inComment )
		{
			line += buf[ch];
		}

		ch++;
	}

	if ( line != "" )
		console->RunCommand( line );

	delete[] buf;
}

CONCMD( echo )
{
	std::string msg = "";

	for ( int i = 0; i < args.size(); i++ )
	{
		//const auto& str = args[i];

		if ( i + 1 < args.size() )
			msg += args[i] + " ";
		else
			msg += args[i] + "\n";
	}

	Print( msg.c_str() );
}

CONCMD( help )
{
	if ( args.size() == 0 )
	{
		console->PrintAllConVars();
		return;
	}

	ConVarBase* cvar = console->GetConVar( args[0] );

	if ( cvar )
	{
		Print( cvar->GetPrintMessage().c_str() );
	}
	else
	{
		Print( "Convar not found: %s\n", args[0].c_str() );
	}
}


// ================================================================================

void Console::ReadConfig( const std::string& name )
{
	
}

void Console::Add( const std::string &srCmd )
{
	aQueue.push_back( srCmd );

	if ( aCommandHistory.empty() || aCommandHistory.back() != srCmd )
		aCommandHistory.push_back( srCmd );

	AddToHistory( srCmd );
}

void Console::AddSilent( const std::string &srCmd )
{
	aQueue.push_back( srCmd );

	if ( aCommandHistory.empty() || aCommandHistory.back() != srCmd )
		aCommandHistory.push_back( srCmd );
}

// Only registers cvar refs
void Console::RegisterConVars(  )
{
	// static bool registered = false;

	//if ( registered )
	//	return;

	std::vector< ConVarRef* > cvarRefList;

	ConVarBase* current = ConVarBase::spConVarBases;
	while ( current )
	{
		if ( ConVarRef* cvarRef = dynamic_cast< ConVarRef* >(current) )
		{
			if ( cvarRef->apRef == nullptr )
				cvarRefList.push_back( cvarRef );
		}

		current = current->apNext;
	}

	// Now link all cvar references
	for ( ConVarRef* cvarRef: cvarRefList )
		cvarRef->SetReference( GetConVar( cvarRef->GetName() ) );

	// registered = true;
}


void Console::AddToHistory( const std::string& str )
{
	std::string cmd = "] " + str + "\n";
	aConsoleHistory.push_back( cmd );
	printf( cmd.c_str() );
}

const std::vector<std::string>& Console::GetConsoleHistory(  )
{
	return aConsoleHistory;
}

std::string Console::GetConsoleHistoryStr( int maxSize )
{
	// make this static?
	std::string output;

	// No limit
	if ( maxSize == -1 )
	{
		for (auto& str: aConsoleHistory)
			output += str;

		return output;
	}

	// go from latest to oldest
	for (int i = aConsoleHistory.size() - 1; i > 0; i--)
	{
		int strLen = glm::min(maxSize, (int)aConsoleHistory[i].length());
		int strStart = aConsoleHistory[i].length() - strLen;

		// if the length wanted is less then the string length, then start at an offset
		output = aConsoleHistory[i].substr(strStart, strLen) + output;

		maxSize -= strLen;

		if ( maxSize == 0 )
			break;
	}

	return output;
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
	ConVarBase* cvar = ConVarBase::spConVarBases;
	while ( cvar )
	{
		if ( ConVar* convar = dynamic_cast<ConVar*>(cvar) )
		{
			if ( convar->aName == name )
				return convar;
		}

		cvar = cvar->apNext;
	}

	return nullptr;
}


static std::string g_strEmpty = "";


const std::string& Console::GetConVarValue( const std::string& name )
{
	if ( ConVar* convar = GetConVar(name) )
		return convar->aValue;
	
	return g_strEmpty;
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

	ConVarBase* cvar = ConVarBase::spConVarBases;
	while ( cvar )
	{
		if ( dynamic_cast<ConVar*>(cvar) )
			ConVarMsgs.push_back( cvar->GetPrintMessage() );

		else if ( dynamic_cast<ConCommand*>(cvar) )
			ConCommandMsgs.push_back( cvar->GetPrintMessage() );

		cvar = cvar->apNext;
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
	aConsoleHistory.push_back( buffer );
}


void Console::Print( Msg type, const char* format, ... )
{
	VSTRING( std::string buffer, format );
	printf( buffer.c_str() );
	aConsoleHistory.push_back( buffer );
}


// Checks if the current convar is a reference, if so, then return what it's pointing to
// return nullptr if it doesn't point to anything, and return the normal cvar if it's not a convarref
ConVarBase* CheckForConVarRef( ConVarBase* cvar )
{
	if ( ConVarRef* cvarRef = dynamic_cast< ConVarRef* >(cvar) )
	{
		if ( cvarRef->apRef == nullptr )
		{
			Print( "[CONSOLE] Found unlinked cvar ref: %s\n", cvarRef->GetName().c_str() );
			return nullptr;
		}

		return cvarRef->apRef;
	}

	return cvar;
}


void Console::CalculateAutoCompleteList( const std::string& textBuffer )
{
	aAutoCompleteList.clear();

	if ( textBuffer.empty() )
		return;

	ConVarBase* cvar = ConVarBase::spConVarBases;
	while ( cvar )
	{
		cvar = CheckForConVarRef( cvar );
		if ( !cvar )
			continue;

		if ( str_lower2(cvar->aName).starts_with( str_lower2(textBuffer) ) )
			aAutoCompleteList.push_back( cvar->aName );

		cvar = cvar->apNext;
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

	for ( const auto& command: aQueue )
	{
		if ( command == "" )
			continue;

		RunCommand( command );
	}

	// clear the queue
	aQueue.clear();
}


bool Console::RunCommand( const std::string& command )
{
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

	str_lower( commandName );

	bool commandCalled = false;

	ConVarBase* cvar = ConVarBase::spConVarBases;
	while ( cvar )
	{
		if ( str_lower2(cvar->aName) == commandName )
		{
			cvar = CheckForConVarRef( cvar );
			if ( !cvar )
				continue;

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

			break;
		}

		cvar = cvar->apNext;
	}

	// command wasn't used?
	if ( !commandCalled )
		Print( "Command \"%s\" is undefined\n", commandName.c_str() );

	return commandCalled;
}


Console::Console(  )
{
	console = this;
}

Console::~Console(  )
{
}
