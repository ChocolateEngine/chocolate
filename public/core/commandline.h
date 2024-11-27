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


struct arg_t
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
// You can't use any of these functions globally in core dll, as we don't have arguments saved yet
// So in ch_core.dll, use these only in functions, but you can use them globally anywhere else

// This one allows you to register one so it still shows in the console, but parse it your own way
// Maybe in the future, add one with a function callback so you can
CORE_API void        args_register_ex( const char* spDesc, const char* spName );
CORE_API void        args_register_ex_names( const char* spDesc, int sCount, const char* spName, ... );

CORE_API bool        args_register( const char* spDesc, const char* spName );  // Default to false
CORE_API bool        args_register( bool sDefault, const char* spDesc, const char* spName );
CORE_API const char* args_register( const char* sDefault, const char* spDesc, const char* spName );
CORE_API int         args_register( int sDefault, const char* spDesc, const char* spName );
CORE_API float       args_register( float sDefault, const char* spDesc, const char* spName );

// these functions allow you to have an argument go by many names, like the argument --width and -w
CORE_API bool        args_register_names( const char* spDesc, int sCount, const char* spName, ... );  // Default to false
CORE_API bool        args_register_names( bool sDefault, const char* spDesc, int sCount, const char* spName, ... );
CORE_API const char* args_register_names( const char* sDefault, const char* spDesc, int sCount, const char* spName, ... );
CORE_API int         args_register_names( int sDefault, const char* spDesc, int sCount, const char* spName, ... );
CORE_API float       args_register_names( float sDefault, const char* spDesc, int sCount, const char* spName, ... );

CORE_API u32         args_get_registered_count();
CORE_API arg_t*      args_get_registered_data( u32 sIndex );
CORE_API ch_string   args_get_registered_print( const arg_t* spArg );
CORE_API void        args_print_registered();

// CORE_API std::string args_get_registered_print( int sIndex );
// CORE_API std::string args_get_registered_print( std::string_view sString );

// Classic Argument Parsing
//CORE_API int              Args_GetIndex( std::string_view search );
CORE_API bool        args_find( const char* search, s32 len = -1 );

// TODO: should i bring this back? no point with the registering system
//CORE_API std::string_view Args_GetString( std::string_view search, std::string_view fallback = "" );
//CORE_API int              Args_GetInt( std::string_view search, int fallback );
//CORE_API float            Args_GetFloat( std::string_view search, float fallback );
//CORE_API double           Args_GetDouble( std::string_view search, double fallback );

// function to be able to find all values like this
// returns true if it finds a value, false if it fails to
CORE_API bool        args_get_value_next( int& index, const char* search, ch_string& ret );

// Get argument count and variables, just like parsing char* argv[] directly
CORE_API int         args_get_count();
CORE_API ch_string*  args_get_variables();

CORE_API void        args_init( int argc, char* argv[] );

