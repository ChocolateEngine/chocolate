#include "core/commandline.h"
#include "core/console.h"
#include "util.h"

#include <stdarg.h>

DLL_EXPORT CommandLine* cmdline = nullptr;


extern "C"
{
	void DLL_EXPORT core_init( int argc, char *argv[] )
	{
		cmdline = new CommandLine;
		cmdline->Init( argc, argv );

		console = new Console;

		if ( FileExists( "cfg/autoexec.cfg" ) )
			console->Add( "exec autoexec" );
		
		std::string execCfg;
		while ( true )
		{
			execCfg = cmdline->GetValue( "-exec" );

			if ( execCfg == "" )
				break;

			console->Add( "exec " + execCfg );
		}
	}
}


// ================================================================================


void CommandLine::Init( int argc, char *argv[] )
{
	aArgs.resize(argc);
	for (int i = 0; i < argc; i++)
		aArgs[i] = argv[i];
}


const std::vector<std::string>& CommandLine::GetAll(  )
{
	return aArgs;
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


CommandLine::CommandLine(  )
{
}


CommandLine::~CommandLine(  )
{
}

