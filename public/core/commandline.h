/*
commandline.h ( Authored by Demez )

Simple Parser for command line options
*/

#pragma once

#include <string>
#include <vector>

#include "core/platform.h"


// call this after the other dlls are loaded, but not initialized yet
// it runs all startup config files, like config.cfg to store saved cvar values
void CORE_API core_post_load();


// Global Argument Parser
// TODO: maybe try to make this closer to argparse in python and setup arg registering
extern "C"
{
	CORE_API void                                        Args_Init( int argc, char* argv[] );
	CORE_API int                                         Args_GetIndex( const std::string& search );
	CORE_API constexpr const std::vector< std::string >& Args_GetArgs();
	CORE_API size_t                                      Args_GetCount();
	CORE_API bool                                        Args_Find( const std::string& search );

	CORE_API const std::string& Args_GetString( const std::string& search, const std::string& fallback = "" );
	CORE_API int                Args_GetInt( const std::string& search, int fallback );
	CORE_API float              Args_GetFloat( const std::string& search, float fallback );
	CORE_API double             Args_GetDouble( const std::string& search, double fallback );

	// function to be able to find all values like this
	// returns true if it finds a value, false if it fails to
	CORE_API bool               Args_GetValueNext( int& index, const std::string& search, std::string& ret );
};

