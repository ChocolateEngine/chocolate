#include "core/commandline.h"
#include "core/console.h"
#include "core/filesystem.h"
#include "core/log.h"
#include "util.h"

#include <stdarg.h>

extern void                Assert_Init();
std::vector< std::string > gArgs;

extern "C"
{
	void DLL_EXPORT core_init( int argc, char *argv[], const char* workingDir )
	{
		sys_init();

		Args_Init( argc, argv );
		Log_Init();
		FileSys_Init( workingDir );
		Assert_Init();
	}

	void DLL_EXPORT core_exit()
	{
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

	std::string execCfg;
	int arg = 0;

	while ( Args_GetValueNext( arg, "-exec", execCfg ) )
	{
		Con_QueueCommandSilent( "exec " + execCfg, false );
	}
}


// ================================================================================


void Args_Init( int argc, char *argv[] )
{
	gArgs.resize( argc );
	for ( int i = 0; i < argc; i++ )
		gArgs[ i ] = argv[ i ];
}


constexpr const std::vector< std::string >& Args_GetArgs()
{
	return gArgs;
}


size_t Args_GetCount()
{
	return gArgs.size();
}


int Args_GetIndex( const std::string& search )
{
	for ( int i = 0; i < gArgs.size(); i++ )
		if ( gArgs[i] == search )
			return i;

	return -1;
}


bool Args_Find( const std::string& search )
{
	for ( auto& arg: gArgs )
		if ( search == arg )
			return true;

	return false;
}


// ------------------------------------------------------------------------------------


const std::string& Args_GetString( const std::string& search, const std::string& fallback )
{
	int i = Args_GetIndex( search );

	if ( i == -1 || i + 1 > gArgs.size() )
		return fallback;

	return gArgs[i + 1];
}


int Args_GetInt( const std::string& search, int fallback )
{
	int i = Args_GetIndex( search );

	if ( i == -1 || i + 1 > gArgs.size() )
		return fallback;

	return ToLong( gArgs[i + 1].c_str(), fallback );
}


float Args_GetFloat( const std::string& search, float fallback )
{
	return Args_GetDouble( search, (double)fallback );
}


double Args_GetDouble( const std::string& search, double fallback )
{
	int i = Args_GetIndex( search );

	if ( i == -1 || i + 1 > gArgs.size() )
		return fallback;

	return ToDouble( gArgs[i + 1], fallback );
}


// ------------------------------------------------------------------------------------


// function to be able to find all values like this
// returns true if it finds a value, false if it fails to
bool Args_GetValueNext( int& i, const std::string& search, std::string& ret )
{
	for ( ; i < gArgs.size(); i++ )
	{
		if ( gArgs[i] == search )
		{
			if ( i == -1 || i + 1 > gArgs.size() )
				return false;

			ret = gArgs[i + 1];
			return true;
		}
	}

	return false;
}

