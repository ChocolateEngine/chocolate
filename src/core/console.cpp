#include "core/filesystem.h"
#include "core/console.h"
#include "core/profiler.h"
#include "core/platform.h"
#include "core/log.h"
#include "core/build_number.h"
#include "util.h"

#include <cstring>
#include <stdarg.h>
#include <iostream>
#include <fstream>
#include <mutex>

LOG_REGISTER_CHANNEL( Console, LogColor::Gray );


std::vector< std::string >                     gQueue;
std::vector< std::string >                     gCommandHistory;
std::vector< ConVarFlagData_t >                gConVarFlags;
ConVarFlag_t                                   gConVarRegisterFlags;

std::vector< FArchive* >                       gArchiveCallbacks;

constexpr const char*                          CON_ARCHIVE_FILE    = "cfg/config.cfg";
constexpr const char*                          CON_ARCHIVE_DEFAULT = "cfg/config_default.cfg";
std::string                                    gConArchiveFile     = CON_ARCHIVE_FILE;
std::string                                    gConArchiveDefault  = CON_ARCHIVE_DEFAULT;


CONVAR_BOOL( con_remove_dup_input_history, true, "Remove duplicate user inputs from the history" );
CONVAR_INT( con_search_behavior, 1, "0 - must start with this string, 1 - must contain this string" );

// std::unordered_map< ConVarBase*, std::string > gConVarLowercaseNames;


DLL_EXPORT ConVarFlag_t CVARF_NONE = 0;

DLL_EXPORT NEW_CVAR_FLAG( CVARF_ARCHIVE );


// ================================================================================


// convars to register after static initalization
// this is a function to ensure this variable gets initialized first
ChVector< ConCommand* >& Con_GetConCommands()
{
	static ChVector< ConCommand* > convars;
	return convars;
}


std::vector< std::string_view >& Con_GetConVarNames()
{
	static std::vector< std::string_view > convars;
	return convars;
}


std::vector< ConVarDescriptor_t >& Con_GetConVarList()
{
	static std::vector< ConVarDescriptor_t > convars;
	return convars;
}


std::unordered_map< std::string_view, ConVarData_t* >& Con_GetConVarMap()
{
	static std::unordered_map< std::string_view, ConVarData_t* > convars;
	return convars;
}


std::unordered_map< std::string_view, const char* >& Con_GetConVarDesc()
{
	static std::unordered_map< std::string_view, const char* > convars;
	return convars;
}


void Con_SetConVarRegisterFlags( ConVarFlag_t sFlags )
{
	gConVarRegisterFlags = sFlags;
}


// ================================================================================


void ConCommand::Init( const char* name, const char* desc, ConVarFlag_t flags, ConCommandFunc* func, ConCommandDropdownFunc* dropDownFunc )
{
	aName            = name;
	aDesc            = desc;
	aFlags           = flags;

	apFunc           = func;
	apDropDownFunc   = dropDownFunc;

	ConCommand* cvar = Con_GetConCommand( aName );

	if ( cvar )
	{
		Log_WarnF( "Multiple ConCommands with the same name! \"%s\"", aName );
		return;
	}

	Con_GetConCommands().push_back( this );
	Con_GetConVarNames().push_back( aName );
	Con_GetConVarList().emplace_back( aName, false, this );

	if ( gConVarRegisterFlags )
	{
		aFlags |= gConVarRegisterFlags;
	}
}

std::string ConCommand::GetPrintMessage()
{
	if ( !aDesc )
		return vstring( UNIX_CLR_DEFAULT "%s\n", aName );

	return vstring( UNIX_CLR_DEFAULT "%s\n\t" UNIX_CLR_CYAN "%s" UNIX_CLR_DEFAULT "\n", aName, aDesc );
}

const char* ConCommand::GetName()
{
	return aName;
}

const char* ConCommand::GetDesc()
{
	return aDesc;
}

ConVarFlag_t ConCommand::GetFlags()
{
	return aFlags;
}


ConCommand* Con_GetConCommand( std::string_view name )
{
	for ( ConCommand* cmd : Con_GetConCommands() )
	{
		size_t nameLen = strlen( cmd->aName );
		if ( nameLen != name.size() )
			continue;

		if ( ch_strncasecmp( cmd->aName, name.data(), name.size() ) == 0 )
			return cmd;
	}

	return nullptr;
}

// ================================================================================


static bool ConVarNameCheck( const char* name, const char* search, size_t size, bool* spStartsWith )
{
	// Must start with string
	bool startsWith = ( ch_strncasecmp( name, search, size ) == 0 );

	if ( startsWith )
	{
		if ( spStartsWith )
			*spStartsWith = true;

		return true;
	}

	if ( con_search_behavior == 0.f )
		return false;

	// See if it contains the string
	if ( strcasestr( name, search ) )
		return true;

	return false;
}


void Con_SearchConVars( std::vector< std::string >& results, const char* search, size_t size )
{
	std::vector< std::string > resultsStartWith;  // results that the convar name starts with the search
	std::vector< std::string > resultsContain;    // results that contain the string somewhere in in a convar name

	for ( std::string_view cvarName : Con_GetConVarNames() )
	{
		bool startsWith = false;
		if ( !ConVarNameCheck( cvarName.data(), search, size, &startsWith ) )
			continue;

		if ( startsWith )
			resultsStartWith.push_back( cvarName.data() );
		else
			resultsContain.push_back( cvarName.data() );
	}

	if ( resultsStartWith.size() )
	{
		results.insert( results.end(), resultsStartWith.begin(), resultsStartWith.end() );
	}

	if ( resultsContain.size() )
	{
		results.insert( results.end(), resultsContain.begin(), resultsContain.end() );
	}
}


struct ConVarSearchResult_t
{
	std::string_view aName;
	ConVarData_t*    apData;
};


static void Con_SearchConVars( std::vector< ConVarSearchResult_t >& results, const char* search, size_t size )
{
	std::vector< ConVarSearchResult_t > resultsStartWith;  // results that the convar name starts with the search
	std::vector< ConVarSearchResult_t > resultsContain;    // results that contain the string somewhere in in a convar name

	for ( auto& [ cvarName, cvarData ] : Con_GetConVarMap() )
	{
		bool startsWith = false;
		if ( !ConVarNameCheck( cvarName.data(), search, size, &startsWith ) )
			continue;

		if ( startsWith )
			resultsStartWith.emplace_back( cvarName, cvarData );
		else
			resultsContain.emplace_back( cvarName, cvarData );
	}

	if ( resultsStartWith.size() )
	{
		results.insert( results.end(), resultsStartWith.begin(), resultsStartWith.end() );
	}

	if ( resultsContain.size() )
	{
		results.insert( results.end(), resultsContain.begin(), resultsContain.end() );
	}
}


void Con_SearchConVars( std::vector< ConVarDescriptor_t >& results, const char* search, size_t size )
{
	std::vector< ConVarDescriptor_t > resultsStartWith;  // results that the convar name starts with the search
	std::vector< ConVarDescriptor_t > resultsContain;    // results that contain the string somewhere in in a convar name

	for ( auto& [ cvarName, cvarData ] : Con_GetConVarMap() )
	{
		bool startsWith = false;
		if ( !ConVarNameCheck( cvarName.data(), search, size, &startsWith ) )
			continue;

		if ( startsWith )
			resultsStartWith.emplace_back( cvarName, true, cvarData );
		else
			resultsContain.emplace_back( cvarName, true, cvarData );
	}

	for ( ConCommand* cmd : Con_GetConCommands() )
	{
		bool startsWith = false;
		if ( !ConVarNameCheck( cmd->aName, search, size, &startsWith ) )
			continue;

		if ( startsWith )
			resultsStartWith.emplace_back( cmd->aName, false, cmd );
		else
			resultsContain.emplace_back( cmd->aName, false, cmd );
	}

	if ( resultsStartWith.size() )
	{
		results.insert( results.end(), resultsStartWith.begin(), resultsStartWith.end() );
	}

	if ( resultsContain.size() )
	{
		results.insert( results.end(), resultsContain.begin(), resultsContain.end() );
	}
}


ConVarDescriptor_t Con_SearchConVarCmd( const char* search, size_t size )
{
	for ( auto& [ cvarName, cvarData ] : Con_GetConVarMap() )
	{
		bool startsWith = false;
		if ( !ConVarNameCheck( cvarName.data(), search, size, &startsWith ) )
			continue;

		return ConVarDescriptor_t( cvarName, true, cvarData );
	}

	for ( ConCommand* cmd : Con_GetConCommands() )
	{
		bool startsWith = false;
		if ( !ConVarNameCheck( cmd->aName, search, size, &startsWith ) )
			continue;

		return ConVarDescriptor_t( cmd->aName, false, cmd );
	}

	return ConVarDescriptor_t();
}


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


void Con_QueueCommand( const std::string &srCmd )
{
	gQueue.push_back( srCmd );

	Con_AddToCommandHistory( srCmd );

	Log_Ex( gConsoleChannel, LogType::Input, srCmd.c_str() );
}


void Con_QueueCommandSilent( const std::string &srCmd, bool sAddToHistory )
{
	if ( srCmd.empty() )
		return;

	gQueue.push_back( srCmd );

	if ( sAddToHistory )
		Con_AddToCommandHistory( srCmd );
}


const std::vector< std::string >& Con_GetCommandHistory()
{
	return gCommandHistory;
}


u32 Con_GetConVarCount()
{
	return Con_GetConVarMap().size();
}


u32 Con_GetConCommandCount()
{
	return Con_GetConCommands().size();
}


void Con_PrintAllConVars()
{
	std::vector< std::string > conVarMsgs;
	std::vector< std::string > conCommandMsgs;

	for ( auto& [ cvarName, cvarData ] : Con_GetConVarMap() )
	{
		conVarMsgs.push_back( Con_GetConVarHelp( cvarName ) );
	}

	for ( ConCommand* cmd : Con_GetConCommands() )
	{
		conCommandMsgs.push_back( cmd->GetPrintMessage() );
	}

	LogGroup group = Log_GroupBegin( gConsoleChannel );
	Log_Group( group, "\nConVars:\n--------------------------------------\n" );
	for ( const auto& msg : conVarMsgs )
		Log_Group( group, msg.c_str() );

	Log_Group( group, "--------------------------------------\nConCommands:\n--------------------------------------\n" );
	for ( const auto& msg : conCommandMsgs )
		Log_Group( group, msg.c_str() );

	Log_Group( group, "--------------------------------------\n" );
	Log_GroupEnd( group );
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
	std::string fullCommand;
	std::vector< std::string > args;
	Con_ParseCommandLine( srSearch, commandName, args, fullCommand );

	str_lower( commandName );

#if 0
	std::vector< std::string > resultsStartWith;  // results that the convar name starts with the search
	std::vector< std::string > resultsContain;    // results that contain the string somewhere in in a convar name

	for ( ConVarBase* cvar : Con_GetConVars() )
	{
		if ( Con_IsConVarRef( cvar ) )
			continue;

		bool startsWithString = false;

		// this SHOULD be fine, if the convar doesn't exist in here, something is very wrong
		if ( !ConVarNameCheck( cvar->aName, commandName.c_str(), commandName.size(), &startsWithString ) )
			continue;

		size_t cvarNameLen = strlen( cvar->aName );
		
		// if ( args.empty() )
		if ( cvarNameLen >= srSearch.size() && commandName.size() >= srSearch.size() )
		{
			if ( startsWithString )
				resultsStartWith.push_back( cvar->aName );
			else
				resultsContain.push_back( cvar->aName );

			continue;
		}

		// make sure this actually matches
		if ( cvarNameLen != commandName.size() || !ConVarNameCheck( cvar->aName, commandName.c_str(), cvarNameLen, nullptr ) )
			continue;

		// is this a concommand with a drop down function?
		if ( typeid(*cvar) == typeid(ConCommand) )
		{
			auto cmd = static_cast<ConCommand*>(cvar);

			if ( cmd->apDropDownFunc )
			{
				std::vector< std::string > dropDownArgs;
				cmd->apDropDownFunc( args, dropDownArgs );

				for ( auto dropArg: dropDownArgs )
				{
					resultsStartWith.push_back( vstring( "%s %s", cvar->aName, dropArg.c_str() ) );
				}

				break;
			}
		}

		resultsStartWith.push_back( cvar->aName );
		break;
	}

	if ( resultsStartWith.size() )
	{
		srResults.insert( srResults.end(), resultsStartWith.begin(), resultsStartWith.end() );
	}

	if ( resultsContain.size() )
	{
		srResults.insert( srResults.end(), resultsContain.begin(), resultsContain.end() );
	}

#else

	std::vector< ConVarDescriptor_t > results;
	Con_SearchConVars( results, commandName.data(), commandName.size() );

	if ( results.empty() )
		return;

	for ( ConVarDescriptor_t& cvarResult : results )
	{
		if ( cvarResult.aName.size() >= srSearch.size() && commandName.size() >= srSearch.size() )
		{
			srResults.push_back( cvarResult.aName.data() );
			continue;
		}

		// make sure this actually matches
		if ( cvarResult.aName.size() != commandName.size() || !ConVarNameCheck( cvarResult.aName.data(), commandName.c_str(), cvarResult.aName.size(), nullptr ) )
			continue;

		// is this a concommand with a drop down function?
		if ( !cvarResult.aIsConVar )
		{
			ConCommand* cmd = static_cast< ConCommand* >( cvarResult.apData );

			if ( cmd->apDropDownFunc )
			{
				std::vector< std::string > dropDownArgs;
				cmd->apDropDownFunc( args, fullCommand, dropDownArgs );

				for ( auto dropArg: dropDownArgs )
				{
					srResults.push_back( vstring( "%s %s", cvarResult.aName.data(), dropArg.c_str() ) );
				}

				break;
			}
		}

		srResults.push_back( cvarResult.aName.data() );
		break;
	}

#endif
}


void Con_Update()
{
	PROF_SCOPE();

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
	std::string fullCommand;
	std::vector< std::string > args;

	for ( size_t i = 0; i < command.size(); i++ )
	{
		commandName.clear();
		args.clear();

		Con_ParseCommandLineEx( command, commandName, args, fullCommand, i );
		str_lower( commandName );

		Con_RunCommandArgs( commandName, args, fullCommand );
	}
}


// Returns true if we can run the concommand or process the convar
// Returns false if we can't
bool Con_CheckFlags( ConVarFlag_t sFlags, const char* name, const std::vector< std::string >& args, const std::string& fullCommand )
{
	if ( sFlags == 0 )
		return true;

	for ( auto& data : gConVarFlags )
	{
		if ( !( sFlags & data.flag ) )
			continue;

		// Can we execute these arguments on the ConVar/ConCommand?
		if ( data.apCallback && !data.apCallback( name, args, fullCommand ) )
		{
			return false;
		}
	}

	// We can process this command
	return true;
}


bool Con_ProcessConVar( ConVarData_t* cvar, const char* name, const std::vector< std::string >& args, std::string_view fullCommand );


bool Con_RunCommandArgs( const std::string& name, const std::vector< std::string >& args, const std::string& fullCommand )
{
	PROF_SCOPE();

	bool commandCalled = false;

	for ( ConCommand* cvar : Con_GetConCommands() )
	{
		size_t cvarNameLen = strlen( cvar->aName );

		if ( name.size() != cvarNameLen )
			continue;

		if ( ch_strncasecmp( cvar->aName, name.c_str(), cvarNameLen ) != 0 )
			continue;

		// Check Flag Callbacks
		if ( !Con_CheckFlags( cvar->aFlags, cvar->aName, args, fullCommand ) )
			continue;

		cvar->apFunc( args, fullCommand );
		commandCalled = true;
		break;
	}

	if ( !commandCalled )
	{
		// Search ConVars - TODO: STORE THE CVAR NAMES AS LOWER CASE AND HASH SEARCH THEM
		for ( auto& [ cvarName, cvarData ] : Con_GetConVarMap() )
		{
			if ( name.size() != cvarName.size() )
				continue;

			if ( ch_strncasecmp( cvarName.data(), name.c_str(), cvarName.size() ) != 0 )
				continue;

			// Check Flag Callbacks
			if ( !Con_CheckFlags( cvarData->aFlags, name.data(), args, fullCommand ) )
				continue;

			commandCalled = Con_ProcessConVar( cvarData, name.data(), args, fullCommand );
			break;
		}
	}

	// command wasn't used?
	if ( !commandCalled )
		Log_WarnF( gConsoleChannel, "Command \"%s\" is undefined\n", name.c_str() );

	return commandCalled;
}


bool Con_RunCommandArgs( const std::string& name, const std::vector< std::string >& args )
{
	// Join args together as one string with spaces
	std::string fullCommand;

	for ( size_t i = 0; i < args.size(); i++ )
	{
		fullCommand += args[ i ];

		if ( i + 1 < args.size() )
			fullCommand += " ";
	}

	return Con_RunCommandArgs( name, args, fullCommand );
}


void Con_ParseCommandLine( std::string_view command, std::string& name, std::vector< std::string >& args, std::string& fullCommand )
{
	size_t i = 0;
	Con_ParseCommandLineEx( command, name, args, fullCommand, i );
}


void Con_ParseCommandLineEx( std::string_view command, std::string& name, std::vector< std::string >& args, std::string& fullCommand, size_t& i )
{
	PROF_SCOPE();

	std::string curArg;

	for ( ; i < command.size(); i++ )
	{
		if ( name.size() )
		{
			if ( command[ i ] == ' ' || command[ i ] == ';' )
				break;

			name += command[i];
		}
		else if ( command[ i ] == ' ' || command[ i ] == ';' )
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

		fullCommand += ch;

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
	// make sure this exists first
	if ( ConVarFlag_t flag = Con_GetCvarFlag( name ) )
	{
		return flag;
	}

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

size_t Con_GetCvarFlagCount()
{
	return gConVarFlags.size();
}


void Con_SetCvarFlagCallback( ConVarFlag_t sFlag, ConVarFlagChangeFunc* spCallback )
{
	for ( auto& data : gConVarFlags )
	{
		if ( data.flag == sFlag )
			data.apCallback = spCallback;
	}
}


ConVarFlagChangeFunc* Con_GetCvarFlagCallback( ConVarFlag_t sFlag )
{
	for ( auto& data : gConVarFlags )
	{
		if ( data.flag == sFlag )
			return data.apCallback;
	}

	return {};
}


void Con_Shutdown()
{
	gArchiveCallbacks.clear();

	// for ( ConVarBase* current : Con_GetConVars() )
	// {
	// 	if ( typeid( *current ) == typeid( ConVar ) )
	// 	{
	// 		// mark data as nullptr
	// 		ConVar* cvar = static_cast< ConVar* >( current );
	// 		cvar->apData = nullptr;
	// 	}
	// }

	// free convar data
	printf( " *** FREE CONVAR DATA !!!!!!!!\n" );
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
	for ( const auto& [ cvarName, cvarData ] : Con_GetConVarMap() )
	{
		if ( cvarData->aFlags & CVARF_ARCHIVE )
			output += vstring( "%s %s\n", cvarName.data(), Con_GetConVarValueStr( cvarData ).data() );
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
		filename.insert( 0, "cfg" PATH_SEP_STR );
	}
	else
	{
		filename = gConArchiveFile;
	}

	// TODO: have a better way to check for a parent path
	fs::path filenamePath = filename.c_str();
	
	fs::path parentPath   = filenamePath.parent_path();

	if ( !parentPath.empty() )
	{
		FileSys_CreateDirectory( parentPath.string() );
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


// Set Default Console Archive File
void Con_SetDefaultArchive( const char* spFile, const char* spDefaultFile )
{
	if ( !spFile )
	{
		gConArchiveFile = CON_ARCHIVE_FILE;
	}
	else
	{
		gConArchiveFile = spFile;

		if ( !gConArchiveFile.starts_with( "cfg/" )
#ifdef _WIN32
		     && !gConArchiveFile.starts_with( "cfg\\" )
#endif
		)
			gConArchiveFile = "cfg" PATH_SEP_STR + gConArchiveFile;
	}

	if ( !spDefaultFile )
	{
		gConArchiveDefault = CON_ARCHIVE_DEFAULT;
	}
	else
	{
		gConArchiveDefault = spDefaultFile;

		if ( !gConArchiveDefault.starts_with( "cfg/" )
#ifdef _WIN32
		     && !gConArchiveDefault.starts_with( "cfg\\" )
#endif
		)
			gConArchiveDefault = "cfg" PATH_SEP_STR + gConArchiveDefault;
	}


}


// ================================================================================
// Console ConCommands


void exec_dropdown(
  const std::vector< std::string >& args,         // arguments currently typed in by the user
  const std::string&                fullCommand,  // the full command line the user has typed in
  std::vector< std::string >&       results )           // results to populate the dropdown list with
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

	std::string path;

	if ( args[ 0 ].starts_with( "cfg/" )
#ifdef _WIN32
	     || args[ 0 ].starts_with( "cfg\\" )
#endif
	|| FileSys_IsAbsolute( args[ 0 ].c_str() ) )
	{
		path = args[ 0 ];
	}
	else
	{
		path = "cfg" CH_PATH_SEP_STR + args[ 0 ];
	}

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

	if ( !fileStream.is_open() )
	{
		Log_ErrorF( gConsoleChannel, "Failed to open file for exec: \"%s\"\n", path.c_str() );
		return;
	}

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
  const std::vector< std::string >& args,         // arguments currently typed in by the user
  const std::string&                fullCommand,  // the full command line the user has typed in
  std::vector< std::string >&       results )     // results to populate the dropdown list with
{
	std::string name = args.empty() ? "" : str_lower2(args[0]);
	Con_SearchConVars( results, name.data(), name.size() );
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

	ConCommand*   cmd  = Con_GetConCommand( args[ 0 ] );
	ConVarData_t* cvar = Con_GetConVarData( args[ 0 ] );  // just to check if it exists

	if ( cmd )
	{
		Log_Msg( gConsoleChannel, cmd->GetPrintMessage().data() );
	}
	else if ( cvar )
	{
		Log_Msg( gConsoleChannel, Con_GetConVarHelp( args[ 0 ] ).data() );
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


void FindStr( bool andSearch, ConVarDescriptor_t& cvar, const std::vector< std::string >& args, std::vector< std::string >& results )
{
	for ( auto arg : args )
	{
		if ( strstr( cvar.aName.data(), arg.c_str() ) )
		{
			if ( cvar.aIsConVar )
				results.push_back( Con_GetConVarHelp( cvar.aName ) );
			else
				results.push_back( static_cast< ConCommand* >( cvar.apData )->GetPrintMessage() );

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

	for ( ConVarDescriptor_t& cvar : Con_GetConVarList() )
	{
		if ( cvar.aIsConVar )
		{
			FindStr( andSearch, cvar, args, resultsCvar );
		}
		else
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
  const std::vector< std::string >& args,         // arguments currently typed in by the user
  const std::string&                fullCommand,  // the full command line the user has typed in
  std::vector< std::string >&       results )     // results to populate the dropdown list with
{
	std::string name = args.empty() ? "" : str_lower2(args[0]);

	std::vector< ConVarSearchResult_t > searchResults;
	Con_SearchConVars( searchResults, name.data(), name.size() );

	for ( ConVarSearchResult_t& cvar : searchResults )
	{
		results.push_back( cvar.aName.data() );
	}
}


CONCMD_DROP_VA( cvar_reset, reset_cvar_dropdown, 0, "reset a convar back to it's default value" )
{
	if ( args.empty() )
	{
		Log_Msg( gConsoleChannel, "No ConVar specified to reset!\n" );
		return;
	}

	Con_ResetConVar( args[ 0 ] );
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

	// TODO: Support toggling between two values
#if 0
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
#endif

	ConVarData_t* cvar = Con_GetConVarData( args[ 0 ] );

	if ( !cvar )
	{
		Log_WarnF( gConsoleChannel, "Convar not found: %s\n", args[ 0 ].c_str() );
		return;
	}

	if ( cvar->aType != EConVarType_Bool )
	{
		Log_WarnF( gConsoleChannel, "Cannot Toggle Non-Bool type Convar: %s\n", args[ 0 ].c_str() );
		return;
	}
	
	Con_SetConVarValue( args[ 0 ].data(), !*cvar->aBool.apData );
}


// same as in source engine lol
CONCMD_VA( host_writeconfig, "Write a config (can optionally specify a path) containing all ConVars marked with archive, and extra data provided by callback functions" )
{
	if ( args.size() )
		Con_Archive( args[ 0 ].c_str() );
	else
		Con_Archive();
}


#if 0
CONCMD_VA( con_cvar_mem_usage, "Print the memory usage usage of all convars" )
{
	size_t      size    = 0;

	Log_DevF( gConsoleChannel, 1, "sizeof( ConVarData_t ): %zu\n", sizeof( ConVarData_t ) );
	Log_DevF( gConsoleChannel, 1, "sizeof( ConCommand ):   %zu\n", sizeof( ConCommand ) );

	Log_Dev( gConsoleChannel, 1, "todo: finish\n" );

	// Calculate the Amount of memory each value take up in the ConVarData_t

	// for ( ConVarBase* current : Con_GetConVars() )
	// {
	// 	if ( typeid( *current ) == typeid( ConVar ) )
	// 		size += sizeof( ConVar );
	// 
	// 	else if ( typeid( *current ) == typeid( ConCommand ) )
	// 		size += sizeof( ConCommand );
	// 
	// 	else if ( typeid( *current ) == typeid( ConVarRef ) )
	// 		size += sizeof( ConVarRef );
	// 
	// 	else
	// 		size += sizeof( *current );
	// }
	// 
	// Log_DevF( gConsoleChannel, 1, "INCOMPLETE Convar Memory Usage: %zu bytes\n", size );
}
#endif


// use if you need to quit the engine right now
CONCMD_VA( _abort, "funny" )
{
	abort();
}


CONCMD_VA( build_number, "Print Build Number" )
{
	Log_MsgF( "Chocolate Engine Build Number: %zu\n", Core_GetBuildNumber() );
}


CONCMD_VA( cat, "Print a File" )
{
	Log_Msg( "\"cat\" command not implemented\n" );
}


#ifdef _WIN32
CONCMD( heapcheck )
{
	int result = _heapchk();

	switch ( result )
	{
		case _HEAPEMPTY:
			Log_Msg( "Heap Empty\n" );
			break;

		case _HEAPOK:
			Log_Msg( "Heap OK\n" );
			break;

		case _HEAPBADBEGIN:
			Log_Msg( "Heap Bad Begin\n" );
			break;

		case _HEAPBADNODE:
			Log_Msg( "Heap Bad Node\n" );
			break;

		case _HEAPEND:
			Log_Msg( "Heap End\n" );
			break;

		case _HEAPBADPTR:
			Log_Msg( "Heap Bad Pointer\n" );
			break;
	}
}
#endif

