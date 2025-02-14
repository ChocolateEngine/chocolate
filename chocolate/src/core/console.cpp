#include "core/filesystem.h"
#include "core/console.h"
#include "core/profiler.h"
#include "core/platform.h"
#include "core/log.h"
#include "core/build_number.h"
#include "core/util.h"

#include <cstring>
#include <stdarg.h>
#include <iostream>
#include <fstream>
#include <mutex>

LOG_CHANNEL_REGISTER( Console, ELogColor_Gray );

std::vector< std::string >      gQueue;
std::vector< std::string >      gCommandHistory;

ConVarFlag_t                    gConVarRegisterFlags;
ConVarFlagData_t*               gConVarFlags;
u8                              gConVarFlagCount;

constexpr u8                    CONVAR_FLAG_MAX_COUNT = 64;

std::vector< FArchive* >        gArchiveCallbacks;

constexpr const char*           CON_ARCHIVE_FILE    = "cfg/config.cfg";
constexpr const char*           CON_ARCHIVE_DEFAULT = "cfg/config_default.cfg";

ch_string                       gConArchiveFile{};
ch_string                       gConArchiveDefault{};

constexpr const char*           CFG_DIR             = "cfg" CH_PATH_SEP_STR;

extern u32                      g_cvar_count;
extern ConVarData_t*            g_cvar_data;
extern ch_string*               g_cvar_desc;


CONVAR_BOOL( con_remove_dup_input_history, true, "Remove duplicate user inputs from the history" );
CONVAR_INT( con_search_behavior, 1, "0 - must start with this string, 1 - must contain this string" );

// std::unordered_map< ConVarBase*, std::string > gConVarLowercaseNames;


DLL_EXPORT NEW_CVAR_FLAG( CVARF_ARCHIVE );


// ================================================================================
// ConVar Searching


bool ConVarNameCheck( const char* name, const char* search, size_t size, bool* spStartsWith )
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


void Con_SearchConVars( ChVector< ConVarSearchResult_t >& results, const char* search, size_t size )
{
	ChVector< ConVarSearchResult_t > resultsStartWith;  // results that the convar name starts with the search
	ChVector< ConVarSearchResult_t > resultsContain;    // results that contain the string somewhere in in a convar name

	for ( auto& [ cvarName, cvarIndex ] : Con_GetConVarIndexMap() )
	{
		bool startsWith = false;
		if ( !ConVarNameCheck( cvarName.data, search, size, &startsWith ) )
			continue;

		if ( startsWith )
		{
			ConVarSearchResult_t& cvar_result = resultsStartWith.emplace_back();
			cvar_result.name                  = cvarName;
			cvar_result.index                 = cvarIndex;
		}
		else
		{
			ConVarSearchResult_t& cvar_result = resultsContain.emplace_back();
			cvar_result.name  = cvarName;
			cvar_result.index = cvarIndex;
		}
	}

	if ( resultsStartWith.size() )
	{
		u32 offset = results.size();
		results.resize( results.size() + resultsStartWith.size() );
		memcpy( results.apData + offset, resultsStartWith.apData, resultsStartWith.size_bytes() );
	}

	if ( resultsContain.size() )
	{
		u32 offset = results.size();
		results.resize( results.size() + resultsContain.size() );
		memcpy( results.apData + offset, resultsContain.apData, resultsContain.size_bytes() );
	}
}


// ================================================================================


const std::vector< std::string >& Con_GetCommandHistory()
{
	return gCommandHistory;
}


void Con_PrintAllConVars()
{
	std::vector< std::string > conVarMsgs;
	std::vector< std::string > conCommandMsgs;

	for ( auto& [ cvarName, cvarIndex ] : Con_GetConVarIndexMap() )
	{
		ConVarData_t* cvarData = Con_GetConVarData( cvarIndex );

		if ( cvarData->aType == EConVarType_Command )
			conCommandMsgs.push_back( Con_GetConVarHelp( cvarName.data, cvarIndex ) );
		else
			conVarMsgs.push_back( Con_GetConVarHelp( cvarName.data, cvarIndex ) );
	}

	log_t group = Log_GroupBegin( gLC_Console );
	Log_Group( group, "\nConVars:\n--------------------------------------\n" );
	for ( const auto& msg : conVarMsgs )
		Log_Group( group, msg.c_str() );

	Log_Group( group, "--------------------------------------\nConCommands:\n--------------------------------------\n" );
	for ( const auto& msg : conCommandMsgs )
		Log_Group( group, msg.c_str() );

	Log_Group( group, "--------------------------------------\n" );
	Log_GroupEnd( group );
}


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
		if ( !ConVarNameCheck( cvar->name, commandName.c_str(), commandName.size(), &startsWithString ) )
			continue;

		size_t cvarNameLen = strlen( cvar->name );
		
		// if ( args.empty() )
		if ( cvarNameLen >= srSearch.size() && commandName.size() >= srSearch.size() )
		{
			if ( startsWithString )
				resultsStartWith.push_back( cvar->name );
			else
				resultsContain.push_back( cvar->name );

			continue;
		}

		// make sure this actually matches
		if ( cvarNameLen != commandName.size() || !ConVarNameCheck( cvar->name, commandName.c_str(), cvarNameLen, nullptr ) )
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
					resultsStartWith.push_back( vstring( "%s %s", cvar->name, dropArg.c_str() ) );
				}

				break;
			}
		}

		resultsStartWith.push_back( cvar->name );
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

	ChVector< ConVarSearchResult_t > results;
	Con_SearchConVars( results, commandName.data(), commandName.size() );

	if ( results.empty() )
		return;

	for ( ConVarSearchResult_t& cvarResult : results )
	{
		if ( cvarResult.name.size >= srSearch.size() && commandName.size() >= srSearch.size() )
		{
			srResults.push_back( cvarResult.name.data );
			continue;
		}

		// make sure this actually matches
		if ( cvarResult.name.size != commandName.size() || !ConVarNameCheck( cvarResult.name.data, commandName.c_str(), cvarResult.name.size, nullptr ) )
			continue;

		// is this a concommand with a drop down function?
		ConVarData_t* cvarData = Con_GetConVarData( cvarResult.index );
		if ( cvarData->aType == EConVarType_Command )
		{
			if ( cvarData->aCommand.apDropDownFunc )
			{
				std::vector< std::string > dropDownArgs;
				cvarData->aCommand.apDropDownFunc( args, fullCommand, dropDownArgs );

				for ( auto dropArg: dropDownArgs )
				{
					srResults.push_back( vstring( "%s %s", cvarResult.name.data, dropArg.c_str() ) );
				}

				break;
			}
		}

		srResults.push_back( cvarResult.name.data );
		break;
	}

#endif
}


// =====================================================================================================
// Console Command Running


// Returns true if we can run the concommand or process the convar
// Returns false if we can't
bool Con_CheckFlags( ConVarFlag_t sFlags, const char* name, const std::vector< std::string >& args, const std::string& fullCommand )
{
	if ( sFlags == 0 )
		return true;

	for ( u8 i = 0; i < gConVarFlagCount; i++ )
	{
		if ( !( sFlags & gConVarFlags[ i ].flag ) )
			continue;

		// Can we execute these arguments on the ConVar/ConCommand?
		if ( gConVarFlags[ i ].apCallback && !gConVarFlags[ i ].apCallback( name, args, fullCommand ) )
			return false;
	}

	// We can process this command
	return true;
}


bool Con_ProcessConVar( ConVarData_t* cvar, const char* name, const std::vector< std::string >& args, std::string_view fullCommand );


bool Con_RunCommandArgs( const std::string& name, const std::vector< std::string >& args, const std::string& fullCommand )
{
	PROF_SCOPE();

	bool commandCalled = false;

	// Search ConVars - TODO: STORE THE CVAR NAMES AS LOWER CASE AND HASH SEARCH THEM
	for ( auto& [ cvarName, cvarIndex ] : Con_GetConVarIndexMap() )
	{
		if ( name.size() != cvarName.size )
			continue;

		if ( ch_strncasecmp( cvarName.data, name.c_str(), cvarName.size ) != 0 )
			continue;

		ConVarData_t* cvarData = Con_GetConVarData( cvarIndex );

		if ( !cvarData )
		{
			// funny
			Log_Error( "Not sure how you got here, but congratulations, you found a convar that doesn't point to any data!\n" );
			continue;
		}

		// Check Flag Callbacks
		if ( !Con_CheckFlags( cvarData->aFlags, name.data(), args, fullCommand ) )
			continue;

		commandCalled = Con_ProcessConVar( cvarData, name.data(), args, fullCommand );
		break;
	}

	// command wasn't used?
	if ( !commandCalled )
		Log_WarnF( gLC_Console, "Command \"%s\" is undefined\n", name.c_str() );

	return commandCalled;
}


bool Con_RunCommandArgs( const std::string& name, const std::vector< std::string >& args )
{
	// Join args together as one string with spaces
	std::string fullCommand;
	fullCommand.reserve( 128 );

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
	curArg.reserve( 128 );

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


void Con_AddToCommandHistory( const std::string& srCmd )
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


void Con_QueueCommand( const char* cmd, int len )
{
	if ( len == -1 )
		len = strlen( cmd );

	if ( len == 0 )
		return;

	std::string& cmdStr = gQueue.emplace_back( cmd, len );

	Con_AddToCommandHistory( cmdStr );

	Log_ExF( gLC_Console, ELogType_Raw, "] %s\n", cmd );
}


void Con_QueueCommandSilent( const char* cmd, int len, bool sAddToHistory )
{
	if ( len == -1 )
		len = strlen( cmd );

	if ( len == 0 )
		return;

	std::string& cmdStr = gQueue.emplace_back( cmd, len );

	if ( sAddToHistory )
		Con_AddToCommandHistory( cmdStr );
}


void Con_RunCommand( const char* cmd, int len )
{
	PROF_SCOPE();

	if ( len == -1 )
		len = strlen( cmd );

	if ( len == 0 )
		return;

	std::string                commandName;
	std::string                fullCommand;
	std::vector< std::string > args;

	for ( size_t i = 0; i < len; i++ )
	{
		commandName.clear();
		args.clear();

		Con_ParseCommandLineEx( cmd, commandName, args, fullCommand, i );
		str_lower( commandName );

		Con_RunCommandArgs( commandName, args, fullCommand );
	}
}


// =====================================================================================================
// ConVar Flags


ConVarFlag_t Con_CreateCvarFlag( const char* name )
{
	// make sure this exists first
	if ( ConVarFlag_t flag = Con_GetCvarFlag( name ) )
		return flag;

	if ( CONVAR_FLAG_MAX_COUNT == gConVarFlagCount )
	{
		Log_Error( "Cannot Register New ConVar Flag, at max capacity of 64 Flags\n" );
		return CVARF_NONE;
	}

	if ( !gConVarFlags )
		gConVarFlags = ch_calloc< ConVarFlagData_t >( CONVAR_FLAG_MAX_COUNT );

	ConVarFlag_t newBitShift                 = ( 1Ui64 << (u64)( gConVarFlagCount ) );

	gConVarFlags[ gConVarFlagCount ].name    = name;
	gConVarFlags[ gConVarFlagCount ].nameLen = strlen( name );
	gConVarFlags[ gConVarFlagCount ].flag    = newBitShift;

	gConVarFlagCount++;
	return newBitShift;
}


const char* Con_GetCvarFlagName( ConVarFlag_t flag )
{
	for ( u8 i = 0; i < gConVarFlagCount; i++ )
	{
		if ( gConVarFlags[ i ].flag == flag )
			return gConVarFlags[ i ].name;
	}

	return nullptr;
}


ConVarFlag_t Con_GetCvarFlag( const char* name )
{
	size_t len = strlen( name );

	for ( u8 i = 0; i < gConVarFlagCount; i++ )
	{
		if ( gConVarFlags[ i ].nameLen != len )
			continue;

		if ( strncmp( gConVarFlags[ i ].name, name, len ) == 0 )
			return gConVarFlags[ i ].flag;
	}

	return CVARF_NONE;
}


u8 Con_GetCvarFlagCount()
{
	return gConVarFlagCount;
}


void Con_SetCvarFlagCallback( ConVarFlag_t sFlag, ConVarFlagChangeFunc* spCallback )
{
	for ( u8 i = 0; i < gConVarFlagCount; i++ )
	{
		if ( gConVarFlags[ i ].flag == sFlag )
			gConVarFlags[ i ].apCallback = spCallback;
	}
}


ConVarFlagChangeFunc* Con_GetCvarFlagCallback( ConVarFlag_t sFlag )
{
	for ( u8 i = 0; i < gConVarFlagCount; i++ )
	{
		if ( gConVarFlags[ i ].flag == sFlag )
			return gConVarFlags[ i ].apCallback;
	}

	return {};
}


// =====================================================================================================
// Standard Console Functions


void con_init()
{
	gConArchiveFile    = ch_str_copy( CON_ARCHIVE_FILE );
	gConArchiveDefault = ch_str_copy( CON_ARCHIVE_DEFAULT );
}


void Con_Shutdown()
{
	free( gConVarFlags );

	gArchiveCallbacks.clear();

	// Free ConVar Data
	// ChVector< ConCommand* >& concmds = Con_GetConCommands();
	auto& cvarMap = Con_GetConVarIndexMap();

	// Free ConVar Data
	for ( auto& [ cvarName, cvarIndex ] : cvarMap )
	{
		ConVarData_t& cvarData = g_cvar_data[ cvarIndex ];

		switch ( cvarData.aType )
		{
			case EConVarType_Bool:
			{
				free( cvarData.aBool.apData );
				break;
			}

			case EConVarType_Int:
			{
				free( cvarData.aInt.apData );
				break;
			}

			case EConVarType_Float:
			{
				free( cvarData.aFloat.apData );
				break;
			}

			case EConVarType_String:
			{
				ch_str_free( *cvarData.aString.apData );
				ch_str_free( (char*)cvarData.aString.aDefaultData );
				break;
			}

			case EConVarType_RangeInt:
			{
				free( cvarData.aRangeInt.apData );
				break;
			}

			case EConVarType_RangeFloat:
			{
				free( cvarData.aRangeFloat.apData );
				break;
			}

			case EConVarType_Vec2:
			{
				free( cvarData.aVec2.apData );
				break;
			}
			
			case EConVarType_Vec3:
			{
				free( cvarData.aVec3.apData );
				break;
			}

			case EConVarType_Vec4:
			{
				free( cvarData.aVec4.apData );
				break;
			}
		}

		ch_str_free( cvarName );
	}

	for ( u32 i = 0; i < g_cvar_count; i++ )
	{
		if ( g_cvar_desc[ i ].data )
			ch_str_free( g_cvar_desc[ i ].data );
	}

	free( g_cvar_data );
	free( g_cvar_desc );

	cvarMap.clear();
}


void Con_Update()
{
	PROF_SCOPE();

	// commands could queue new commands to be run next update, so don't try to run those now
	size_t queueSize = gQueue.size();
	for ( size_t i = 0; i < queueSize; i++ )
	{
		Con_RunCommand( gQueue[ 0 ].data(), gQueue[ 0 ].size() );
		vec_remove_index( gQueue, 0 );
	}
}


// =====================================================================================================
// Console Archive File


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
	for ( const auto& [ cvarName, cvarIndex ] : Con_GetConVarIndexMap() )
	{
		ConVarData_t& cvarData = g_cvar_data[ cvarIndex ];

		if ( cvarData.aFlags & CVARF_ARCHIVE )
			output += vstring( "%s %s\n", cvarName.data, Con_GetConVarValueStr( &cvarData ).data() );
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
		ch_string_auto filenameExt = FileSys_GetFileExt( filename.data(), filename.size() );
		if ( ch_str_equals( filenameExt, "cfg", 3 ) )
			filename += ".cfg";

		filename.insert( 0, "cfg" PATH_SEP_STR );
	}
	else
	{
		filename = gConArchiveFile.data;
	}

	// TODO: have a better way to check for a parent path
	char* parentPath = FileSys_GetDirName( filename.data(), filename.size() ).data;

	if ( parentPath )
	{
		if ( !FileSys_Exists( parentPath ) && !FileSys_CreateDirectory( parentPath ) )
		{
			Log_ErrorF( "Failed to create directory: \"%s\"\n", parentPath );
			ch_str_free( parentPath );
			return;
		}
	}

	ch_str_free( parentPath );

	// Write the data
	FILE* fp = fopen( filename.c_str(), "wb" );

	if ( fp == nullptr )
	{
		Log_ErrorF( gLC_Console, "Failed to open file handle: \"%s\"\n", filename.c_str() );
		return;
	}

	fwrite( output.c_str(), sizeof( char ), output.size(), fp );
	fclose( fp );

	Log_DevF( gLC_Console, 1, "Wrote Config to File: \"%s\"\n", filename.c_str() );
}


// Set Default Console Archive File
void Con_SetDefaultArchive( const char* spFile, const char* spDefaultFile )
{
	ch_string file;
	ch_string default_file;

	if ( !spFile )
		file = ch_str_realloc( gConArchiveFile.data, CON_ARCHIVE_FILE );

	else if ( ch_str_starts_with( spFile, "cfg/" ) || ch_str_starts_with( spFile, "cfg\\" ) )
		file = ch_str_realloc( gConArchiveFile.data, spFile );

	else
		file = ch_str_join_arr( gConArchiveFile.data, 2, CFG_DIR, spFile );

	// -----------------------------------------------------
	// Set Default Console Archive File

	if ( !spDefaultFile )
		default_file = ch_str_realloc( gConArchiveDefault.data, CON_ARCHIVE_DEFAULT );

	else if ( ch_str_starts_with( spDefaultFile, "cfg/" ) || ch_str_starts_with( spDefaultFile, "cfg\\" ) )
		default_file = ch_str_realloc( gConArchiveDefault.data, spDefaultFile );

	else
		default_file = ch_str_join_arr( gConArchiveDefault.data, 2, CFG_DIR, spDefaultFile );

	if ( !file.data || !default_file.data )
	{
		Log_Error( gLC_Console, "Failed to allocate memory for console config file path\n" );
		return;
	}

	gConArchiveFile    = file;
	gConArchiveDefault = default_file;
}

