#include "core/commandline.h"

#include "core/console.h"
#include "core/filesystem.h"
#include "core/log.h"
#include "core/app_info.h"
#include "core/util.h"

#include <stdarg.h>

#if CH_USE_MIMALLOC
  #include "mimalloc-new-delete.h"
#endif


// static std::vector< arg_t > g_arg_list;
static ChVector< arg_t > g_arg_list;

extern void              Assert_Init();
extern void              args_shutdown();
extern void              ch_str_free_all();

static ch_string*        gArgV = nullptr;
static int               gArgC = 0;

extern ch_string         gConArchiveFile;
extern ch_string         gConArchiveDefault;


LOG_CHANNEL( Console )


// TODO: Check for name conflicts


extern "C"
{
	int DLL_EXPORT core_init( int argc, char* argv[], const char* desiredWorkingDir )
	{
		args_init( argc, argv );
		Log_Init();

		if ( SDL_Init( SDL_INIT_VIDEO | SDL_INIT_EVENTS | SDL_INIT_AUDIO ) != 0 )
		{
			Log_Fatal( "Unable to initialize SDL2!" );
			return 1;
		}

		sys_init();

		if ( !FileSys_Init( desiredWorkingDir ) )
		{
			Log_Fatal( "Failed to initialize File System!" );
			return 2;
		}

		con_init();
		Assert_Init();
		//Thread_Init();

		// handled by apps now, not required
		// Core_LoadAppInfo();

		args_register_ex( "Execute Scripts on Engine Start", "-exec" );
		return 0;
	}

	void DLL_EXPORT core_exit( bool write_archive )
	{
		// should only do this on safe shutdown
		if ( write_archive )
			Con_Archive();

		// Shutdown All Systems
		Mod_Shutdown();
		Con_Shutdown();

		Core_DestroyAppInfo();
		//Thread_Shutdown();

		FileSys_Shutdown();

		// shutdown sdl
		SDL_Quit();

		args_shutdown();

		Log_Shutdown();

		// Check if all memory is freed
		u32 stringAllocCount = ch_str_get_alloc_count();
		ch_str_free_all();

		sys_shutdown();
	}
}

// call this after the other dlls are loaded, but not initialized yet
// it runs all startup config files, like config.cfg to store saved cvar values
void DLL_EXPORT core_post_load()
{
	if ( FileSys_Exists( CH_STR_UNROLL( gConArchiveFile ) ) )
	{
		const char*    strings[] = { "exec ", gConArchiveFile.data };
		const u64      lengths[] = { 5, gConArchiveFile.size };
		ch_string_auto command   = ch_str_join( 2, strings, lengths );

		Con_QueueCommandSilent( command.data, command.size, false );
	}
	else if ( FileSys_Exists( CH_STR_UNROLL( gConArchiveDefault ) ) )
	{
		const char*    strings[] = { "exec ", gConArchiveDefault.data };
		const u64      lengths[] = { 5, gConArchiveFile.size };
		ch_string_auto command   = ch_str_join( 2, strings, lengths );
		//ch_string_auto command = ch_str_join( 2, { "exec ", gConArchiveDefault.data }, { 5, gConArchiveFile.size } );

		Con_QueueCommandSilent( command.data, command.size, false );
	}

	// if ( FileSys_Exists( "cfg/autoexec.cfg" ) )
	// 	Con_QueueCommandSilent( "exec autoexec", false );

	ch_string      execCfg;
	int            arg = 0;

	while ( args_get_value_next( arg, "-exec", execCfg ) )
	{
		const char*    strings[] = { "exec ", execCfg.data };
		const u64      lengths[] = { 5, execCfg.size };
		ch_string_auto command   = ch_str_join( 2, strings, lengths );
		Con_QueueCommandSilent( command.data, command.size, false );
	}
}


// ================================================================================


static arg_t& args_register_internal( const char* spDesc, const char* spName, EArgType sType )
{
	arg_t& arg = g_arg_list.emplace_back( true );
	arg.apDesc = spDesc;
	arg.aType  = sType;
	arg.aNames.push_back( spName );
	return arg;
}


static arg_t& args_register_internal( const char* spDesc, EArgType sType )
{
	arg_t& arg = g_arg_list.emplace_back( true );
	arg.apDesc = spDesc;
	arg.aType  = sType;
	return arg;
}


static bool args_register_bool( arg_t& srArg, bool sDefault )
{
	srArg.aBool        = sDefault;
	srArg.aDefaultBool = sDefault;

	for ( auto& name : srArg.aNames )
	{
		if ( args_find( name ) )
		{
			srArg.aBool = !sDefault;
			return !sDefault;
		}
	}

	return sDefault;
}


static const char* args_register_string( arg_t& srArg, const char* sDefault )
{
	const char* value = sDefault;

	// Setup so the last argument gets the highest priority
	for ( int i = 0; i < gArgC; i++ )
	{
		for ( auto& name : srArg.aNames )
		{
			if ( ch_str_equals( gArgV[ i ], name ) )
			{
				if ( i + 1 >= gArgC )
				{
					Log_WarnF( gLC_Console, "No Value Specified for Argument \"%s\"", name );
					break;
				}

				value = gArgV[ i + 1 ].data;
				break;
			}
		}
	}

	srArg.apString = value;
	srArg.apDefaultString = sDefault;
	return value;
}


static int args_register_int( arg_t& srArg, int sDefault )
{
	// Setup so the last argument gets the highest priority
	int value = sDefault;
	for ( int i = 0; i < gArgC; i++ )
	{
		for ( auto& name : srArg.aNames )
		{
			if ( ch_str_equals( gArgV[ i ], name ) )
			{
				if ( i + 1 >= gArgC )
				{
					Log_WarnF( gLC_Console, "No Int Value Specified for Argument \"%s\"", name );
					break;
				}

				long out;
				if ( ch_to_long( gArgV[ i + 1 ].data, out ) )
					value = out;
				break;
			}
		}
	}

	srArg.aInt = value;
	srArg.aDefaultInt = sDefault;
	return value;
}


static float args_register_names_float( arg_t& srArg, float sDefault )
{
	float value = sDefault;
	for ( int i = 0; i < gArgC; i++ )
	{
		for ( auto& name : srArg.aNames )
		{
			if ( ch_str_equals( gArgV[ i ], name ) )
			{
				if ( i + 1 >= gArgC )
				{
					Log_WarnF( gLC_Console, "No Int Value Specified for Argument \"%s\"", name );
					break;
				}

				double out;
				if ( ch_to_double( gArgV[ i + 1 ].data, out ) )
					value = out;
				break;
			}
		}
	}

	srArg.aFloat        = value;
	srArg.aDefaultFloat = sDefault;
	return value;
}


// ================================================================================


void args_register_ex( const char* spDesc, const char* spName )
{
	args_register_internal( spDesc, spName, EArgType_Custom );
}

// Default to False
bool args_register( const char* spDesc, const char* spName )
{
	arg_t& arg = args_register_internal( spDesc, spName, EArgType_Bool );
	return args_register_bool( arg, false );
}

bool args_register( bool sDefault, const char* spDesc, const char* spName )
{
	arg_t& arg = args_register_internal( spDesc, spName, EArgType_Bool );
	return args_register_bool( arg, sDefault );
}

const char* args_register( const char* sDefault, const char* spDesc, const char* spName )
{
	arg_t& arg = args_register_internal( spDesc, spName, EArgType_String );
	return args_register_string( arg, sDefault );
}

int args_register( int sDefault, const char* spDesc, const char* spName )
{
	arg_t& arg = args_register_internal( spDesc, spName, EArgType_Int );
	return args_register_int( arg, sDefault );
}

float args_register( float sDefault, const char* spDesc, const char* spName )
{
	arg_t& arg = args_register_internal( spDesc, spName, EArgType_Float );
	return args_register_names_float( arg, sDefault );
}


// ================================================================================


void args_register_ex_names( const char* spDesc, int sCount, const char* spName, ... )
{
	arg_t&  arg = args_register_internal( spDesc, EArgType_Custom );

	va_list args;
	va_start( args, spName );

	arg.aNames.push_back( spName );
	for ( int i = 1; i < sCount; i++ )
		arg.aNames.push_back( va_arg( args, const char* ) );

	va_end( args );
}

// Default to False
bool args_register_names( const char* spDesc, int sCount, const char* spName, ... )
{
	arg_t& arg = args_register_internal( spDesc, EArgType_Bool );

	va_list args;
	va_start( args, spName );

	arg.aNames.push_back( spName );
	for ( int i = 1; i < sCount; i++ )
		arg.aNames.push_back( va_arg( args, const char* ) );

	va_end( args );

	return args_register_bool( arg, false );
}

bool args_register_names( bool sDefault, const char* spDesc, int sCount, const char* spName, ... )
{
	arg_t& arg = args_register_internal( spDesc, EArgType_Bool );

	va_list args;
	va_start( args, spName );

	arg.aNames.push_back( spName );
	for ( int i = 1; i < sCount; i++ )
		arg.aNames.push_back( va_arg( args, const char* ) );

	va_end( args );

	return args_register_bool( arg, sDefault );
}

const char* args_register_names( const char* sDefault, const char* spDesc, int sCount, const char* spName, ... )
{
	arg_t& arg = args_register_internal( spDesc, EArgType_String );

	va_list args;
	va_start( args, spName );

	arg.aNames.push_back( spName );
	for ( int i = 1; i < sCount; i++ )
	{
		arg.aNames.push_back( va_arg( args, const char* ) );
	}

	va_end( args );

	return args_register_string( arg, sDefault );
}

int args_register_names( int sDefault, const char* spDesc, int sCount, const char* spName, ... )
{
	arg_t& arg = args_register_internal( spDesc, EArgType_Int );

	va_list args;
	va_start( args, spName );

	arg.aNames.push_back( spName );
	for ( int i = 1; i < sCount; i++ )
		arg.aNames.push_back( va_arg( args, const char* ) );

	va_end( args );

	return args_register_int( arg, sDefault );
}

float args_register_names( float sDefault, const char* spDesc, int sCount, const char* spName, ... )
{
	arg_t& arg = args_register_internal( spDesc, EArgType_Float );

	va_list args;
	va_start( args, spName );

	arg.aNames.push_back( spName );
	for ( int i = 1; i < sCount; i++ )
		arg.aNames.push_back( va_arg( args, const char* ) );

	va_end( args );

	return args_register_names_float( arg, sDefault );
}


// ================================================================================


u32 args_get_registered_count()
{
	return g_arg_list.size();
}


arg_t* args_get_registered_data( u32 sIndex )
{
	if ( sIndex >= g_arg_list.size() )
	{
		Log_WarnF( gLC_Console, "Argument Index Out of Bounds: %zd size, %d index\n", g_arg_list.size(), sIndex );
		return nullptr;
	}

	return &g_arg_list[ sIndex ];
}


// u32 args_findRegisteredIndex( std::string_view sString )
// {
// 	for ( u32 i = 0; i < g_arg_list.size(); i++ )
// 	{
// 		for ( u32 n = 0; n < g_arg_list[ i ].aNames.size(); n++ )
// 		{
// 			if ( g_arg_list[ i ].aNames[ n ] == sString )
// 				return i;
// 		}
// 	}
// 
// 	return UINT32_MAX;
// }


ch_string args_get_registered_print( const arg_t* spArg )
{
	if ( !spArg )
		return {};

	if ( spArg->aNames.empty() )
	{
		Log_Warn( gLC_Console, "No names for argument!\n" );
		return {};
	}

	ch_string      msg       = ch_str_copy( ANSI_CLR_DEFAULT );
	ch_string_auto join_temp = ch_str_join_space( spArg->aNames.size(), spArg->aNames.apData, " " );

	msg = ch_str_concat( CH_STR_UR( msg ), join_temp.data, join_temp.size );

	switch ( spArg->aType )
	{
		default:
		case EArgType_None:
			Log_WarnF( gLC_Console, "Unknown argument type for arg: %s\n", spArg->aNames[ 0 ] );
			CH_ASSERT_MSG( 0, "Unknown Argument Type" );
			break;

		case EArgType_Custom:
			msg = ch_str_concat( CH_STR_UR( msg ), ANSI_CLR_BLUE " Custom Type", 7 + 12 );
			break;

		case EArgType_Bool:
			// msg = ch_str_join_arr( msg.data, 3, msg.data, ANSI_CLR_BLUE " ", spArg->aBool ? "true" : "false" );
			msg = ch_str_join_list( msg.data, { msg.data, ANSI_CLR_BLUE " ", spArg->aBool ? "true" : "false" } );
			if ( spArg->aBool != spArg->aDefaultBool )
				msg = ch_str_join_arr( msg.data, 4, msg.data, ANSI_CLR_YELLOW " (", spArg->aDefaultBool ? "true" : "false", " default)" );
			break;

		case EArgType_String:
			msg = ch_str_join_arr( msg.data, 3, msg.data, ANSI_CLR_GREEN " ", spArg->apString );
			if ( spArg->apDefaultString && ch_str_equals( spArg->apString, spArg->apDefaultString ) )
				msg = ch_str_join_arr( msg.data, 4, msg.data, ANSI_CLR_YELLOW " (", spArg->apDefaultString, " default)" );
			break;

		case EArgType_Int:
			msg = ch_str_realloc_f( msg.data, "%s" ANSI_CLR_GREEN " %d", msg.data, spArg->aInt );
			if ( spArg->aInt != spArg->aDefaultInt )
				msg = ch_str_realloc_f( msg.data, "%s" ANSI_CLR_YELLOW " (%d default)", msg.data, spArg->aDefaultInt );
			break;

		case EArgType_Float:
			msg = ch_str_realloc_f( msg.data, "%s" ANSI_CLR_GREEN " %.f", msg.data, spArg->aFloat );
			if ( spArg->aFloat != spArg->aDefaultFloat )
				msg = ch_str_realloc_f( msg.data, "%s" ANSI_CLR_YELLOW " (%.f default)", msg.data, spArg->aDefaultFloat );
			break;
	}

	if ( spArg->apDesc )
		msg = ch_str_join_arr( msg.data, 3, msg.data, "\n\t" ANSI_CLR_CYAN, spArg->apDesc );

	msg = ch_str_concat( CH_STR_UR( msg ), "\n", 1 );

	return msg;
}


void args_print_registered()
{
	Log_Msg( gLC_Console, "\nArguments:\n--------------------------------------\n" );
	for ( const arg_t& arg : g_arg_list )
	{
		if ( arg.aNames.empty() )
		{
			Log_Warn( gLC_Console, "No names for argument!\n" );
			continue;
		}

		ch_string_auto msg = args_get_registered_print( &arg );

		if ( !msg.data )
			continue;

		Log_Msg( gLC_Console, msg.data );
	}
}


void args_init( int argc, char* argv[] )
{
	gArgV = new ch_string[ argc ];
	gArgC = argc;

	for ( int i = 0; i < argc; i++ )
	{
		if ( !argv[ i ] )
			break;

		gArgV[ i ].data = argv[ i ];
		gArgV[ i ].size = strlen( argv[ i ] );
	}
}


void args_shutdown()
{
	delete[] gArgV;
	gArgV = nullptr;
	gArgC = 0;
}


int args_get_count()
{
	return gArgC;
}


ch_string* args_get_variables()
{
	return gArgV;
}


bool args_find( const char* search, s32 len )
{
	if ( !ch_str_check_empty( search, len ) )
		return false;

	for ( int i = 0; i < gArgC; i++ )
		if ( ch_str_equals( gArgV[ i ], search, len ) )
			return true;

	return false;
}


#if 0
int Args_GetIndex( std::string_view search )
{
	for ( int i = 0; i < gArgC; i++ )
		if ( gArgV[ i ] == search )
			return i;

	return -1;
}


// ------------------------------------------------------------------------------------


std::string_view Args_GetString( std::string_view search, std::string_view fallback )
{
	int i = Args_GetIndex( search );

	if ( i == -1 || i + 1 > gArgC )
		return fallback;

	return gArgV[ i + 1 ];
}


int Args_GetInt( std::string_view search, int fallback )
{
	int i = Args_GetIndex( search );

	if ( i == -1 || i + 1 > gArgC )
		return fallback;

	long out;
	if ( ch_to_long( gArgV[ i + 1 ].data(), out ) )
		return out;

	return fallback;
}


float Args_GetFloat( std::string_view search, float fallback )
{
	return Args_GetDouble( search, (double)fallback );
}


double Args_GetDouble( std::string_view search, double fallback )
{
	int i = Args_GetIndex( search );

	if ( i == -1 || i + 1 > gArgC )
		return fallback;

	return ToDouble( gArgV[ i + 1 ].data(), fallback );
}
#endif


// ------------------------------------------------------------------------------------


// function to be able to find all values like this
// returns true if it finds a value, false if it fails to
bool args_get_value_next( int& i, const char* search, ch_string& ret )
{
	if ( !search )
		return false;

	size_t searchLen = strlen( search );

	for ( ; i < gArgC; i++ )
	{
		if ( ch_str_equals( gArgV[ i ], search, searchLen ) )
		{
			if ( i == -1 || i + 1 > gArgC )
				return false;

			ret = gArgV[ i + 1 ];
			return true;
		}
	}

	return false;
}

