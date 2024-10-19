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

LOG_CHANNEL_REGISTER( Console, ELogColor_Gray );

std::vector< std::string >      gQueue;
std::vector< std::string >      gCommandHistory;
std::vector< ConVarFlagData_t > gConVarFlags;
ConVarFlag_t                    gConVarRegisterFlags;

std::vector< FArchive* >        gArchiveCallbacks;

constexpr const char*           CON_ARCHIVE_FILE    = "cfg/config.cfg";
constexpr const char*           CON_ARCHIVE_DEFAULT = "cfg/config_default.cfg";

ch_string                       gConArchiveFile{};
ch_string                       gConArchiveDefault{};

constexpr const char*           CFG_DIR             = "cfg" CH_PATH_SEP_STR;


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


std::unordered_map< std::string_view, ch_string >& Con_GetConVarDesc()
{
	static std::unordered_map< std::string_view, ch_string > convars;
	return convars;
}


void Con_SetConVarRegisterFlags( ConVarFlag_t sFlags )
{
	gConVarRegisterFlags = sFlags;
}


// ================================================================================


void ConCommand::Init( const char* name, const char* desc, ConVarFlag_t flags, ConCommandFunc* func, ConCommandDropdownFunc* dropDownFunc )
{
	this->name           = name;
	this->aDesc          = desc;
	this->aFlags         = flags;

	this->apFunc         = func;
	this->apDropDownFunc = dropDownFunc;

	#pragma message( "TODO: OPTIMIZATION - ConCmds queue all other commands during init, and i imagine that can be a bit slow" );

	ConCommand* cvar = Con_GetConCommand( name );

	if ( cvar )
	{
		Log_WarnF( "Multiple ConCommands with the same name! \"%s\"", name );
		return;
	}

	Con_GetConCommands().push_back( this );
	Con_GetConVarNames().push_back( name );
	Con_GetConVarList().emplace_back( name, false, this );

	if ( gConVarRegisterFlags )
	{
		this->aFlags |= gConVarRegisterFlags;
	}
}


std::string ConCommand::GetPrintMessage()
{
	if ( !aDesc )
		return vstring( ANSI_CLR_DEFAULT "%s\n", name );

	return vstring( ANSI_CLR_DEFAULT "%s\n\t" ANSI_CLR_CYAN "%s" ANSI_CLR_DEFAULT "\n", name, aDesc );
}


ConCommand* Con_GetConCommand( std::string_view name )
{
	for ( ConCommand* cmd : Con_GetConCommands() )
	{
		size_t nameLen = strlen( cmd->name );
		if ( nameLen != name.size() )
			continue;

		if ( ch_strncasecmp( cmd->name, name.data(), name.size() ) == 0 )
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
	std::string_view name;
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
		if ( !ConVarNameCheck( cmd->name, search, size, &startsWith ) )
			continue;

		if ( startsWith )
			resultsStartWith.emplace_back( cmd->name, false, cmd );
		else
			resultsContain.emplace_back( cmd->name, false, cmd );
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
		if ( !ConVarNameCheck( cmd->name, search, size, &startsWith ) )
			continue;

		return ConVarDescriptor_t( cmd->name, false, cmd );
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


// ConVarBase* Con_CheckForConVarRef( ConVarBase* cvar )
// {
// 	if ( typeid(*cvar) == typeid(ConVarRef) )
// 	{
// 		ConVarRef* cvarRef = static_cast<ConVarRef*>(cvar);
// 
// 		if ( cvarRef->apRef == nullptr )
// 		{
// 			Log_WarnF( gLC_Console, "Found unlinked cvar ref: %s\n", cvarRef->GetName() );
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

	std::vector< ConVarDescriptor_t > results;
	Con_SearchConVars( results, commandName.data(), commandName.size() );

	if ( results.empty() )
		return;

	for ( ConVarDescriptor_t& cvarResult : results )
	{
		if ( cvarResult.name.size() >= srSearch.size() && commandName.size() >= srSearch.size() )
		{
			srResults.push_back( cvarResult.name.data() );
			continue;
		}

		// make sure this actually matches
		if ( cvarResult.name.size() != commandName.size() || !ConVarNameCheck( cvarResult.name.data(), commandName.c_str(), cvarResult.name.size(), nullptr ) )
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
					srResults.push_back( vstring( "%s %s", cvarResult.name.data(), dropArg.c_str() ) );
				}

				break;
			}
		}

		srResults.push_back( cvarResult.name.data() );
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
		Con_RunCommand( gQueue[ 0 ].data(), gQueue[ 0 ].size() );
		vec_remove_index( gQueue, 0 );
	}

	// clear the queue, but make sure to not remove any new commands added to the queue from commands just ran
	// for ( size_t i = 0; i < queueSize; i++ )
	// {
	// 	vec_remove_index( gQueue, 0 );
	// }
}


void Con_RunCommand( const char* cmd, int len )
{
	PROF_SCOPE();

	if ( len == -1 )
		len = strlen( cmd );

	if ( len == 0 )
		return;

	std::string commandName;
	std::string fullCommand;
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
		size_t cvarNameLen = strlen( cvar->name );

		if ( name.size() != cvarNameLen )
			continue;

		if ( ch_strncasecmp( cvar->name, name.c_str(), cvarNameLen ) != 0 )
			continue;

		// Check Flag Callbacks
		if ( !Con_CheckFlags( cvar->aFlags, cvar->name, args, fullCommand ) )
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
		Log_WarnF( gLC_Console, "Command \"%s\" is undefined\n", name.c_str() );

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

	ConVarFlag_t newBitShift = (1 << (u64)gConVarFlags.size());
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


void con_init()
{
	gConArchiveFile    = ch_str_copy( CON_ARCHIVE_FILE );
	gConArchiveDefault = ch_str_copy( CON_ARCHIVE_DEFAULT );
}


void Con_Shutdown()
{
	gArchiveCallbacks.clear();

	// Free ConVar Data
	// ChVector< ConCommand* >& concmds = Con_GetConCommands();
	auto&                    cvarMap = Con_GetConVarMap();
	auto&                    descs   = Con_GetConVarDesc();

	// Free ConVar Data
	for ( auto& [ cvarName, cvarData ] : cvarMap )
	{
		switch ( cvarData->aType )
		{
			case EConVarType_Bool:
			{
				free( cvarData->aBool.apData );
				break;
			}

			case EConVarType_Int:
			{
				free( cvarData->aInt.apData );
				break;
			}

			case EConVarType_Float:
			{
				free( cvarData->aFloat.apData );
				break;
			}

			case EConVarType_String:
			{
				ch_str_free( *cvarData->aString.apData );
				ch_str_free( (char*)cvarData->aString.aDefaultData );
				break;
			}

			case EConVarType_RangeInt:
			{
				free( cvarData->aRangeInt.apData );
				break;
			}

			case EConVarType_RangeFloat:
			{
				free( cvarData->aRangeFloat.apData );
				break;
			}

			case EConVarType_Vec2:
			{
				free( cvarData->aVec2.apData );
				break;
			}
			
			case EConVarType_Vec3:
			{
				free( cvarData->aVec3.apData );
				break;
			}

			case EConVarType_Vec4:
			{
				free( cvarData->aVec4.apData );
				break;
			}
		}

		delete cvarData;
	}

	// Free ConVar Descriptions
	for ( auto& [ cvarName, desc ] : descs )
	{
		if ( desc.data )
			ch_str_free( desc.data );
	}
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
		ch_string_auto filenameExt = FileSys_GetFileExt( filename.data(), filename.size() );
		if ( ch_str_equals( filenameExt, "cfg" ) )
		{
			filename += ".cfg";
		}
		filename.insert( 0, "cfg" PATH_SEP_STR );
	}
	else
	{
		filename = gConArchiveFile.data;
	}

	// TODO: have a better way to check for a parent path
	fs::path filenamePath = filename.c_str();
	
	fs::path parentPath   = filenamePath.parent_path();

	if ( !parentPath.empty() )
	{
		const std::string& parentPathStr = parentPath.string();
		FileSys_CreateDirectory( parentPathStr.data() );
	}

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
	// Set the default file

	if ( !spFile )
	{
		ch_string new_data = ch_str_realloc( gConArchiveFile.data, CON_ARCHIVE_FILE );

		if ( !new_data.data )
		{
			Log_Error( gLC_Console, "Failed to allocate memory for default console config file path\n" );
			return;
		}

		gConArchiveFile = new_data;
	}
	else
	{
		if ( ch_str_starts_with( spFile, "cfg/" ) || ch_str_starts_with( spFile, "cfg\\" ) )
		{
			ch_string new_data = ch_str_realloc( gConArchiveFile.data, spFile );

			if ( !new_data.data )
			{
				Log_Error( gLC_Console, "Failed to allocate memory for console config file path\n" );
				return;
			}

			gConArchiveFile = new_data;
		}
		else
		{
			ch_string new_data = ch_str_join_arr( gConArchiveFile.data, 2, CFG_DIR, spFile );

			if ( !new_data.data )
			{
				Log_Error( gLC_Console, "Failed to allocate memory for console config file path\n" );
				return;
			}

			gConArchiveFile = new_data;
		}
	}

	// -----------------------------------------------------
	// Set Default Console Archive File

	if ( !spDefaultFile )
	{
		ch_string new_data = ch_str_realloc( gConArchiveDefault.data, CON_ARCHIVE_DEFAULT );

		if ( !new_data.data )
		{
			Log_Error( gLC_Console, "Failed to allocate memory for default console config file path\n" );
			return;
		}

		gConArchiveDefault = new_data;
	}
	else
	{
		if ( ch_str_starts_with( spDefaultFile, "cfg/" ) || ch_str_starts_with( spDefaultFile, "cfg\\" ) )
		{
			ch_string new_data = ch_str_realloc( gConArchiveDefault.data, spDefaultFile );

			if ( !new_data.data )
			{
				Log_Error( gLC_Console, "Failed to allocate memory for default console config file path\n" );
				return;
			}

			gConArchiveDefault = new_data;
		}
		else
		{
			ch_string new_data = ch_str_join_arr( gConArchiveDefault.data, 2, CFG_DIR, spDefaultFile );

			if ( !new_data.data )
			{
				Log_Error( gLC_Console, "Failed to allocate memory for default console config file path\n" );
				return;
			}

			gConArchiveDefault = new_data;
		}
	}
}


// ================================================================================
// Console ConCommands


void exec_dropdown(
  const std::vector< std::string >& args,         // arguments currently typed in by the user
  const std::string&                fullCommand,  // the full command line the user has typed in
  std::vector< std::string >&       results )           // results to populate the dropdown list with
{
	std::vector< ch_string > files = FileSys_ScanDir( "cfg", ReadDir_AllPaths | ReadDir_Recursive );

	for ( const auto& file : files )
	{
		if ( ch_str_ends_with( file, "..", 2 ) )
			continue;

		ch_string execName = FileSys_GetFileName( file.data, file.size );

		if ( args.size() && !ch_str_starts_with( execName.data, execName.size, args[ 0 ].data(), args[ 0 ].size() ) )
			continue;

		results.emplace_back( execName.data, execName.size );
	}

	ch_str_free( files );
}


CONCMD_DROP_VA( exec, exec_dropdown, 0, "Execute a script full of console commands" )
{
	if ( args.size() == 0 )
	{
		Log_Msg( gLC_Console, "No Path Specified for exec!\n" );
		return;
	}

	ch_string path;

	if ( args[ 0 ].starts_with( "cfg/" )
#ifdef _WIN32
	     || args[ 0 ].starts_with( "cfg\\" )
#endif
	|| FileSys_IsAbsolute( args[ 0 ].c_str() ) )
	{
		path = ch_str_copy( args[ 0 ].data(), args[ 0 ].size() );
	}
	else
	{
		const char* strings[] = { "cfg" CH_PATH_SEP_STR, args[ 0 ].data() };
		const u64   lengths[] = { 4, args[ 0 ].size() };

		path = ch_str_join( 2, strings, lengths );
	}

	if ( !FileSys_IsFile( path.data, path.size ) )
	{
		if ( !ch_str_ends_with( path.data, path.size, ".cfg", 4 ) )
		{
			const char* strings[] = { path.data, ".cfg" };
			const u64   lengths[] = { path.size, 4 };
			ch_string   new_data  = ch_str_join( 2, strings, lengths );

			if ( !new_data.data )
			{
				Log_Error( gLC_Console, "Failed to allocate memory for console config file path\n" );
				ch_str_free( path.data );
				return;
			}

			path = new_data;
		}
	}

	if ( !FileSys_IsFile( path.data, path.size ) )
	{
		Log_WarnF( gLC_Console, "File does not exist: \"%s\"\n", path.data );
		ch_str_free( path.data );
		return;
	}

	std::ifstream fileStream = std::ifstream( path.data, std::ios::in | std::ios::binary | std::ios::ate );

	if ( !fileStream.is_open() )
	{
		Log_ErrorF( gLC_Console, "Failed to open file for exec: \"%s\"\n", path.data );
		ch_str_free( path.data );
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
					Log_Warn( gLC_Console, "cfg file trying to exec itself and cause infinite recursion\n" );
				else
					Con_RunCommand( line.data(), line.size() );

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
		Con_RunCommand( line.data(), line.size() );

	delete[] buf;

	ch_str_free( path.data );
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

	Log_Msg( gLC_Console, msg.c_str() );
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

		Log_Msg( gLC_Console, "--------------------------------------\n" );
		return;
	}

	ConCommand*   cmd  = Con_GetConCommand( args[ 0 ] );
	ConVarData_t* cvar = Con_GetConVarData( args[ 0 ] );  // just to check if it exists

	if ( cmd )
	{
		Log_Msg( gLC_Console, cmd->GetPrintMessage().data() );
	}
	else if ( cvar )
	{
		Log_Msg( gLC_Console, Con_GetConVarHelp( args[ 0 ] ).data() );
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
					Log_Msg( gLC_Console, Args_GetRegisteredPrint( arg ).data );
					return;
				}
			}
		}

		Log_WarnF( gLC_Console, "Convar not found: %s\n", args[0].c_str() );
	}
}


void FindStr( bool andSearch, ConVarDescriptor_t& cvar, const std::vector< std::string >& args, std::vector< std::string >& results )
{
	for ( auto arg : args )
	{
		if ( strstr( cvar.name.data(), arg.c_str() ) )
		{
			if ( cvar.aIsConVar )
				results.push_back( Con_GetConVarHelp( cvar.name ) );
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
				ch_string_auto msg = Args_GetRegisteredPrint( spArg );
				results.emplace_back( msg.data, msg.size );

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

	Log_MsgF( gLC_Console, "Search Results: %zu\n", resultsCvar.size() + resultsCCmd.size() );

	Log_MsgF( gLC_Console,
		"\nConVars: %zu"
		"\n--------------------------------------\n", resultsCvar.size() );

	for ( const auto& msg : resultsCvar )
		Log_Msg( gLC_Console, msg.c_str() );

	Log_MsgF( gLC_Console,
		"--------------------------------------\n"
		"\nConCommands: %zu"
		"\n--------------------------------------\n", resultsCCmd.size() );

	for ( const auto& msg : resultsCCmd )
		Log_Msg( gLC_Console, msg.c_str() );

	Log_MsgF( gLC_Console,
		"--------------------------------------\n"
		"\nArguments: %zu"
		"\n--------------------------------------\n", resultsArgs.size() );

	for ( const auto& msg : resultsArgs )
		Log_Msg( gLC_Console, msg.c_str() );

	Log_Msg( gLC_Console, "--------------------------------------\n" );
}


// IDEA: later when you add in ConVar flags and descriptions, make a findex cmd that you can choose specific parts to search
// like only desc, or only name, or both, and probably the rest of the args for flag searching
// and if search string is empty but flags isn't, just list all for those flags
CONCMD_VA( find, "Search if cvar name contains any of the search arguments" )
{
	if ( args.empty() )
	{
		Log_MsgF( gLC_Console, "%s\n", find_cmd.aDesc );
		return;
	}

	CmdFind( false, args );
}


CONCMD_VA( findand, "Search if cvar name contains all of the search arguments" )
{
	if ( args.empty() )
	{
		Log_MsgF( gLC_Console, "%s\n", findand_cmd.aDesc );
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
		results.push_back( cvar.name.data() );
	}
}


CONCMD_DROP_VA( cvar_reset, reset_cvar_dropdown, 0, "reset a convar back to it's default value" )
{
	if ( args.empty() )
	{
		Log_Msg( gLC_Console, "No ConVar specified to reset!\n" );
		return;
	}

	Con_ResetConVar( args[ 0 ] );
}


CONCMD_DROP_VA( cvar_toggle, reset_cvar_dropdown, 0, "toggle a convar between two values" )
{
	if ( args.empty() )
	{
		Log_Msg( gLC_Console, "No ConVar specified to reset!\n" );
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
		Log_WarnF( gLC_Console, "Convar not found: %s\n", args[ 0 ].c_str() );
		return;
	}

	if ( cvar->aType != EConVarType_Bool )
	{
		Log_WarnF( gLC_Console, "Cannot Toggle Non-Bool type Convar: %s\n", args[ 0 ].c_str() );
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


#if 1
CONCMD_VA( con_cvar_mem_usage, "Print the memory usage usage of all convars" )
{
	Log_MsgF( gLC_Console, "sizeof( ConVarData_t ): %zu\n", sizeof( ConVarData_t ) );
	Log_MsgF( gLC_Console, "sizeof( ConCommand ):   %zu\n", sizeof( ConCommand ) );

	// Calculate the Amount of memory each value take up in the ConVarData_t
	size_t cvarHeapMemory   = 0;
	size_t cvarStringMemory = 0;

	//size_t cmdHeapMemory    = 0;
	//size_t cmdStackMemory   = 0;
	//size_t cmdStringMemory  = 0;

	for ( auto& [ cvarName, cvarData ] : Con_GetConVarMap() )
	{
		cvarHeapMemory   += sizeof( ConVarData_t );
		cvarStringMemory += cvarName.size() * sizeof( std::string_view::value_type );

		// the value is stored on the heap
		switch ( cvarData->aType )
		{
			case EConVarType_Bool:
				cvarHeapMemory += sizeof( bool );  // value + default value
				break;

			case EConVarType_Int:
			case EConVarType_RangeInt:
				cvarHeapMemory += sizeof( int );
				break;

			case EConVarType_Float:
			case EConVarType_RangeFloat:
				cvarHeapMemory += sizeof( float );
				break;

			case EConVarType_String:
				// cvarHeapMemory += sizeof( char* ) * 2;
				cvarStringMemory += strlen( *cvarData->aString.apData ) * sizeof( char );
				cvarStringMemory += cvarData->aString.aDefaultData ? strlen( cvarData->aString.aDefaultData ) * sizeof( char ) : 0;
				break;

			case EConVarType_Vec2:
				cvarHeapMemory += sizeof( glm::vec2 );
				break;

			case EConVarType_Vec3:
				cvarHeapMemory += sizeof( glm::vec3 );
				break;

			case EConVarType_Vec4:
				cvarHeapMemory += sizeof( glm::vec4 );
				break;

			default:
				break;
		}
	}

	Log_MsgF( gLC_Console, "ConVar Count: %zu\n", Con_GetConVarMap().size() );
	Log_MsgF( gLC_Console, "ConVar Memory Usage: %.6f KB\n", Util_BytesToKB( cvarHeapMemory + cvarStringMemory ) );

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
	// Log_DevF( gLC_Console, 1, "INCOMPLETE Convar Memory Usage: %zu bytes\n", size );
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

