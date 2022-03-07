#include "core/filesystem.h"
#include "core/console.h"
#include "util.h"

#include <stdarg.h>
#include <iostream>
#include <fstream>
#include <mutex>

LOG_REGISTER_CHANNEL( Console, LogColor::Gray );

DLL_EXPORT Console* console = nullptr;


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


void ConCommand::Init( const std::string& name, ConVarFunc func, ConCommandDropdownFunc dropDownFunc )
{
	aName = name;
	aFunc = func;
	aDropDownFunc = dropDownFunc;
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

void exec_dropdown(
	const std::vector< std::string >& args,  // arguments currently typed in by the user
	std::vector< std::string >& results )      // results to populate the dropdown list with
{
	for ( const auto file : filesys->ScanDir( "cfg", ReadDir_AllPaths | ReadDir_Recursive ) )
	{
		if ( file.ends_with( ".." ) )
			continue;

		std::string execName = filesys->GetFileName( file );


		if ( args.size() && !execName.starts_with( args[0] ) )
			continue;

		results.push_back( execName );
	}
}


CONCMD_DROP( exec, exec_dropdown )
{
	if ( args.size() == 0 )
	{
		LogMsg( gConsoleChannel, "No Path Specified for exec!\n" );
		return;
	}

	std::string path = "cfg/" + args[0];

	if ( !filesys->IsFile( path ) )
	{
		if ( !path.ends_with( ".cfg" ) )
			path += ".cfg";
	}

	if ( !filesys->IsFile( path ) )
	{
		LogWarn( gConsoleChannel, "File does not exist: \"%s\"\n", path );
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
					LogWarn( gConsoleChannel, "cfg file trying to exec itself and cause infinite recursion\n" );
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

	LogMsg( gConsoleChannel, msg.c_str() );
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
		LogMsg( gConsoleChannel, cvar->GetPrintMessage().c_str() );
	}
	else
	{
		LogWarn( gConsoleChannel, "Convar not found: %s\n", args[0].c_str() );
	}
}


void FindStr( bool andSearch, ConVarBase* cvar, const std::vector< std::string >& args, std::vector< std::string >& results )
{
	for ( auto arg : args )
	{
		if ( cvar->GetName().find( arg ) != std::string::npos )
		{
			results.push_back( "\t" + cvar->GetPrintMessage() );

			if ( !andSearch )
				return;
		}
	}
}


void CmdFind( bool andSearch, std::vector< std::string >& args )
{
	std::vector< std::string > resultsCvar;
	std::vector< std::string > resultsCCmd;

	ConVarBase* cvar = ConVarBase::spConVarBases;
	while ( cvar )
	{
		// ugly but probably faster than doing an extra dynamic cast to check if not a cvarref
		// or doing the string search on a cvarref
		if ( IS_TYPE( *cvar, ConVar ) )
		{
			FindStr( andSearch, cvar, args, resultsCvar );
		}
		else if ( IS_TYPE( *cvar, ConCommand ) )
		{
			FindStr( andSearch, cvar, args, resultsCCmd );
		}

		cvar = cvar->apNext;
	}

	LogMsg( gConsoleChannel, "Search Results: %zu\n", resultsCvar.size() + resultsCCmd.size() );

	LogMsg( gConsoleChannel, "\nConVars: %zu\n--------------------------------------\n", resultsCvar.size() );
	for ( const auto& msg : resultsCvar )
		LogPuts( gConsoleChannel, msg.c_str() );

	LogMsg( gConsoleChannel, "\nConCommands: %zu\n--------------------------------------\n", resultsCCmd.size() );
	for ( const auto& msg : resultsCCmd )
		LogPuts( gConsoleChannel, msg.c_str() );

	LogPuts( gConsoleChannel, "--------------------------------------\n" );
}


// IDEA: later when you add in ConVar flags and descriptions, make a findex cmd that you can choose specific parts to search
// like only desc, or only name, or both, and probably the rest of the args for flag searching
// and if search string is empty but flags isn't, just list all for those flags
CONCMD( find )
{
	if ( args.size() == 0 )
	{
		LogPuts( gConsoleChannel, "Search if cvar name contains any of the search arguments\n" );
		return;
	}

	CmdFind( false, args );
}

CONCMD( findand )
{
	if ( args.size() == 0 )
	{
		LogPuts( gConsoleChannel, "Search if cvar name contains all of the search arguments\n" );
		return;
	}

	CmdFind( true, args );
}


// ================================================================================

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
		if ( typeid(*current) == typeid(ConVarRef) )
		{
			ConVarRef* cvarRef = static_cast<ConVarRef*>(current);

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
	Print( "] %s\n", str.c_str() );
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
		if ( typeid(*cvar) == typeid(ConVar) )
		{
			if ( cvar->aName == name )
				return static_cast<ConVar*>(cvar);
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
		if ( typeid(*cvar) == typeid(ConVar)  )
			ConVarMsgs.push_back( cvar->GetPrintMessage() );

		else if ( typeid(*cvar) == typeid(ConCommand) )
			ConCommandMsgs.push_back( cvar->GetPrintMessage() );

		cvar = cvar->apNext;
	}

	LogPuts( gConsoleChannel, "\nConVars:\n--------------------------------------\n" );
	for ( const auto& msg : ConVarMsgs )
		LogPuts( gConsoleChannel, msg.c_str() );

	LogPuts( gConsoleChannel, "\nConCommands:\n--------------------------------------\n" );
	for ( const auto& msg : ConCommandMsgs )
		LogPuts( gConsoleChannel, msg.c_str() );

	LogPuts( gConsoleChannel, "--------------------------------------\n" );
}


void Console::Print( const char* format, ... )
{
	VSTRING( std::string buffer, format );
	printf( buffer.c_str() );
}


// Checks if the current convar is a reference, if so, then return what it's pointing to
// return nullptr if it doesn't point to anything, and return the normal cvar if it's not a convarref
ConVarBase* CheckForConVarRef( ConVarBase* cvar )
{
	if ( typeid(*cvar) == typeid(ConVarRef) )
	{
		ConVarRef* cvarRef = static_cast<ConVarRef*>(cvar);

		if ( cvarRef->apRef == nullptr )
		{
			LogWarn( gConsoleChannel, "Found unlinked cvar ref: %s\n", cvarRef->GetName().c_str() );
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

	std::string commandName;
	std::vector< std::string > args;
	ParseCommandLine( textBuffer, commandName, args );

	ConVarBase* cvar = ConVarBase::spConVarBases;
	while ( cvar )
	{
		cvar = CheckForConVarRef( cvar );
		if ( !cvar )
			continue;

		if ( !str_lower2( cvar->aName ).starts_with( str_lower2( commandName ) ) )
		{
			cvar = cvar->apNext;
			continue;
		}
		
		// if ( args.empty() )
		if ( cvar->aName.length() >= textBuffer.length() )
		{
			aAutoCompleteList.push_back( cvar->aName );
			cvar = cvar->apNext;
			continue;
		}

		// is this a concommand with a drop down function?
		if ( typeid(*cvar) == typeid(ConCommand) )
		{
			auto cmd = static_cast<ConCommand*>(cvar);

			if ( cmd->aDropDownFunc )
			{
				std::vector< std::string > dropDownArgs;
				cmd->aDropDownFunc( args, dropDownArgs );

				for ( auto dropArg: dropDownArgs )
				{
					aAutoCompleteList.push_back( cvar->aName + " " + dropArg );
				}
			}
			else
			{
				aAutoCompleteList.push_back( cvar->aName );
			}
		}
		else
		{
			aAutoCompleteList.push_back( cvar->aName );
		}

		break;
	}
}


const std::vector< std::string >& Console::GetAutoCompleteList( )
{
	return aAutoCompleteList;
}


void Console::Update(  )
{
	static bool init = false;

	if ( !init )
	{
		// TODO: rethink this stupid thing
		// also call again if convar count changes
		RegisterConVars(  );
		init = true;
	}

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

	ParseCommandLine( command, commandName, args );

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

			if ( typeid(*cvar) == typeid(ConVar) )
			{
				ConVar* convar = static_cast<ConVar*>(cvar);

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
					LogPuts( gConsoleChannel, convar->GetPrintMessage().c_str() );
				}
			}
			else if ( typeid(*cvar) == typeid(ConCommand) )
			{
				ConCommand* cmd = static_cast<ConCommand*>(cvar);
				commandCalled = true;
				cmd->aFunc( args );
			}

			break;
		}

		cvar = cvar->apNext;
	}

	// command wasn't used?
	if ( !commandCalled )
		LogWarn( gConsoleChannel, "Command \"%s\" is undefined\n", commandName.c_str() );

	return commandCalled;
}


void Console::ParseCommandLine( const std::string &command, std::string& name, std::vector< std::string >& args )
{
	std::string curArg;

	int i = 0;
	for ( ; i < command.size(); i++ )
	{
		if ( command[i] == ' ' )
			break;
		name += command[i];
	}

	i++;

	for ( ; i < command.size(); i++ )
	{
		char ch = command[i];

#ifdef _WIN32
		if ( ch == '\r' )
			continue;
#endif

		if ( ch == '\n' || ch == ';' )
			break;

		// if a space, add
		if ( ch == ' ' )
		{
			if ( curArg.size() )
				args.push_back( curArg );

			curArg = "";
		}

		// are we entering a quote?
		else if ( ch == '"' || ch == '\'' )
		{
			char q = ch;

			for ( ; i < command.size(); i++ )
			{
				if ( command[i] == q )
					break;
				else
					curArg += command[i];
			}

			if ( curArg.size() )
				args.push_back( curArg );

			curArg = "";
		}
		else
		{
			curArg += ch;
		}
	}

	if ( curArg.size() )
		args.push_back( curArg );
}


Console::Console(  )
{
	console = this;
}

Console::~Console(  )
{
}
