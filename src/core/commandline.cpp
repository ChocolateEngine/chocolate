#include "core/commandline.h"

#include "core/console.h"
#include "core/filesystem.h"
#include "core/log.h"
#include "util.h"

#include <stdarg.h>

extern void              Assert_Init();

static std::string_view* gArgV = nullptr;
static int               gArgC = 0;


extern "C"
{
	void DLL_EXPORT core_init( int argc, char* argv[], const char* workingDir )
	{
		sys_init();

		Args_Init( argc, argv );
		Log_Init();
		FileSys_Init( workingDir );
		Assert_Init();
	}

	void DLL_EXPORT core_exit()
	{
		Con_Archive();
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

