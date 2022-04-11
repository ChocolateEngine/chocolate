#include "core/filesystem.h"
#include "core/console.h"
#include "core/profiler.h"
#include "util.h"

#include <stdarg.h>
#include <iostream>
#include <fstream>
#include <mutex>

LOG_REGISTER_CHANNEL( Console, LogColor::Gray );

DLL_EXPORT Console* console = nullptr;


struct ConVarFlagData_t
{
	ConVarFlag_t flag;
	const char*  name;
	size_t       nameLen;
};

std::vector< ConVarFlagData_t > gConVarFlags;

std::unordered_map< ConVarBase*, std::string > gConVarLowercaseNames;


DLL_EXPORT ConVarFlag_t CVARF_NONE = 0;

DLL_EXPORT NEW_CVAR_FLAG( CVARF_ARCHIVE );
DLL_EXPORT NEW_CVAR_FLAG( CVARF_INSTANT );


// ================================================================================


// convars to register after static initalization
ConVarBase* ConVarBase::spConVarBases = nullptr;


ConVarBase::ConVarBase( const std::string& name, ConVarFlag_t flags ):
	aName( name ), aDesc(), aFlags( flags )
{
	apNext = spConVarBases;
	spConVarBases = this;
}

ConVarBase::ConVarBase( const std::string& name, const std::string& desc ):
	aName( name ), aDesc( desc ), aFlags()
{
	apNext = spConVarBases;
	spConVarBases = this;
}

ConVarBase::ConVarBase( const std::string& name, ConVarFlag_t flags, const std::string& desc ):
	aName( name ), aDesc( desc ), aFlags( flags )
{
	apNext = spConVarBases;
	spConVarBases = this;
}

const std::string& ConVarBase::GetName(  )
{
	return aName;
}

const std::string& ConVarBase::GetDesc(  )
{
	return aDesc;
}

ConVarFlag_t ConVarBase::GetFlags(  )
{
	return aFlags;
}

ConVarBase* ConVarBase::GetNext(  )
{
	return apNext;
}


// ================================================================================


void ConCommand::Init( ConCommandFunc func, ConCommandDropdownFunc dropDownFunc )
{
	aFunc = func;
	aDropDownFunc = dropDownFunc;
}

std::string ConCommand::GetPrintMessage(  )
{
	if ( aDesc.empty() )
		return aName + "\n";

	return aName + "\n\t" + aDesc + "\n";
}

// ================================================================================


void ConVar::Init( const std::string& defaultValue, ConVarFunc callback )
{
	aDefaultValue = defaultValue;
	aDefaultValueFloat = ToDouble( defaultValue, 0.f );
	aFunc = callback;

	SetValue( defaultValue );
}


void ConVar::Init( float defaultValue, ConVarFunc callback )
{
	aDefaultValue = ToString( defaultValue );
	aDefaultValueFloat = defaultValue;
	aFunc = callback;

	SetValue( aDefaultValue );
}

std::string ConVar::GetPrintMessage(  )
{
	std::string msg = aName + " = " + GetValue() + " (" + aDefaultValue + " default)\n";

	if ( aDesc.size() )
		return msg + "\t" + aDesc + "\n\n";

	return msg;
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
	Init( aDefaultValue, aFunc );
}


const std::string& ConVar::GetValue(  )
{
	return aValue;
}

float ConVar::GetFloat(  )
{
	return aValueFloat;
}

int ConVar::GetInt(  )
{
	return (int)aValueFloat;
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
	for ( const auto& file : filesys->ScanDir( "cfg", ReadDir_AllPaths | ReadDir_Recursive ) )
	{
		if ( file.ends_with( ".." ) )
			continue;

		std::string execName = filesys->GetFileName( file );


		if ( args.size() && !execName.starts_with( args[0] ) )
			continue;

		results.push_back( execName );
	}
}


CONCMD_DROP_VA( exec, exec_dropdown, 0, "Execute a script full of console commands" )
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
		LogWarn( gConsoleChannel, "File does not exist: \"%s\"\n", path.c_str() );
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


CONCMD_VA( echo, "Print a string to the console" )
{
	// std::string msg = "";
	auto p = args.begin();
	std::string msg = *p++;

	if (p == args.end() - 1)
		msg += " ";

	// for (auto p = args.begin(); p != args.end(); p)
	for (; p != args.end(); ++p)
	{
		msg += *p;

		if (p == args.end() - 1)
			msg += " ";
	}

	msg += "\n";

	LogPuts( gConsoleChannel, msg.c_str() );
}


void help_dropdown(
	const std::vector< std::string >& args,  // arguments currently typed in by the user
	std::vector< std::string >& results )      // results to populate the dropdown list with
{
	std::string name = args.empty() ? "" : str_lower2(args[0]);

	ConVarBase* cvar = ConVarBase::spConVarBases;
	while ( cvar )
	{
		cvar = console->CheckForConVarRef( cvar );
		if ( !cvar )
			break;  // wtf

		if ( !gConVarLowercaseNames[cvar].starts_with( name ) )
		{
			cvar = cvar->apNext;
			continue;
		}

		results.push_back( cvar->GetName() );
		cvar = cvar->apNext;
	}
}


CONCMD_DROP_VA( help, help_dropdown, 0, "If no args specified, Print all Registered ConVars, otherwise, print information about this convar" )
{
	if ( args.empty() )
	{
		console->PrintAllConVars();
		return;
	}

	ConVarBase* cvar = console->GetConVarBase( args[0] );

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
CONCMD_VA( find, "Search if cvar name contains any of the search arguments" )
{
	if ( args.empty() )
	{
		LogMsg( gConsoleChannel, "%s\n", find_cmd.GetDesc().c_str() );
		return;
	}

	CmdFind( false, args );
}

CONCMD_VA( findand, "Search if cvar name contains all of the search arguments" )
{
	if ( args.empty() )
	{
		LogMsg( gConsoleChannel, "%s\n", findand_cmd.GetDesc().c_str() );
		return;
	}

	CmdFind( true, args );
}


void reset_cvar_dropdown(
	const std::vector< std::string >& args,  // arguments currently typed in by the user
	std::vector< std::string >& results )      // results to populate the dropdown list with
{
	std::string name = args.empty() ? "" : str_lower2(args[0]);

	ConVarBase* cvar = ConVarBase::spConVarBases;
	while ( cvar )
	{
		cvar = console->CheckForConVarRef( cvar );
		if ( !cvar )
			break;  // wtf

		if ( IS_NOT_TYPE( *cvar, ConVar ) || !gConVarLowercaseNames[cvar].starts_with( name ) )
		{
			cvar = cvar->apNext;
			continue;
		}

		results.push_back( cvar->GetName() );
		cvar = cvar->apNext;
	}
}


// use if you need to quit the engine right now
CONCMD_DROP_VA( reset_cvar, reset_cvar_dropdown, 0, "reset a convar back to it's default value" )
{
	if ( args.empty() )
	{
		LogPuts( gConsoleChannel, "No ConVar specified to reset!\n" );
		return;
	}

	ConVar* cvar = console->GetConVar( args[0] );

	if ( cvar )
	{
		cvar->Reset();
	}
	else
	{
		LogWarn( gConsoleChannel, "Convar not found: %s\n", args[0].c_str() );
	}
}


// use if you need to quit the engine right now
CONCMD_VA( _abort, CVARF_INSTANT, "funny" )
{
	abort();
}


// remove duplicate user inputs from the history
CONVAR( con_remove_dup_input_history, 1 );


// ================================================================================


void Console::AddToCommandHistory( const std::string &srCmd )
{
	if ( srCmd.empty() )
		return;

	if ( con_remove_dup_input_history )
	{
		vec_remove_if( aCommandHistory, srCmd );
		aCommandHistory.push_back( srCmd );
	}
	else
	{
		if ( aCommandHistory.empty() || aCommandHistory.back() != srCmd )
			aCommandHistory.push_back( srCmd );
	}
}


void Console::CheckInstantCommands( const std::string &srCmd )
{
	for ( size_t i = 0; i < aInstantConVars.size(); i++ )
	{
		if ( aInstantConVars[i]->GetName() == srCmd )
			aInstantConVars[i]->aFunc( {} );
	}
}


void Console::QueueCommand( const std::string &srCmd )
{
	CheckInstantCommands( srCmd );

	aQueue.push_back( srCmd );

	AddToCommandHistory( srCmd );

	LogEx( gConsoleChannel, LogType::Input, srCmd.c_str() );
}

void Console::QueueCommandSilent( const std::string &srCmd, bool sAddToHistory )
{
	CheckInstantCommands( srCmd );

	aQueue.push_back( srCmd );

	if ( sAddToHistory )
		AddToCommandHistory( srCmd );
}


// TODO: rethink this function
void Console::RegisterConVars(  )
{
	PROF_SCOPE();

	// static bool registered = false;

	//if ( registered )
	//	return;

	std::vector< ConVarRef* > cvarRefList;

	aInstantConVars.clear();

	ConVarBase* current = ConVarBase::spConVarBases;
	while ( current )
	{
		if ( typeid(*current) == typeid(ConVarRef) )
		{
			ConVarRef* cvarRef = static_cast<ConVarRef*>(current);

			if ( cvarRef->apRef == nullptr )
				cvarRefList.push_back( cvarRef );
		}

		if ( current->aFlags & CVARF_INSTANT && typeid(*current) == typeid(ConCommand) )
			aInstantConVars.push_back( static_cast<ConCommand*>(current) );

		gConVarLowercaseNames[current] = str_lower2( current->aName );

		current = current->apNext;
	}

	// Now link all cvar references
	for ( ConVarRef* cvarRef: cvarRefList )
		cvarRef->SetReference( GetConVar( cvarRef->GetName() ) );

	// registered = true;
}


const std::vector< std::string >& Console::GetCommandHistory(  )
{
	return aCommandHistory;
}


// this is stupid
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
	PROF_SCOPE();

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


ConVarBase* Console::GetConVarBase( const std::string& name )
{
	PROF_SCOPE();

	ConVarBase* cvar = ConVarBase::spConVarBases;
	while ( cvar )
	{
		if ( cvar->aName == name )
			return cvar;

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


ConVarBase* Console::CheckForConVarRef( ConVarBase* cvar )
{
	PROF_SCOPE();

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
	PROF_SCOPE();

	aAutoCompleteList.clear();

	if ( textBuffer.empty() )
		return;

	std::string commandName;
	std::vector< std::string > args;
	ParseCommandLine( textBuffer, commandName, args );

	str_lower( commandName );

	ConVarBase* cvar = ConVarBase::spConVarBases;
	while ( cvar )
	{
		cvar = CheckForConVarRef( cvar );
		if ( !cvar )
			continue;

		// this SHOULD be fine, if the convar doesn't exist in here, something is very wrong
		if ( !gConVarLowercaseNames[cvar].starts_with( commandName ) )
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
	PROF_SCOPE();

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


void Console::RunCommand( const std::string& command )
{
	PROF_SCOPE();

	std::string commandName;
	std::vector< std::string > args;

	for ( size_t i = 0; i < command.size(); i++ )
	{
		commandName.clear();
		args.clear();

		ParseCommandLine( command, commandName, args, i );
		str_lower( commandName );

		RunCommand( commandName, args );
	}
}


bool Console::RunCommand( const std::string &name, const std::vector< std::string > &args )
{
	PROF_SCOPE();

	bool commandCalled = false;

	ConVarBase* cvar = ConVarBase::spConVarBases;
	while ( cvar )
	{
		if ( gConVarLowercaseNames[cvar] == name )
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
					std::string prevString = convar->aValue;
					float       prevFloat  = convar->aValueFloat;

					convar->SetValue( args[0] );

					// TODO: convar callbacks should have different parameters
					if ( convar->aFunc )
						convar->aFunc( prevString, prevFloat, args );
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
		LogWarn( gConsoleChannel, "Command \"%s\" is undefined\n", name.c_str() );

	return commandCalled;
}


void Console::ParseCommandLine( const std::string &command, std::string& name, std::vector< std::string >& args )
{
	size_t i = 0;
	ParseCommandLine( command, name, args, i );
}

void Console::ParseCommandLine( const std::string &command, std::string& name, std::vector< std::string >& args, size_t& i )
{
	PROF_SCOPE();

	std::string curArg;

	for ( ; i < command.size(); i++ )
	{
		if ( name.size() )
		{
			if ( command[i] == ' ' )
				break;

			name += command[i];
		}
		else if ( command[i] == ' ' )
		{
			continue;
		}
		else
		{
			name += command[i];
		}
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


ConVarFlag_t Console::CreateCvarFlag( const char* name )
{
	ConVarFlag_t newBitShift = (1 << gConVarFlags.size());
	size_t len = strlen( name );
	gConVarFlags.emplace_back( newBitShift, name, len );
	return newBitShift;
}

const char* Console::GetCvarFlagName( ConVarFlag_t flag )
{
	for ( auto& data : gConVarFlags )
	{
		if ( data.flag == flag )
			return data.name;
	}

	return nullptr;
}

ConVarFlag_t Console::GetCvarFlag( const char* name )
{
	size_t len = strlen( name );

	for ( auto& data : gConVarFlags )
	{
		if ( data.nameLen != len )
			continue;

		if ( strncmp( data.name, name, len ) == 0 )
			return data.flag;
	}

	return CVARF_NONE;
}


Console::Console(  )
{
	console = this;
}

Console::~Console(  )
{
}


Console& GetConsole()
{
	if ( console == nullptr )
		console = new Console;

	return *console;
}
