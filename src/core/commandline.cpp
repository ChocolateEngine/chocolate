#include "core/commandline.h"
#include "core/console.h"
#include "core/filesystem.h"
#include "util.h"

#include <stdarg.h>

DLL_EXPORT CommandLine* cmdline = nullptr;


extern "C"
{
	void DLL_EXPORT core_init( int argc, char *argv[], const char* workingDir )
	{
		sys_init();

		cmdline = new CommandLine;
		cmdline->Init( argc, argv );

		filesys = new FileSystem;
		filesys->Init( workingDir );

		console = new Console;
	}
}


// call this after the other dlls are loaded, but not initialized yet
// it runs all startup config files, like config.cfg to store saved cvar values
void DLL_EXPORT core_post_load()
{
	if ( filesys->Exists( "cfg/config.cfg" ) )
		console->QueueCommandSilent( "exec config", false );

	if ( filesys->Exists( "cfg/autoexec.cfg" ) )
		console->QueueCommandSilent( "exec autoexec", false );

	std::string execCfg;
	int arg = 0;

	while ( cmdline->GetValueNext( arg, "-exec", execCfg ) )
	{
		console->QueueCommandSilent( "exec " + execCfg, false );
	}
}


// ================================================================================


void CommandLine::Init( int argc, char *argv[] )
{
	aArgs.resize(argc);
	for (int i = 0; i < argc; i++)
		aArgs[i] = argv[i];
}


constexpr const std::vector<std::string>& CommandLine::GetArgs(  )
{
	return aArgs;
}


size_t CommandLine::GetCount(  )
{
	return aArgs.size();
}


int CommandLine::GetIndex( const std::string& search )
{
	for ( int i = 0; i < aArgs.size(); i++ )
		if ( aArgs[i] == search )
			return i;

	return -1;
}


bool CommandLine::Find( const std::string& search )
{
	for ( auto& arg: aArgs )
		if ( search == arg )
			return true;

	return false;
}


// ------------------------------------------------------------------------------------


const std::string& CommandLine::GetValue( const std::string& search, const std::string& fallback )
{
	int i = GetIndex( search );

	if ( i == -1 || i + 1 > aArgs.size() )
		return fallback;

	return aArgs[i + 1];
}


int CommandLine::GetValue( const std::string& search, int fallback )
{
	int i = GetIndex( search );

	if ( i == -1 || i + 1 > aArgs.size() )
		return fallback;

	return ToLong( aArgs[i + 1].c_str(), fallback );
}


float CommandLine::GetValue( const std::string& search, float fallback )
{
	return GetValue( search, (double)fallback );
}


double CommandLine::GetValue( const std::string& search, double fallback )
{
	int i = GetIndex( search );

	if ( i == -1 || i + 1 > aArgs.size() )
		return fallback;

	return ToDouble( aArgs[i + 1], fallback );
}


// ------------------------------------------------------------------------------------


// function to be able to find all values like this
// returns true if it finds a value, false if it fails to
bool CommandLine::GetValueNext( int& i, const std::string& search, std::string& ret )
{
	for ( ; i < aArgs.size(); i++ )
	{
		if ( aArgs[i] == search )
		{
			if ( i == -1 || i + 1 > aArgs.size() )
				return false;

			ret = aArgs[i + 1];
			return true;
		}
	}

	return false;
}


// ------------------------------------------------------------------------------------


CommandLine::CommandLine(  )
{
}


CommandLine::~CommandLine(  )
{
}

