#include "core/commandline.h"

#include "core/console.h"
#include "core/filesystem.h"
#include "core/log.h"
#include "core/app_info.h"
#include "util.h"

#include <stdarg.h>

#if CH_USE_MIMALLOC
  #include "mimalloc-new-delete.h"
#endif


// static std::vector< Arg_t > gArgList;
static ChVector< Arg_t > gArgList;

extern void              Assert_Init();
extern void              Args_Shutdown();
extern void              ch_str_free_all();

static ch_string*        gArgV = nullptr;
static int               gArgC = 0;

extern LogChannel        gConsoleChannel;

extern ch_string         gConArchiveFile;
extern ch_string         gConArchiveDefault;

extern "C"
{
	void DLL_EXPORT core_init( int argc, char* argv[], const char* workingDir )
	{
		if ( SDL_Init( SDL_INIT_VIDEO | SDL_INIT_EVENTS | SDL_INIT_AUDIO ) != 0 )
			Log_Fatal( "Unable to initialize SDL2!" );

		Args_Init( argc, argv );
		Log_Init();
		sys_init();
		FileSys_Init( workingDir );
		Assert_Init();
		//Thread_Init();

		Core_LoadAppInfo();

		Args_RegisterEx( "Execute Scripts on Engine Start", "-exec" );
	}

	void DLL_EXPORT core_exit( bool writeArchive )
	{
		// should only do this on safe shutdown
		if ( writeArchive )
			Con_Archive();

		// Shutdown All Systems
		Mod_Shutdown();
		Con_Shutdown();

		Core_DestroyAppInfo();
		//Thread_Shutdown();

		FileSys_Shutdown();

		// shutdown sdl
		SDL_Quit();

		Args_Shutdown();

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
		ch_string_auto command   = ch_str_concat( 2, strings, lengths );

		Con_QueueCommandSilent( command.data, command.size, false );
	}
	else if ( FileSys_Exists( CH_STR_UNROLL( gConArchiveDefault ) ) )
	{
		const char*    strings[] = { "exec ", gConArchiveDefault.data };
		const u64      lengths[] = { 5, gConArchiveFile.size };
		ch_string_auto command   = ch_str_concat( 2, strings, lengths );
		//ch_string_auto command = ch_str_concat( 2, { "exec ", gConArchiveDefault.data }, { 5, gConArchiveFile.size } );

		Con_QueueCommandSilent( command.data, command.size, false );
	}

	// if ( FileSys_Exists( "cfg/autoexec.cfg" ) )
	// 	Con_QueueCommandSilent( "exec autoexec", false );

	ch_string      execCfg;
	int            arg = 0;

	while ( Args_GetValueNext( arg, "-exec", execCfg ) )
	{
		const char*    strings[] = { "exec ", execCfg.data };
		const u64      lengths[] = { 5, execCfg.size };
		ch_string_auto command   = ch_str_concat( 2, strings, lengths );
		Con_QueueCommandSilent( command.data, command.size, false );
	}
}


// ================================================================================


static Arg_t& Args_RegisterInternal( const char* spDesc, const char* spName, EArgType sType )
{
	Arg_t& arg = gArgList.emplace_back( true );
	arg.apDesc = spDesc;
	arg.aType  = sType;
	arg.aNames.push_back( spName );
	return arg;
}


static Arg_t& Args_RegisterInternal( const char* spDesc, EArgType sType )
{
	Arg_t& arg = gArgList.emplace_back( true );
	arg.apDesc = spDesc;
	arg.aType  = sType;
	return arg;
}


static bool Args_RegisterBool( Arg_t& srArg, bool sDefault )
{
	bool value = sDefault;
	for ( auto& name : srArg.aNames )
	{
		value = Args_Find( name );
		if ( value )
			break;
	}

	srArg.aBool        = value;
	srArg.aDefaultBool = sDefault;
	return value;
}


static const char* Args_RegisterString( Arg_t& srArg, const char* sDefault )
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
					Log_WarnF( gConsoleChannel, "No Value Specified for Argument \"%s\"", name );
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


static int Args_RegisterInt( Arg_t& srArg, int sDefault )
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
					Log_WarnF( gConsoleChannel, "No Int Value Specified for Argument \"%s\"", name );
					break;
				}

				long out;
				if ( ToLong3( gArgV[ i + 1 ].data, out ) )
					value = out;
				break;
			}
		}
	}

	srArg.aInt = value;
	srArg.aDefaultInt = sDefault;
	return value;
}


static float Args_RegisterFloat( Arg_t& srArg, float sDefault )
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
					Log_WarnF( gConsoleChannel, "No Int Value Specified for Argument \"%s\"", name );
					break;
				}

				double out;
				if ( ToDouble3( gArgV[ i + 1 ].data, out ) )
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


void Args_RegisterEx( const char* spDesc, const char* spName )
{
	Args_RegisterInternal( spDesc, spName, EArgType_Custom );
}

// Default to False
bool Args_Register( const char* spDesc, const char* spName )
{
	Arg_t& arg = Args_RegisterInternal( spDesc, spName, EArgType_Bool );
	return Args_RegisterBool( arg, false );
}

bool Args_Register( bool sDefault, const char* spDesc, const char* spName )
{
	Arg_t& arg = Args_RegisterInternal( spDesc, spName, EArgType_Bool );
	return Args_RegisterBool( arg, sDefault );
}

const char* Args_Register( const char* sDefault, const char* spDesc, const char* spName )
{
	Arg_t& arg = Args_RegisterInternal( spDesc, spName, EArgType_String );
	return Args_RegisterString( arg, sDefault );
}

int Args_Register( int sDefault, const char* spDesc, const char* spName )
{
	Arg_t& arg = Args_RegisterInternal( spDesc, spName, EArgType_Int );
	return Args_RegisterInt( arg, sDefault );
}

float Args_Register( float sDefault, const char* spDesc, const char* spName )
{
	Arg_t& arg = Args_RegisterInternal( spDesc, spName, EArgType_Float );
	return Args_RegisterFloat( arg, sDefault );
}


// ================================================================================


void Args_RegisterExF( const char* spDesc, int sCount, const char* spName, ... )
{
	Arg_t&  arg = Args_RegisterInternal( spDesc, EArgType_Custom );

	va_list args;
	va_start( args, spName );

	arg.aNames.push_back( spName );
	for ( int i = 1; i < sCount; i++ )
		arg.aNames.push_back( va_arg( args, const char* ) );

	va_end( args );
}

// Default to False
bool Args_RegisterF( const char* spDesc, int sCount, const char* spName, ... )
{
	Arg_t& arg = Args_RegisterInternal( spDesc, EArgType_Bool );

	va_list args;
	va_start( args, spName );

	arg.aNames.push_back( spName );
	for ( int i = 1; i < sCount; i++ )
		arg.aNames.push_back( va_arg( args, const char* ) );

	va_end( args );

	return Args_RegisterBool( arg, false );
}

bool Args_RegisterF( bool sDefault, const char* spDesc, int sCount, const char* spName, ... )
{
	Arg_t& arg = Args_RegisterInternal( spDesc, EArgType_Bool );

	va_list args;
	va_start( args, spName );

	arg.aNames.push_back( spName );
	for ( int i = 1; i < sCount; i++ )
		arg.aNames.push_back( va_arg( args, const char* ) );

	va_end( args );

	return Args_RegisterBool( arg, sDefault );
}

const char* Args_RegisterF( const char* sDefault, const char* spDesc, int sCount, const char* spName, ... )
{
	Arg_t& arg = Args_RegisterInternal( spDesc, EArgType_String );

	va_list args;
	va_start( args, spName );

	arg.aNames.push_back( spName );
	for ( int i = 1; i < sCount; i++ )
	{
		arg.aNames.push_back( va_arg( args, const char* ) );
	}

	va_end( args );

	return Args_RegisterString( arg, sDefault );
}

int Args_RegisterF( int sDefault, const char* spDesc, int sCount, const char* spName, ... )
{
	Arg_t& arg = Args_RegisterInternal( spDesc, EArgType_Int );

	va_list args;
	va_start( args, spName );

	arg.aNames.push_back( spName );
	for ( int i = 1; i < sCount; i++ )
		arg.aNames.push_back( va_arg( args, const char* ) );

	va_end( args );

	return Args_RegisterInt( arg, sDefault );
}

float Args_RegisterF( float sDefault, const char* spDesc, int sCount, const char* spName, ... )
{
	Arg_t& arg = Args_RegisterInternal( spDesc, EArgType_Float );

	va_list args;
	va_start( args, spName );

	arg.aNames.push_back( spName );
	for ( int i = 1; i < sCount; i++ )
		arg.aNames.push_back( va_arg( args, const char* ) );

	va_end( args );

	return Args_RegisterFloat( arg, sDefault );
}


// ================================================================================


u32 Args_GetRegisteredCount()
{
	return gArgList.size();
}


Arg_t* Args_GetRegisteredData( u32 sIndex )
{
	if ( sIndex >= gArgList.size() )
	{
		Log_WarnF( gConsoleChannel, "Argument Index Out of Bounds: %zd size, %d index\n", gArgList.size(), sIndex );
		return nullptr;
	}

	return &gArgList[ sIndex ];
}


// u32 Args_FindRegisteredIndex( std::string_view sString )
// {
// 	for ( u32 i = 0; i < gArgList.size(); i++ )
// 	{
// 		for ( u32 n = 0; n < gArgList[ i ].aNames.size(); n++ )
// 		{
// 			if ( gArgList[ i ].aNames[ n ] == sString )
// 				return i;
// 		}
// 	}
// 
// 	return UINT32_MAX;
// }


ch_string_auto Args_GetRegisteredPrint( const Arg_t* spArg )
{
	if ( !spArg )
		return {};

	if ( spArg->aNames.empty() )
	{
		Log_Warn( gConsoleChannel, "No names for argument!\n" );
		return {};
	}

	ch_string msg = ch_str_copy( ANSI_CLR_DEFAULT );

	for ( u32 i = 0; i < spArg->aNames.size(); i++ )
	{
		msg = ch_str_concat( msg.data, 1, spArg->aNames[ i ] );

		if ( i + 1 != spArg->aNames.size() )
			msg = ch_str_concat( msg.data, 1, " " );
	}

	switch ( spArg->aType )
	{
		default:
		case EArgType_None:
			Log_WarnF( gConsoleChannel, "Unknown argument type for arg: %s\n", spArg->aNames[ 0 ] );
			CH_ASSERT_MSG( 0, "Unknown Argument Type" );
			break;

		case EArgType_Custom:
			msg = ch_str_concat( msg.data, 1, ANSI_CLR_BLUE " Custom Type" );
			break;

		case EArgType_Bool:
			msg = ch_str_concat( msg.data, 3, msg.data, ANSI_CLR_BLUE " ", spArg->aBool ? "true" : "false" );
			if ( spArg->aBool != spArg->aDefaultBool )
				msg = ch_str_concat( msg.data, 4, msg.data, ANSI_CLR_YELLOW " (", spArg->aDefaultBool ? "true" : "false", " default)" );
			break;

		case EArgType_String:
			msg = ch_str_concat( msg.data, 3, msg.data, ANSI_CLR_GREEN " ", spArg->apString );
			if ( spArg->apDefaultString && ch_str_equals( spArg->apString, spArg->apDefaultString ) )
				msg = ch_str_concat( msg.data, 4, msg.data, ANSI_CLR_YELLOW " (", spArg->apDefaultString, " default)" );
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
		msg = ch_str_concat( msg.data, 1, "\n\t" ANSI_CLR_CYAN, spArg->apDesc );

	msg = ch_str_concat( msg.data, 1, "\n" );

	ch_string_auto msg_auto;
	msg_auto.data = msg.data;
	msg_auto.size = msg.size;
	return msg_auto;
}


void Args_PrintRegistered()
{
	ChVector< std::string > args;
	
	Log_Msg( gConsoleChannel, "\nArguments:\n--------------------------------------\n" );
	for ( const Arg_t& arg : gArgList )
	{
		if ( arg.aNames.empty() )
		{
			Log_Warn( gConsoleChannel, "No names for argument!\n" );
			continue;
		}

		ch_string_auto msg = Args_GetRegisteredPrint( &arg );

		if ( !msg.data )
			continue;

		Log_Msg( gConsoleChannel, msg.data );
	}
}


void Args_Init( int argc, char* argv[] )
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


void Args_Shutdown()
{
	delete[] gArgV;
	gArgV = nullptr;
	gArgC = 0;
}


int Args_GetCount()
{
	return gArgC;
}


bool Args_Find( const char* search, s32 len )
{
	if ( !search )
		return false;
	
	if ( len == -1 )
		len = strlen( search );

	if ( len == 0 )
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
	if ( ToLong3( gArgV[ i + 1 ].data(), out ) )
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
bool Args_GetValueNext( int& i, const char* search, ch_string& ret )
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

