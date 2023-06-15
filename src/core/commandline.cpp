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

static std::string_view* gArgV = nullptr;
static int               gArgC = 0;

extern LogChannel        gConsoleChannel;

extern "C"
{
	void DLL_EXPORT core_init( int argc, char* argv[], const char* workingDir )
	{
		Args_Init( argc, argv );
		Log_Init();
		sys_init();
		FileSys_Init( workingDir );
		Assert_Init();

		Core_LoadAppInfo();

		Args_RegisterEx( "Execute Scripts on Engine Start", "-exec" );
	}

	void DLL_EXPORT core_exit()
	{
		Con_Archive();
		Core_DestroyAppInfo();
		sys_shutdown();
	}
}

// call this after the other dlls are loaded, but not initialized yet
// it runs all startup config files, like config.cfg to store saved cvar values
void DLL_EXPORT core_post_load()
{
	if ( FileSys_Exists( "cfg/config.cfg" ) )
		Con_QueueCommandSilent( "exec config", false );

	if ( FileSys_Exists( "cfg/autoexec.cfg" ) )
		Con_QueueCommandSilent( "exec autoexec", false );

	std::string      exec = "exec ";
	std::string_view execCfg;
	int              arg = 0;

	while ( Args_GetValueNext( arg, "-exec", execCfg ) )
	{
		Con_QueueCommandSilent( exec + execCfg.data(), false );
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
			if ( gArgV[ i ] == name )
			{
				if ( i + 1 >= gArgC )
				{
					Log_WarnF( gConsoleChannel, "No Value Specified for Argument \"%s\"", name );
					break;
				}

				value = gArgV[ i + 1 ].data();
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
			if ( gArgV[ i ] == name )
			{
				if ( i + 1 >= gArgC )
				{
					Log_WarnF( gConsoleChannel, "No Int Value Specified for Argument \"%s\"", name );
					break;
				}

				long out;
				if ( ToLong3( gArgV[ i + 1 ].data(), out ) )
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
			if ( gArgV[ i ] == name )
			{
				if ( i + 1 >= gArgC )
				{
					Log_WarnF( gConsoleChannel, "No Int Value Specified for Argument \"%s\"", name );
					break;
				}

				double out;
				if ( ToDouble3( gArgV[ i + 1 ].data(), out ) )
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
		arg.aNames.push_back( va_arg( args, const char* ) );

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


u32 Args_FindRegisteredIndex( std::string_view sString )
{
	for ( u32 i = 0; i < gArgList.size(); i++ )
	{
		for ( u32 n = 0; n < gArgList[ i ].aNames.size(); n++ )
		{
			if ( gArgList[ i ].aNames[ n ] == sString )
				return i;
		}
	}

	return UINT32_MAX;
}


std::string Args_GetRegisteredPrint( const Arg_t* spArg )
{
	if ( !spArg )
		return "";

	if ( spArg->aNames.empty() )
	{
		Log_Warn( gConsoleChannel, "No names for argument!\n" );
		return "";
	}

	std::string msg = UNIX_CLR_DEFAULT;

	for ( u32 i = 0; i < spArg->aNames.size(); i++ )
	{
		msg += spArg->aNames[ i ];

		if ( i + 1 != spArg->aNames.size() )
			msg += " ";
	}

	switch ( spArg->aType )
	{
		default:
		case EArgType_None:
			Log_WarnF( gConsoleChannel, "Unknown argument type for arg: %s\n", spArg->aNames[ 0 ] );
			AssertMsg( 0, "Unknown Argument Type" );
			break;

		case EArgType_Custom:
			msg += UNIX_CLR_BLUE " Custom Type";
			break;

		case EArgType_Bool:
			msg += vstring( UNIX_CLR_BLUE " %s", spArg->aBool ? "true" : "false" );
			if ( spArg->aBool != spArg->aDefaultBool )
				msg += vstring( UNIX_CLR_YELLOW " (%s default)", spArg->aDefaultBool ? "true" : "false" );
			break;

		case EArgType_String:
			msg += vstring( UNIX_CLR_GREEN " %s", spArg->apString );
			if ( strcmp( spArg->apString, spArg->apDefaultString ) != 0 )
				msg += vstring( UNIX_CLR_YELLOW " (%s default)", spArg->apDefaultString );
			break;

		case EArgType_Int:
			msg += vstring( UNIX_CLR_GREEN " %d", spArg->aInt );
			if ( spArg->aInt != spArg->aDefaultInt )
				msg += vstring( UNIX_CLR_YELLOW " (%d default)", spArg->aDefaultInt );
			break;

		case EArgType_Float:
			msg += vstring( UNIX_CLR_GREEN " %.f", spArg->aFloat );
			if ( spArg->aFloat != spArg->aDefaultFloat )
				msg += vstring( UNIX_CLR_YELLOW " (%d default)", spArg->aDefaultFloat );
			break;
	}

	if ( spArg->apDesc )
		msg += vstring( "\n\t" UNIX_CLR_CYAN "%s", spArg->apDesc );

	return msg + "\n";
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

		std::string msg = Args_GetRegisteredPrint( &arg );

		if ( msg.empty() )
			continue;

		Log_Msg( gConsoleChannel, msg.c_str() );
	}
}


void Args_Init( int argc, char* argv[] )
{
	gArgV = new std::string_view[ argc ];
	gArgC = argc;

	for ( int i = 0; i < argc; i++ )
		gArgV[ i ] = argv[ i ];
}


int Args_GetCount()
{
	return gArgC;
}


int Args_GetIndex( std::string_view search )
{
	for ( int i = 0; i < gArgC; i++ )
		if ( gArgV[ i ] == search )
			return i;

	return -1;
}


bool Args_Find( std::string_view search )
{
	for ( int i = 0; i < gArgC; i++ )
		if ( gArgV[ i ] == search )
			return true;

	return false;
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


// ------------------------------------------------------------------------------------


// function to be able to find all values like this
// returns true if it finds a value, false if it fails to
bool Args_GetValueNext( int& i, std::string_view search, std::string_view& ret )
{
	for ( ; i < gArgC; i++ )
	{
		if ( gArgV[ i ] == search )
		{
			if ( i == -1 || i + 1 > gArgC )
				return false;

			ret = gArgV[ i + 1 ];
			return true;
		}
	}

	return false;
}

