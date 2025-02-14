#include "core/console.h"
#include "core/build_number.h"

#include <iostream>
#include <fstream>

LOG_CHANNEL( Console );


extern bool ConVarNameCheck( const char* name, const char* search, size_t size, bool* spStartsWith );


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

		path                  = ch_str_join( 2, strings, lengths );
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
	fileStream.seekg( 0, fileStream.beg );

	char* buf = new char[ fileLen ];
	fileStream.read( buf, fileLen );
	fileStream.close();

	int         ch        = 0;
	bool        inComment = false;
	std::string line;

	// parse it
	while ( ch < fileLen )
	{
#ifdef _WIN32
		if ( buf[ ch ] == '\r' )
		{
			ch++;
			continue;
		}
		else
#endif
		  if ( buf[ ch ] == '\n' || buf[ ch ] == ';' )
		{
			if ( line != "" )
			{
				if ( line == "exec " + args[ 0 ] )
					Log_Warn( gLC_Console, "cfg file trying to exec itself and cause infinite recursion\n" );
				else
					Con_RunCommand( line.data(), line.size() );

				line = "";
			}

			inComment = false;
		}
		// check for a comment
		else if ( buf[ ch ] == '/' && ch + 1 < fileLen && buf[ ch + 1 ] == '/' )
		{
			inComment = true;
			ch++;
		}
		else if ( !inComment )
		{
			line += buf[ ch ];
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
	auto        p   = args.begin();
	std::string msg = *p++;

	if ( p == args.end() - 1 )
		msg += " ";

	// for (auto p = args.begin(); p != args.end(); p)
	for ( ; p != args.end(); ++p )
	{
		msg += *p;

		if ( p == args.end() - 1 )
			msg += " ";
	}

	msg += "\n";

	Log_Msg( gLC_Console, msg.c_str() );
}


void help_dropdown(
  const std::vector< std::string >& args,         // arguments currently typed in by the user
  const std::string&                fullCommand,  // the full command line the user has typed in
  std::vector< std::string >&       results )           // results to populate the dropdown list with
{
	std::string                name = args.empty() ? "" : str_lower2( args[ 0 ] );
	// Con_SearchConVars( results, name.data(), name.size() );

	std::vector< std::string > resultsStartWith;  // results that the convar name starts with the search
	std::vector< std::string > resultsContain;    // results that contain the string somewhere in in a convar name

	for ( const auto& [ cvarName, cvarIndex ] : Con_GetConVarIndexMap() )
	{
		bool startsWith = false;
		if ( !ConVarNameCheck( cvarName.data, name.data(), name.size(), &startsWith ) )
			continue;

		if ( startsWith )
			resultsStartWith.push_back( cvarName.data );
		else
			resultsContain.push_back( cvarName.data );
	}

	// Also Search Registered Arguments
	for ( u32 i = 0; i < args_get_registered_count(); i++ )
	{
		const arg_t* arg = args_get_registered_data( i );

		for ( u32 n = 0; n < arg->aNames.size(); n++ )
		{
			bool startsWith = false;
			if ( !ConVarNameCheck( arg->aNames[ n ], name.data(), name.size(), &startsWith ) )
				continue;

			if ( startsWith )
				resultsStartWith.push_back( arg->aNames[ n ] );
			else
				resultsContain.push_back( arg->aNames[ n ] );

			break;
		}
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


CONCMD_DROP_VA( help, help_dropdown, 0, "If no args specified, Print all Registered ConVars and Arguments, otherwise, print information about this convar/argument" )
{
	if ( args.empty() )
	{
		Con_PrintAllConVars();
		args_print_registered();

		Log_Msg( gLC_Console, "--------------------------------------\n" );
		return;
	}

	ConVarData_t* cvar = Con_GetConVarData( args[ 0 ].data(), args[ 0 ].size() );  // just to check if it exists

	if ( cvar )
	{
		Log_Msg( gLC_Console, Con_GetConVarHelp( args[ 0 ] ).data() );
	}
	else
	{
		for ( u32 i = 0; i < args_get_registered_count(); i++ )
		{
			const arg_t* arg = args_get_registered_data( i );

			for ( u32 n = 0; n < arg->aNames.size(); n++ )
			{
				if ( arg->aNames[ n ] == args[ 0 ] )
				{
					Log_MsgF( gLC_Console, "Argument: %s\n", args_get_registered_print( arg ).data );
					return;
				}
			}
		}

		Log_WarnF( gLC_Console, "Convar/Argument not found: %s\n", args[ 0 ].c_str() );
	}
}


void FindStr( bool andSearch, const ch_string& name, ConVarData_t* cvar, const std::vector< std::string >& args, std::vector< std::string >& results )
{
	for ( auto arg : args )
	{
		if ( strstr( name.data, arg.c_str() ) )
		{
			results.push_back( Con_GetConVarHelp( name.data ) );

			if ( !andSearch )
				return;
		}
	}
}


static void FindStrArg( bool andSearch, const arg_t* spArg, const std::vector< std::string >& args, std::vector< std::string >& results )
{
	if ( !spArg )
		return;

	for ( auto arg : args )
	{
		for ( u32 n = 0; n < spArg->aNames.size(); n++ )
		{
			if ( strstr( spArg->aNames[ n ], arg.c_str() ) )
			{
				ch_string_auto msg = args_get_registered_print( spArg );
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

	for ( const auto& [ cvarName, cvarIndex ] : Con_GetConVarIndexMap() )
	{
		ConVarData_t* cvarData = Con_GetConVarData( cvarIndex );

		if ( cvarData->aType == EConVarType_Command )
			FindStr( andSearch, cvarName, cvarData, args, resultsCCmd );
		else
			FindStr( andSearch, cvarName, cvarData, args, resultsCvar );
	}

	for ( u32 i = 0; i < args_get_registered_count(); i++ )
	{
		FindStrArg( andSearch, args_get_registered_data( i ), args, resultsArgs );
	}

	Log_MsgF( gLC_Console, "Search Results: %zu\n", resultsCvar.size() + resultsCCmd.size() );

	Log_MsgF( gLC_Console,
	          "\nConVars: %zu"
	          "\n--------------------------------------\n",
	          resultsCvar.size() );

	for ( const auto& msg : resultsCvar )
		Log_Msg( gLC_Console, msg.c_str() );

	Log_MsgF( gLC_Console,
	          "--------------------------------------\n"
	          "\nConCommands: %zu"
	          "\n--------------------------------------\n",
	          resultsCCmd.size() );

	for ( const auto& msg : resultsCCmd )
		Log_Msg( gLC_Console, msg.c_str() );

	Log_MsgF( gLC_Console,
	          "--------------------------------------\n"
	          "\nArguments: %zu"
	          "\n--------------------------------------\n",
	          resultsArgs.size() );

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
		Log_MsgF( gLC_Console, "%s\n", Con_GetConVarHelp( "find" ).c_str() );
		return;
	}

	CmdFind( false, args );
}


CONCMD_VA( findand, "Search if cvar name contains all of the search arguments" )
{
	if ( args.empty() )
	{
		Log_MsgF( gLC_Console, "%s\n", Con_GetConVarHelp( "findand" ).c_str() );
		return;
	}

	CmdFind( true, args );
}


void reset_cvar_dropdown(
  const std::vector< std::string >& args,         // arguments currently typed in by the user
  const std::string&                fullCommand,  // the full command line the user has typed in
  std::vector< std::string >&       results )           // results to populate the dropdown list with
{
	std::string                      name = args.empty() ? "" : str_lower2( args[ 0 ] );

	ChVector< ConVarSearchResult_t > searchResults;
	Con_SearchConVars( searchResults, name.data(), name.size() );

	for ( ConVarSearchResult_t& cvar : searchResults )
		results.push_back( cvar.name.data );
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

	ConVarData_t* cvar = Con_GetConVarData( args[ 0 ].data(), args[ 0 ].size() );

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
CONCMD_NAME_VA( app_config_write, "app.config.write", "Write a config (can optionally specify a path) containing all ConVars marked with archive, and extra data provided by callback functions" )
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

	// Calculate the Amount of memory each value take up in the ConVarData_t
	size_t cvarHeapMemory   = sizeof( ConVarData_t ) * Con_GetConVarCount();
	size_t cvarStringMemory = 0;

	//size_t cmdHeapMemory    = 0;
	//size_t cmdStackMemory   = 0;
	//size_t cmdStringMemory  = 0;

	for ( auto& [ cvarName, cvarIndex ] : Con_GetConVarIndexMap() )
	{
		cvarStringMemory += cvarName.size * sizeof( size_t );

		ConVarData_t* cvarData = Con_GetConVarData( cvarIndex );

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

		// ConVar Descriptions
		const ch_string& cvarDesc = Con_GetConVarDesc( cvarIndex );

		if ( cvarDesc.data )
			cvarStringMemory += cvarDesc.size * sizeof( size_t );
	}

	Log_MsgF( gLC_Console, "ConVar Count: %zu\n", Con_GetConVarCount() );
	Log_MsgF( gLC_Console, "ConVar String Memory Usage: %.6f KB\n", ch_bytes_to_kb( cvarStringMemory ) );
	Log_MsgF( gLC_Console, "ConVar Heap Memory Usage: %.6f KB\n", ch_bytes_to_kb( cvarHeapMemory ) );
	Log_MsgF( gLC_Console, "ConVar Total Memory Usage: %.6f KB\n", ch_bytes_to_kb( cvarHeapMemory + cvarStringMemory ) );
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

