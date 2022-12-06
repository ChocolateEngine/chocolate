#include "core/filesystem.h"
#include "core/console.h"
#include "core/profiler.h"
#include "core/platform.h"
#include "core/log.h"
#include "util.h"

#include <cstring>
#include <stdarg.h>
#include <iostream>
#include <fstream>
#include <mutex>

LOG_REGISTER_CHANNEL( Console, LogColor::Gray );


struct ConVarFlagData_t
{
	ConVarFlag_t flag;
	const char*  name;
	size_t       nameLen;
};


int                                            gCmdIndex = -1;
std::vector< std::string >                     gQueue;
std::vector< std::string >                     gCommandHistory;
std::vector< ConCommand* >                     gInstantConVars;
std::vector< ConVarFlagData_t >                gConVarFlags;

std::vector< FArchive* >                       gArchiveCallbacks;

constexpr const char*                          CON_ARCHIVE_FILE = "cfg/config.cfg";

// std::unordered_map< ConVarBase*, std::string > gConVarLowercaseNames;


DLL_EXPORT ConVarFlag_t CVARF_NONE = 0;

DLL_EXPORT NEW_CVAR_FLAG( CVARF_ARCHIVE );
DLL_EXPORT NEW_CVAR_FLAG( CVARF_INSTANT );


// ================================================================================


// convars to register after static initalization
// this is a function to ensure this variable gets initialized first
static ChVector< ConVarBase* >& Con_GetConVars()
{
	static ChVector< ConVarBase* > convars;
	return convars;
}


ConVarBase::ConVarBase( const char* name, ConVarFlag_t flags ) :
	aName( name ), aDesc( nullptr ), aFlags( flags )
{
	Con_GetConVars().push_back( this );
}

ConVarBase::ConVarBase( const char* name, const char* desc ) :
	aName( name ), aDesc( desc ), aFlags()
{
	Con_GetConVars().push_back( this );
}

ConVarBase::ConVarBase( const char* name, ConVarFlag_t flags, const char* desc ) :
	aName( name ), aDesc( desc ), aFlags( flags )
{
	Con_GetConVars().push_back( this );
}

const char* ConVarBase::GetName()
{
	return aName;
}

const char* ConVarBase::GetDesc()
{
	return aDesc;
}

ConVarFlag_t ConVarBase::GetFlags()
{
	return aFlags;
}


// ================================================================================


void ConCommand::Init( ConCommandFunc func, ConCommandDropdownFunc dropDownFunc )
{
	aFunc = func;
	aDropDownFunc = dropDownFunc;
}

std::string ConCommand::GetPrintMessage(  )
{
	if ( !aDesc )
		return vstring( UNIX_CLR_DEFAULT "%s\n", aName );

	return vstring( UNIX_CLR_DEFAULT "%s\n\t" UNIX_CLR_CYAN "%s" UNIX_CLR_DEFAULT "\n", aName, aDesc );
}

// ================================================================================


void ConVar::Init( std::string_view defaultValue, ConVarFunc callback )
{
	aDefaultValue = defaultValue;
	aDefaultValueFloat = ToDouble( defaultValue.data(), 0.f );
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

// 1 as a size_t
// 1Ui64
constexpr size_t one = 1;

std::string ConVar::GetPrintMessage()
{
	// TODO: make a shared util function for some of this, smh
	std::string msg = vstring( "%s%s %s", UNIX_CLR_DEFAULT, aName, GetValue().data() );

	if ( GetValue() != aDefaultValue )
		msg += vstring( " " UNIX_CLR_YELLOW "(%s default)", aDefaultValue.c_str() );

	if ( aFlags )
	{
		msg += "\n\t" UNIX_CLR_DARK_GREEN;
		// TODO: this could be better probably
		// 3:30 am and very lazy programming here
		for ( size_t i = 0, j = 0; i < gConVarFlags.size(); i++ )
		{
			if ( ( one << i ) > aFlags )
				break;

			if ( !( aFlags & ( one << i ) ) )
				continue;

			if ( j != 0 )
				msg += " | ";

			msg += Con_GetCvarFlagName( ( one << i ) );
			j++;
		}
	}

	if ( aDesc )
		return msg + "\n\t" UNIX_CLR_CYAN + aDesc + UNIX_CLR_DEFAULT "\n\n";

	return msg + UNIX_CLR_DEFAULT "\n";
}


void ConVar::SetValue( std::string_view value )
{
	aValue = value;
	aValueFloat = ToDouble( value.data(), aValueFloat );
}

void ConVar::SetValue( float value )
{
	aValue = ToString( value );
	aValueFloat = value;
}


void ConVar::Reset()
{
	Init( aDefaultValue.c_str(), aFunc );
}


std::string_view ConVar::GetValue()
{
	return aValue;
}

float ConVar::GetFloat()
{
	return aValueFloat;
}

int ConVar::GetInt()
{
	return (int)aValueFloat;
}

bool ConVar::GetBool()
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


void ConVarRef::Init()
{
	// if this is created later, just search for the convar
	SetReference( Con_GetConVar( aName ) );
}

void ConVarRef::SetReference( ConVar* ref )
{
	apRef = ref;
	aValid = apRef != nullptr;
}

// should never be called on a ConVarRef really
std::string ConVarRef::GetPrintMessage(  )
{
	if ( apRef )
		return apRef->GetPrintMessage();

	return vstring( "Invalid ConVarRef: %s\n", aName );
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


CONVAR( con_search_behavior, 0, "0 - must start with this string, 1 - must contain this string" );


static bool ConVarNameCheck( const char* name, const char* search, size_t size )
{
	if ( con_search_behavior == 0.f )
	{
		// Must start with string
#ifdef _WIN32
		if ( _strnicmp( name, search, size ) == 0 )
			return true;
#else
		if ( strncasecmp( name, search, size ) == 0 )
			return true;
#endif
	}
	else
	{
		// Must contain string
		if ( strcasestr( name, search ) )
			return true;
	}

	return false;
}


// remove duplicate user inputs from the history
CONVAR( con_remove_dup_input_history, 1 );


// ================================================================================


void Con_AddToCommandHistory( const std::string &srCmd )
{
	if ( srCmd.empty() )
		return;

	if ( con_remove_dup_input_history )
	{
		vec_remove_if( gCommandHistory, srCmd );
		gCommandHistory.push_back( srCmd );
	}
	else
	{
		if ( gCommandHistory.empty() || gCommandHistory.back() != srCmd )
			gCommandHistory.push_back( srCmd );
	}
}


void Con_CheckInstantCommands( const std::string &srCmd )
{
	for ( size_t i = 0; i < gInstantConVars.size(); i++ )
	{
		if ( gInstantConVars[i]->GetName() == srCmd )
			gInstantConVars[i]->aFunc( {} );
	}
}


void Con_QueueCommand( const std::string &srCmd )
{
	Con_CheckInstantCommands( srCmd );

	gQueue.push_back( srCmd );

	Con_AddToCommandHistory( srCmd );

	Log_Ex( gConsoleChannel, LogType::Input, srCmd.c_str() );
}

void Con_QueueCommandSilent( const std::string &srCmd, bool sAddToHistory )
{
	if ( srCmd.empty() )
		return;

	Con_CheckInstantCommands( srCmd );

	gQueue.push_back( srCmd );

	if ( sAddToHistory )
		Con_AddToCommandHistory( srCmd );
}


// TODO: rethink this function
void Con_RegisterConVars()
{
	PROF_SCOPE();

	// static bool registered = false;

	//if ( registered )
	//	return;

	std::vector< ConVarRef* > cvarRefList;

	gInstantConVars.clear();

	for ( ConVarBase* current : Con_GetConVars() )
	{
		if ( typeid(*current) == typeid(ConVarRef) )
		{
			ConVarRef* cvarRef = static_cast<ConVarRef*>(current);

			if ( cvarRef->apRef == nullptr )
				cvarRefList.push_back( cvarRef );
		}

		if ( current->GetFlags() & CVARF_INSTANT && typeid(*current) == typeid(ConCommand) )
			gInstantConVars.push_back( static_cast<ConCommand*>(current) );
	}

	// Now link all cvar references
	for ( ConVarRef* cvarRef: cvarRefList )
		cvarRef->SetReference( Con_GetConVar( cvarRef->GetName() ) );

	// registered = true;
}


const std::vector< std::string >& Con_GetCommandHistory()
{
	return gCommandHistory;
}


uint32_t Con_GetConVarCount()
{
	return Con_GetConVars().size();
}


ConVarBase* Con_GetConVar( uint32_t sIndex )
{
	if ( sIndex >= Con_GetConVars().size() )
		return nullptr;

	return Con_GetConVars()[ sIndex ];
}


ConVar* Con_GetConVar( std::string_view name )
{
	for ( ConVarBase* cvar : Con_GetConVars() )
	{
		if ( typeid( *cvar ) != typeid( ConVar ) )
			continue;

		if ( cvar->GetName() == name )
			return static_cast< ConVar* >( cvar );
	}

	return nullptr;
}


ConVarBase* Con_GetConVarBase( std::string_view name )
{
	for ( ConVarBase* cvar : Con_GetConVars() )
	{
		if ( cvar->GetName() == name )
			return cvar;
	}

	return nullptr;
}


static std::string g_strEmpty = "";


const std::string& Con_GetConVarValue( std::string_view name )
{
	if ( ConVar* convar = Con_GetConVar( name ) )
		return convar->aValue;
	
	return g_strEmpty;
}


float Con_GetConVarFloat( std::string_view name )
{
	if ( ConVar* convar = Con_GetConVar( name ) )
		return convar->aValueFloat;
	
	return 0.f;
}


void Con_PrintAllConVars()
{
	std::vector< std::string > ConVarMsgs;
	std::vector< std::string > ConCommandMsgs;

	for ( ConVarBase* cvar : Con_GetConVars() )
	{
		if ( typeid(*cvar) == typeid(ConVar)  )
			ConVarMsgs.push_back( cvar->GetPrintMessage() );

		else if ( typeid(*cvar) == typeid(ConCommand) )
			ConCommandMsgs.push_back( cvar->GetPrintMessage() );
	}

	LogGroup group = Log_GroupBegin( gConsoleChannel );
	Log_Group( group, "\nConVars:\n--------------------------------------\n" );
	for ( const auto& msg : ConVarMsgs )
		Log_Group( group, msg.c_str() );

	Log_Group( group, "--------------------------------------\nConCommands:\n--------------------------------------\n" );
	for ( const auto& msg : ConCommandMsgs )
		Log_Group( group, msg.c_str() );

	Log_Group( group, "--------------------------------------\n" );
	Log_GroupEnd( group );
}


inline bool Con_IsConVarRef( ConVarBase* cvar )
{
	return typeid( *cvar ) == typeid( ConVarRef );
}


// ConVarBase* Con_CheckForConVarRef( ConVarBase* cvar )
// {
// 	if ( typeid(*cvar) == typeid(ConVarRef) )
// 	{
// 		ConVarRef* cvarRef = static_cast<ConVarRef*>(cvar);
// 
// 		if ( cvarRef->apRef == nullptr )
// 		{
// 			Log_WarnF( gConsoleChannel, "Found unlinked cvar ref: %s\n", cvarRef->GetName() );
// 			return nullptr;
// 		}
// 
// 		return cvarRef->apRef;
// 	}
// 
// 	return cvar;
// }


void Con_BuildAutoCompleteList( const std::string& srSearch, std::vector< std::string >& srResults )
{
	PROF_SCOPE();

	if ( srSearch.empty() )
		return;

	std::string commandName;
	std::vector< std::string > args;
	Con_ParseCommandLine( srSearch, commandName, args );

	str_lower( commandName );

	for ( ConVarBase* cvar : Con_GetConVars() )
	{
		if ( Con_IsConVarRef( cvar ) )
			continue;

		// this SHOULD be fine, if the convar doesn't exist in here, something is very wrong
		if ( !ConVarNameCheck( cvar->aName, commandName.c_str(), commandName.size() ) )
			continue;

		size_t cvarNameLen = strlen( cvar->aName );
		
		// if ( args.empty() )
		if ( cvarNameLen >= srSearch.size() && commandName.size() >= srSearch.size() )
		{
			srResults.push_back( cvar->aName );
			continue;
		}

		// make sure this actually matches
		if ( cvarNameLen != commandName.size() || !ConVarNameCheck( cvar->aName, commandName.c_str(), cvarNameLen ) )
			continue;

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
					srResults.push_back( vstring( "%s %s", cvar->aName, dropArg.c_str() ) );
				}

				break;
			}
		}

		srResults.push_back( cvar->aName );
		break;
	}
}


void Con_Update()
{
	PROF_SCOPE();

	static bool init = false;

	if ( !init )
	{
		// TODO: rethink this stupid thing
		// also call again if convar count changes
		Con_RegisterConVars();
		init = true;
	}

	// commands could queue new commands to be run next update, so don't try to run those now
	size_t queueSize = gQueue.size();
	for ( size_t i = 0; i < queueSize; i++ )
	{
		Con_RunCommand( gQueue[ 0 ] );
		vec_remove_index( gQueue, 0 );
	}

	// clear the queue, but make sure to not remove any new commands added to the queue from commands just ran
	// for ( size_t i = 0; i < queueSize; i++ )
	// {
	// 	vec_remove_index( gQueue, 0 );
	// }
}


void Con_RunCommand( std::string_view command )
{
	PROF_SCOPE();

	std::string commandName;
	std::vector< std::string > args;

	for ( size_t i = 0; i < command.size(); i++ )
	{
		commandName.clear();
		args.clear();

		Con_ParseCommandLineEx( command, commandName, args, i );
		str_lower( commandName );

		Con_RunCommandArgs( commandName, args );
	}
}


bool Con_RunCommandArgs( const std::string& name, const std::vector< std::string >& args )
{
	PROF_SCOPE();

	bool commandCalled = false;

	for ( ConVarBase* cvar : Con_GetConVars() )
	{
		size_t cvarNameLen = strlen( cvar->aName );

		if ( name.size() != cvarNameLen )
			continue;

#ifdef _WIN32
		if ( _strnicmp( cvar->aName, name.c_str(), cvarNameLen ) != 0 )
			continue;
#else
		if ( strncasecmp( cvar->aName, name.c_str(), cvarNameLen ) != 0 )
			continue;
#endif
		if ( Con_IsConVarRef( cvar ) )
			continue;

		if ( typeid(*cvar) == typeid(ConVar) )
		{
			ConVar* convar = static_cast<ConVar*>(cvar);

			commandCalled = true;

			if ( !args.empty() )
			{
				if ( convar->aFunc )
				{
					std::string prevString = convar->aValue;
					float       prevFloat  = convar->aValueFloat;

					convar->SetValue( args[ 0 ] );

					convar->aFunc( prevString, prevFloat, args );
				}
				else
				{
					convar->SetValue( args[ 0 ] );
				}
			}
			else
			{
				Log_Msg( gConsoleChannel, convar->GetPrintMessage().c_str() );
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

	// command wasn't used?
	if ( !commandCalled )
		Log_WarnF( gConsoleChannel, "Command \"%s\" is undefined\n", name.c_str() );

	return commandCalled;
}


void Con_ParseCommandLine( std::string_view command, std::string& name, std::vector< std::string >& args )
{
	size_t i = 0;
	Con_ParseCommandLineEx( command, name, args, i );
}


void Con_ParseCommandLineEx( std::string_view command, std::string& name, std::vector< std::string >& args, size_t& i )
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

			for ( i++; i < command.size(); i++ )
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


ConVarFlag_t Con_CreateCvarFlag( const char* name )
{
	ConVarFlag_t newBitShift = (1 << gConVarFlags.size());
	size_t len = strlen( name );
	gConVarFlags.emplace_back( newBitShift, name, len );
	return newBitShift;
}

const char* Con_GetCvarFlagName( ConVarFlag_t flag )
{
	for ( auto& data : gConVarFlags )
	{
		if ( data.flag == flag )
			return data.name;
	}

	return nullptr;
}

ConVarFlag_t Con_GetCvarFlag( const char* name )
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


// Add a callback function to add data to the config.cfg file
void Con_AddArchiveCallback( FArchive* sFunc )
{
	gArchiveCallbacks.push_back( sFunc );
}


void Con_Archive( const char* spFile )
{
	std::string output =
	  "// -----------------------------------------------------\n"
	  "// Auto Generated File by Chocolate Engine\n\n";

	// Write all ConVars marked with CVARF_ARCHIVE to this file
	for ( ConVarBase* command : Con_GetConVars() )
	{
		if ( typeid( *command ) != typeid( ConVar ) )
			continue;

		ConVar* cvar = static_cast< ConVar* >( command );

		if ( cvar->aFlags & CVARF_ARCHIVE )
			output += vstring( "%s %s\n", cvar->aName, cvar->GetValue().data() );
	}

	// Run all callback functons on this
	for ( auto& callback : gArchiveCallbacks )
	{
		output += "\n// -----------------------------------------------------\n";
		callback( output );
	}

	std::string filename;
	if ( spFile )
	{
		filename = spFile;
		if ( FileSys_GetFileExt( filename ) != "cfg" )
		{
			filename += ".cfg";
		}
		filename.insert( 0, "cfg/" );
	}
	else
	{
		filename = CON_ARCHIVE_FILE;
	}

	// Write the data
	FILE* fp = fopen( filename.c_str(), "wb" );

	if ( fp == nullptr )
	{
		Log_ErrorF( gConsoleChannel, "Failed to open file handle: \"%s\"\n", filename.c_str() );
		return;
	}

	fwrite( output.c_str(), sizeof( char ), output.size(), fp );
	fclose( fp );

	Log_DevF( gConsoleChannel, 1, "Wrote Config to File: \"%s\"\n", filename.c_str() );
}


// ================================================================================
// Console ConCommands


void exec_dropdown(
	const std::vector< std::string >& args,  // arguments currently typed in by the user
	std::vector< std::string >& results )      // results to populate the dropdown list with
{
	for ( const auto& file : FileSys_ScanDir( "cfg", ReadDir_AllPaths | ReadDir_Recursive ) )
	{
		if ( file.ends_with( ".." ) )
			continue;

		std::string execName = FileSys_GetFileName( file );


		if ( args.size() && !execName.starts_with( args[0] ) )
			continue;

		results.push_back( execName );
	}
}


CONCMD_DROP_VA( exec, exec_dropdown, 0, "Execute a script full of console commands" )
{
	if ( args.size() == 0 )
	{
		Log_Msg( gConsoleChannel, "No Path Specified for exec!\n" );
		return;
	}

	std::string path = "cfg/" + args[0];

	if ( !FileSys_IsFile( path ) )
	{
		if ( !path.ends_with( ".cfg" ) )
			path += ".cfg";
	}

	if ( !FileSys_IsFile( path ) )
	{
		Log_WarnF( gConsoleChannel, "File does not exist: \"%s\"\n", path.c_str() );
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
					Log_Warn( gConsoleChannel, "cfg file trying to exec itself and cause infinite recursion\n" );
				else
					Con_RunCommand( line );

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
		Con_RunCommand( line );

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

	Log_Msg( gConsoleChannel, msg.c_str() );
}


void help_dropdown(
	const std::vector< std::string >& args,  // arguments currently typed in by the user
	std::vector< std::string >& results )      // results to populate the dropdown list with
{
	std::string name = args.empty() ? "" : str_lower2(args[0]);

	for ( ConVarBase* cvar : Con_GetConVars() )
	{
		if ( Con_IsConVarRef( cvar ) )
			continue;

		if ( !ConVarNameCheck( cvar->aName, name.c_str(), name.size() ) )
			continue;

		results.push_back( cvar->GetName() );
	}
}


CONCMD_DROP_VA( help, help_dropdown, 0, "If no args specified, Print all Registered ConVars, otherwise, print information about this convar" )
{
	if ( args.empty() )
	{
		Con_PrintAllConVars();
		Args_PrintRegistered();

		Log_Msg( gConsoleChannel, "--------------------------------------\n" );
		return;
	}

	ConVarBase* cvar = Con_GetConVarBase( args[0] );

	if ( cvar )
	{
		Log_Msg( gConsoleChannel, cvar->GetPrintMessage().c_str() );
	}
	else
	{
		for ( u32 i = 0; i < Args_GetRegisteredCount(); i++ )
		{
			const Arg_t* arg = Args_GetRegisteredData( i );

			for ( u32 n = 0; n < arg->aNames.size(); n++ )
			{
				if ( arg->aNames[ n ] == args[ 0 ] )
				{
					Log_Msg( gConsoleChannel, Args_GetRegisteredPrint( arg ).c_str() );
					return;
				}
			}
		}

		Log_WarnF( gConsoleChannel, "Convar not found: %s\n", args[0].c_str() );
	}
}


void FindStr( bool andSearch, ConVarBase* cvar, const std::vector< std::string >& args, std::vector< std::string >& results )
{
	for ( auto arg : args )
	{
		if ( strstr( cvar->GetName(), arg.c_str() ) )
		{
			results.push_back( cvar->GetPrintMessage() );

			if ( !andSearch )
				return;
		}
	}
}


static void FindStrArg( bool andSearch, const Arg_t* spArg, const std::vector< std::string >& args, std::vector< std::string >& results )
{
	if ( !spArg )
		return;

	for ( auto arg : args )
	{
		for ( u32 n = 0; n < spArg->aNames.size(); n++ )
		{
			if ( strstr( spArg->aNames[ n ], arg.c_str() ) )
			{
				results.push_back( Args_GetRegisteredPrint( spArg ) );

				if ( !andSearch )
					return;
			}
		}
	}
}


void CmdFind( bool andSearch, const std::vector< std::string >& args )
{
	std::vector< std::string > resultsCvar;
	std::vector< std::string > resultsCCmd;
	std::vector< std::string > resultsArgs;

	for ( ConVarBase* cvar : Con_GetConVars() )
	{
		if ( IS_TYPE( *cvar, ConVar ) )
		{
			FindStr( andSearch, cvar, args, resultsCvar );
		}
		else if ( IS_TYPE( *cvar, ConCommand ) )
		{
			FindStr( andSearch, cvar, args, resultsCCmd );
		}
	}

	for ( u32 i = 0; i < Args_GetRegisteredCount(); i++ )
	{
		FindStrArg( andSearch, Args_GetRegisteredData( i ), args, resultsArgs );
	}

	Log_MsgF( gConsoleChannel, "Search Results: %zu\n", resultsCvar.size() + resultsCCmd.size() );

	Log_MsgF( gConsoleChannel,
		"\nConVars: %zu"
		"\n--------------------------------------\n", resultsCvar.size() );

	for ( const auto& msg : resultsCvar )
		Log_Msg( gConsoleChannel, msg.c_str() );

	Log_MsgF( gConsoleChannel,
		"--------------------------------------\n"
		"\nConCommands: %zu"
		"\n--------------------------------------\n", resultsCCmd.size() );

	for ( const auto& msg : resultsCCmd )
		Log_Msg( gConsoleChannel, msg.c_str() );

	Log_MsgF( gConsoleChannel,
		"--------------------------------------\n"
		"\nArguments: %zu"
		"\n--------------------------------------\n", resultsArgs.size() );

	for ( const auto& msg : resultsArgs )
		Log_Msg( gConsoleChannel, msg.c_str() );

	Log_Msg( gConsoleChannel, "--------------------------------------\n" );
}


// IDEA: later when you add in ConVar flags and descriptions, make a findex cmd that you can choose specific parts to search
// like only desc, or only name, or both, and probably the rest of the args for flag searching
// and if search string is empty but flags isn't, just list all for those flags
CONCMD_VA( find, "Search if cvar name contains any of the search arguments" )
{
	if ( args.empty() )
	{
		Log_MsgF( gConsoleChannel, "%s\n", find_cmd.GetDesc() );
		return;
	}

	CmdFind( false, args );
}


CONCMD_VA( findand, "Search if cvar name contains all of the search arguments" )
{
	if ( args.empty() )
	{
		Log_MsgF( gConsoleChannel, "%s\n", findand_cmd.GetDesc() );
		return;
	}

	CmdFind( true, args );
}


void reset_cvar_dropdown(
	const std::vector< std::string >& args,  // arguments currently typed in by the user
	std::vector< std::string >& results )      // results to populate the dropdown list with
{
	std::string name = args.empty() ? "" : str_lower2(args[0]);

	for ( ConVarBase* cvar : Con_GetConVars() )
	{
		if ( Con_IsConVarRef( cvar ) )
			continue;

		if ( IS_NOT_TYPE( *cvar, ConVar ) || !ConVarNameCheck( cvar->aName, name.c_str(), name.size() ) )
			continue;

		results.push_back( cvar->GetName() );
	}
}


CONCMD_DROP_VA( cvar_reset, reset_cvar_dropdown, 0, "reset a convar back to it's default value" )
{
	if ( args.empty() )
	{
		Log_Msg( gConsoleChannel, "No ConVar specified to reset!\n" );
		return;
	}

	ConVar* cvar = Con_GetConVar( args[0] );

	if ( cvar )
	{
		cvar->Reset();
	}
	else
	{
		Log_WarnF( gConsoleChannel, "Convar not found: %s\n", args[ 0 ].c_str() );
	}
}


CONCMD_DROP_VA( cvar_toggle, reset_cvar_dropdown, 0, "toggle a convar between two values" )
{
	if ( args.empty() )
	{
		Log_Msg( gConsoleChannel, "No ConVar specified to reset!\n" );
		return;
	}

	const char* value0 = nullptr;
	const char* value1 = nullptr;

	if ( args.size() >= 3 )
	{
		value0 = args[ 1 ].c_str();
		value1 = args[ 2 ].c_str();
	}
	else if ( args.size() == 2 )
	{
		value0 = args[ 1 ].c_str();
		value1 = "0";  // Default Other Type to toggle between
	}
	else
	{
		// Default Types to toggle between
		value0 = "0";
		value1 = "1";
	}

	ConVar* cvar = Con_GetConVar( args[0] );

	if ( !cvar )
	{
		Log_WarnF( gConsoleChannel, "Convar not found: %s\n", args[ 0 ].c_str() );
		return;
	}
	
	if ( cvar->GetValue() == value1 )
		cvar->SetValue( value0 );
	else
		cvar->SetValue( value1 );
}


// same as in source engine lol
CONCMD_VA( host_writeconfig, "Write a config (can optionally specify a path) containing all ConVars marked with archive, and extra data provided by callback functions" )
{
	if ( args.size() )
		Con_Archive( args[ 0 ].c_str() );
	else
		Con_Archive();
}


CONCMD_VA( con_cvar_stack_size, "Print the stack usage of all convars" )
{
	size_t      size    = 0;

	Log_DevF( gConsoleChannel, 1, "sizeof( ConvarBase ): %zu\n", sizeof( ConVarBase ) );
	Log_DevF( gConsoleChannel, 1, "sizeof( Convar ):     %zu\n", sizeof( ConVar ) );
	Log_DevF( gConsoleChannel, 1, "sizeof( ConCommand ): %zu\n", sizeof( ConCommand ) );
	Log_DevF( gConsoleChannel, 1, "sizeof( ConVarRef ):  %zu\n", sizeof( ConVarRef ) );

	for ( ConVarBase* current : Con_GetConVars() )
	{
		if ( typeid( *current ) == typeid( ConVar ) )
			size += sizeof( ConVar );

		else if ( typeid( *current ) == typeid( ConCommand ) )
			size += sizeof( ConCommand );

		else if ( typeid( *current ) == typeid( ConVarRef ) )
			size += sizeof( ConVarRef );

		else
			size += sizeof( *current );
	}

	Log_DevF( gConsoleChannel, 1, "Convar Stack Size: %zu bytes\n", size );
}


// use if you need to quit the engine right now
CONCMD_VA( _abort, CVARF_INSTANT, "funny" )
{
	abort();
}

