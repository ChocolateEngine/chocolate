#pragma once


#include <stdint.h>
#include <stdlib.h>
#include <initializer_list>

#include "util.h"



// TODO: (for ch_string)
// one day when we have UTF-8 support, we'll need to know whether this is a UTF-8 string or not
// using a different struct could work, and would have compiler errors making sure we don't mix them up
// or we could use the last byte of the size to store the type, but that would be really annoying
// as that would mean we need a function to access size and type separately
// 
// TODO UTF8: a lot of functions will expect UTF-8 strings, but how do we know if the input is UTF-8 or not?
//


#ifdef _DEBUG
	#define CH_STRING_MEM_TRACKING 1
#else
	#define CH_STRING_MEM_TRACKING 0
#endif


#if CH_STRING_MEM_TRACKING
	#define STR_FILE_LINE     __FILE__, __LINE__, CH_FUNC_NAME_CLASS,
	#define STR_FILE_LINE_DEF const char *strtrack_file, size_t strtrack_line, const char *strtrack_func,
	#define STR_FILE_LINE_INT strtrack_file, strtrack_line, strtrack_func,
#else
	#define STR_FILE_LINE
	#define STR_FILE_LINE_DEF
	#define STR_FILE_LINE_INT
#endif

// -----------------------------------------------------------------

// Get the total amount of strings allocated
CORE_API size_t    ch_str_get_alloc_count();

// Get the total amount of memory allocated for strings
CORE_API size_t    ch_str_get_alloc_size();

// Adds a string to the list of strings allocated
// This only does something when string memory tracking is enabled
CORE_API void      ch_str_add( STR_FILE_LINE_DEF const char* string );
CORE_API void      ch_str_add( STR_FILE_LINE_DEF const char* string, size_t size );
CORE_API void      ch_str_add( STR_FILE_LINE_DEF const ch_string& string );

// Removes a string from the list of strings allocated, does not free the memory
// This only does something when string memory tracking is enabled
CORE_API void      ch_str_remove( const char* string );

// -----------------------------------------------------------------

// USE THIS AS STRING ALLOCATIONS ARE TRACKED!
CORE_API void      ch_str_free( char* string );
CORE_API void      ch_str_free( char** strings, size_t count );
CORE_API void      ch_str_free( ch_string string );
CORE_API void      ch_str_free( ch_string* strings, size_t count );
CORE_API void      ch_str_free( std::vector< ch_string >& strings );

// -----------------------------------------------------------------

// Copies a string, useful for if you're copying a string from reading a file and then going to free that string later
CORE_API ch_string ch_str_copy( STR_FILE_LINE_DEF const char* format );
CORE_API ch_string ch_str_copy( STR_FILE_LINE_DEF const char* format, size_t len );

// Automatically appends a null terminator
CORE_API ch_string ch_str_realloc( STR_FILE_LINE_DEF char* data, size_t len );
CORE_API ch_string ch_str_realloc( STR_FILE_LINE_DEF char* data, const char* string );
CORE_API ch_string ch_str_realloc( STR_FILE_LINE_DEF char* data, const char* string, size_t len );

// malloc a string with string formatting
CORE_API ch_string ch_str_copy_f( STR_FILE_LINE_DEF const char* format, ... );
CORE_API ch_string ch_str_copy_v( STR_FILE_LINE_DEF const char* format, va_list args );

// snprintf but it uses realloc
CORE_API ch_string ch_str_realloc_f( STR_FILE_LINE_DEF char* data, const char* format, ... );
CORE_API ch_string ch_str_realloc_v( STR_FILE_LINE_DEF char* data, const char* format, va_list args );

// OLD:
// This doesn't really make sense, when you call concat, you expect to append to the end of a string
// but if you pass in data, it will realloc that memory, so it will be at the start of the string
// naming this to join would make more sense, but that's already used for joining strings with a space
// we could make an "append" function, but that would be the same as concat, but it would append to the end of the string
// and this function name would still be confusing
// maybe change ch_str_join_space to ch_str_join_space_space, and ch_str_join to ch_str_join_space?

// Appends string onto the destination string
CORE_API ch_string ch_str_concat( STR_FILE_LINE_DEF char* dest, const char* string );
CORE_API ch_string ch_str_concat( STR_FILE_LINE_DEF char* dest, size_t destLen, const char* string, size_t stringLen );

// Concatenates an array of strings together, and appends it to the destination
CORE_API ch_string ch_str_concat( STR_FILE_LINE_DEF char* dest, size_t destLen, size_t count, const char** strings );
CORE_API ch_string ch_str_concat( STR_FILE_LINE_DEF char* dest, size_t destLen, size_t count, const char** strings, const size_t* lengths );
CORE_API ch_string ch_str_concat( STR_FILE_LINE_DEF char* dest, size_t destLen, size_t count, const ch_string* strings );

// Joins strings together, pass in data to realloc that instead of allocating new memory
CORE_API ch_string ch_str_join( STR_FILE_LINE_DEF const char* strLeft, const char* strRight );
CORE_API ch_string ch_str_join( STR_FILE_LINE_DEF const char* strLeft, size_t leftLen, const char* strRight, size_t rightLen );

CORE_API ch_string ch_str_join( STR_FILE_LINE_DEF size_t count, const char** strings, char* data = nullptr );
CORE_API ch_string ch_str_join( STR_FILE_LINE_DEF size_t count, const char** strings, const size_t* lengths, char* data = nullptr );
CORE_API ch_string ch_str_join( STR_FILE_LINE_DEF size_t count, const ch_string* strings, char* data = nullptr );

// Joins strings together with a space, or what ever string you want, in between each item
CORE_API ch_string ch_str_join_space( STR_FILE_LINE_DEF size_t count, const char** strings, const char* space = " ", char* data = nullptr );
CORE_API ch_string ch_str_join_space( STR_FILE_LINE_DEF size_t count, const char** strings, const size_t* lengths, const char* space = " ", char* data = nullptr );
CORE_API ch_string ch_str_join_space( STR_FILE_LINE_DEF size_t count, const ch_string* strings, const char* space = " ", char* data = nullptr );


// hmm
// CORE_API ch_string ch_str_join( size_t count, const std::vector< const char* >& strings, const std::vector< size_t >& lengths, char* data = nullptr );

// what if you use std::initilizer_list? hmm

// data can be nullptr, if it is it will allocate new memory
// if it isn't it will realloc that memory
// ONLY PASS IN const char* strings, no other types
// takes in a variable size array of strings
CORE_API ch_string ch_str_join_arr( STR_FILE_LINE_DEF char* data, size_t count, const char* string, ... );

CORE_API ch_string ch_str_join_list( STR_FILE_LINE_DEF char* data, std::initializer_list< const char* > strings );
CORE_API ch_string ch_str_join_list( STR_FILE_LINE_DEF char* data, std::initializer_list< ch_string > strings );


// String equality functions
CORE_API bool      ch_str_equals( const char* str1, const char* str2 );
CORE_API bool      ch_str_equals( const char* str1, const char* str2, size_t count );

CORE_API bool      ch_str_equals( const char* str1, size_t str1Len, const char* str2 );
CORE_API bool      ch_str_equals( const char* str1, size_t str1Len, const char* str2, size_t str2Len );

CORE_API bool      ch_str_equals( const ch_string& str1, const char* str2 );
CORE_API bool      ch_str_equals( const ch_string& str1, const char* str2, size_t str2Len );

CORE_API bool      ch_str_equals( const ch_string& str1, const ch_string& str2 );

CORE_API bool      ch_str_equals( const ch_string_auto& str1, const char* str2 );
CORE_API bool      ch_str_equals( const ch_string_auto& str1, const char* str2, size_t str2Len );

CORE_API bool      ch_str_equals( const ch_string_auto& str1, const ch_string_auto& str2 );
CORE_API bool      ch_str_equals( const ch_string& str1, const ch_string_auto& str2 );


#define CH_STR_EQUALS_STATIC( str1, str2 ) ch_str_equals( str1, str2, sizeof( str2 ) - 1 )


// Compare a string to a list of strings
CORE_API bool      ch_str_equals_any( const char* str1, size_t strLen, size_t count, const char** strings );
CORE_API bool      ch_str_equals_any( const char* str1, size_t strLen, size_t count, const char** strings, size_t* lengths );

CORE_API bool      ch_str_equals_any( const ch_string& str1, size_t count, const char** strings );
CORE_API bool      ch_str_equals_any( const ch_string& str1, size_t count, const char** strings, size_t* lengths );

CORE_API bool      ch_str_equals_any( const ch_string& str1, size_t count, const ch_string* strings );


// -----------------------------------------------------------------------------------------------------


// maybe change size_t to size_t, so you can pass in -1 to calc the string length in the function?
CORE_API bool      ch_str_starts_with( const char* s, size_t len, const char* start );
CORE_API bool      ch_str_starts_with( const char* s, size_t len, const char* start, size_t startLen );

CORE_API bool      ch_str_starts_with( const char* s, const char* start );
CORE_API bool      ch_str_starts_with( const char* s, const char* start, size_t startLen );

CORE_API bool      ch_str_starts_with( const ch_string& s, const char* start );
CORE_API bool      ch_str_starts_with( const ch_string& s, const char* start, size_t startLen );
CORE_API bool      ch_str_starts_with( const ch_string& s, const ch_string& start );

CORE_API bool      ch_str_ends_with( const char* s, size_t len, const char* end );
CORE_API bool      ch_str_ends_with( const char* s, size_t len, const char* end, size_t endLen );

CORE_API bool      ch_str_ends_with( const char* s, const char* end );
CORE_API bool      ch_str_ends_with( const char* s, const char* end, size_t endLen );

CORE_API bool      ch_str_ends_with( const ch_string& s, const char* end );
CORE_API bool      ch_str_ends_with( const ch_string& s, const char* end, size_t endLen );
CORE_API bool      ch_str_ends_with( const ch_string& s, const ch_string& end );

// returns the index of the first instance of find in s
// returns SIZE_MAX if not found
CORE_API size_t    ch_str_contains( const char* s, size_t len, const char* find );
CORE_API size_t    ch_str_contains( const char* s, size_t len, const char* find, size_t findLen );

CORE_API size_t    ch_str_contains( const char* s, const char* find );
CORE_API size_t    ch_str_contains( const char* s, const char* find, size_t findLen );


// check if a string has characters in it
inline bool   ch_str_check_empty( const char* s, size_t& length )
{
	if ( !s )
		return false;

	if ( length <= 0 )
		length = strlen( s );

	return ( length > 0 );
}


// check if a string has characters in it, can't be empty, returns false if it's empty
inline bool ch_str_check_empty( const char* s, s32& length )
{
	if ( !s )
		return false;

	if ( length <= 0 )
	{
		size_t bigLength = strlen( s );

		if ( bigLength > INT32_MAX )
			return false;

		length = static_cast< s32 >( bigLength );
	}

	return ( length > 0 );
}


// ensures the length is correct, returns true if it's greater than 0, false if it's empty
inline bool ch_str_check_len( const char* s, size_t& length )
{
	if ( !s )
		return false;

	if ( length <= 0 )
		length = strlen( s );

	return ( length > 0 );
}


// ensures the length is correct, returns true if it's greater than 0, false if it's empty
inline bool ch_str_check_len( const char* s, s32& length )
{
	if ( !s )
		return false;

	if ( length <= 0 )
	{
		size_t bigLength = strlen( s );

		if ( bigLength > INT32_MAX )
			return false;

		length = static_cast< s32 >( bigLength );
	}

	return ( length > 0 );
}


#if CH_STRING_MEM_TRACKING
	#define ch_str_copy( ... )       ch_str_copy( STR_FILE_LINE __VA_ARGS__ )
	#define ch_str_realloc( ... )    ch_str_realloc( STR_FILE_LINE __VA_ARGS__ )

	#define ch_str_copy_f( ... )     ch_str_copy_f( STR_FILE_LINE __VA_ARGS__ )
	#define ch_str_copy_v( ... )     ch_str_copy_v( STR_FILE_LINE __VA_ARGS__ )

	#define ch_str_realloc_f( ... )  ch_str_realloc_f( STR_FILE_LINE __VA_ARGS__ )
	#define ch_str_realloc_v( ... )  ch_str_realloc_v( STR_FILE_LINE __VA_ARGS__ )

	#define ch_str_concat( ... )     ch_str_concat( STR_FILE_LINE __VA_ARGS__ )

	#define ch_str_join( ... )       ch_str_join( STR_FILE_LINE __VA_ARGS__ )
	#define ch_str_join_space( ... ) ch_str_join_space( STR_FILE_LINE __VA_ARGS__ )

	#define ch_str_join_arr( ... )   ch_str_join_arr( STR_FILE_LINE __VA_ARGS__ )
	#define ch_str_join_list( ... )  ch_str_join_list( STR_FILE_LINE __VA_ARGS__ )

	#define ch_str_add( ... )        ch_str_add( STR_FILE_LINE __VA_ARGS__ )
#endif

