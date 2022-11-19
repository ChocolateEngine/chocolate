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
	CORE_API void             Args_Init( int argc, char* argv[] );
	CORE_API int              Args_GetIndex( std::string_view search );
	CORE_API int              Args_GetCount();
	CORE_API bool             Args_Find( std::string_view search );

	CORE_API std::string_view Args_GetString( std::string_view search, std::string_view fallback = "" );
	CORE_API int              Args_GetInt( std::string_view search, int fallback );
	CORE_API float            Args_GetFloat( std::string_view search, float fallback );
	CORE_API double           Args_GetDouble( std::string_view search, double fallback );

	// function to be able to find all values like this
	// returns true if it finds a value, false if it fails to
	CORE_API bool             Args_GetValueNext( int& index, std::string_view search, std::string_view& ret );
};

