/*
commandline.h ( Authored by Demez )

Simple Parser for command line options
*/

#pragma once

#include <string>

#include "core/platform.h"
#include "core/vector.hpp"


enum EArgType
{
	EArgType_None,
	EArgType_Custom,
	EArgType_Bool,
	EArgType_String,
	EArgType_Int,
	EArgType_Float,
	// EArgType_List,
};


struct Arg_t
{
	ChVector< const char* > aNames;
	const char*             apDesc;
	EArgType                aType;

	union
	{
		bool        aBool;
		const char* apString;
		int         aInt;
		float       aFloat;
	};

	union
	{
		bool        aDefaultBool;
		const char* apDefaultString;
		int         aDefaultInt;
		float       aDefaultFloat;
	};
};


// call this after the other dlls are loaded, but not initialized yet
// it runs all startup config files, like config.cfg to store saved cvar values
void CORE_API core_post_load();


// ==================================================================================================
// New Argument Registering, allows you to search for arguments and add descriptions to them

// This one allows you to register one so it still shows in the console, but parse it your own way
// Maybe in the future, add one with a function callback so you can
CORE_API void        Args_RegisterEx( const char* spDesc, const char* spName );
CORE_API void        Args_RegisterExF( const char* spDesc, int sCount, const char* spName, ... );

CORE_API bool        Args_Register( const char* spDesc, const char* spName );  // Default to false
CORE_API bool        Args_Register( bool sDefault, const char* spDesc, const char* spName );
CORE_API const char* Args_Register( const char* sDefault, const char* spDesc, const char* spName );
CORE_API int         Args_Register( int sDefault, const char* spDesc, const char* spName );
CORE_API float       Args_Register( float sDefault, const char* spDesc, const char* spName );

// why is this called F? it's not a format string...
// it's just a way to register multiple names at once
CORE_API bool        Args_RegisterF( const char* spDesc, int sCount, const char* spName, ... );  // Default to false
CORE_API bool        Args_RegisterF( bool sDefault, const char* spDesc, int sCount, const char* spName, ... );
CORE_API const char* Args_RegisterF( const char* sDefault, const char* spDesc, int sCount, const char* spName, ... );
CORE_API int         Args_RegisterF( int sDefault, const char* spDesc, int sCount, const char* spName, ... );
CORE_API float       Args_RegisterF( float sDefault, const char* spDesc, int sCount, const char* spName, ... );

#define CH_ARG_REGISTER_BOOL( varName, defaultValue, desc, name ) bool varName = Args_Register( defaultValue, desc, name )

CORE_API u32         Args_GetRegisteredCount();
CORE_API Arg_t*      Args_GetRegisteredData( u32 sIndex );
CORE_API ch_string_auto Args_GetRegisteredPrint( const Arg_t* spArg );

// CORE_API std::string Args_GetRegisteredPrint( int sIndex );
// CORE_API std::string Args_GetRegisteredPrint( std::string_view sString );

// Global Argument Parser
// TODO: maybe try to make this closer to argparse in python and setup arg registering
extern "C"
{
	CORE_API void             Args_Init( int argc, char* argv[] );
	CORE_API int              Args_GetCount();

	CORE_API void             Args_PrintRegistered();

	// Classic Argument Parsing
	//CORE_API int              Args_GetIndex( std::string_view search );
	CORE_API bool             Args_Find( const char* search, s32 len = -1 );

	//CORE_API std::string_view Args_GetString( std::string_view search, std::string_view fallback = "" );
	//CORE_API int              Args_GetInt( std::string_view search, int fallback );
	//CORE_API float            Args_GetFloat( std::string_view search, float fallback );
	//CORE_API double           Args_GetDouble( std::string_view search, double fallback );

	// function to be able to find all values like this
	// returns true if it finds a value, false if it fails to
	CORE_API bool             Args_GetValueNext( int& index, const char* search, ch_string& ret );
};

