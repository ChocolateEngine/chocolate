#pragma once


#include <stdint.h>
#include <stdlib.h>

#include "util.h"


#ifdef _DEBUG
	#define CH_STRING_MEM_TRACKING 1
#else
	#define CH_STRING_MEM_TRACKING 0
#endif


#if CH_STRING_MEM_TRACKING
	#define STR_FILE_LINE     __FILE__, __LINE__, CH_FUNC_NAME_CLASS,
	#define STR_FILE_LINE_DEF const char *file, u64 line, const char *func,
#else
	#define STR_FILE_LINE
	#define STR_FILE_LINE_DEF
#endif

// Copies a string, useful for if you're copying a string from reading a file and then going to free that string later
CORE_API ch_string ch_str_copy( STR_FILE_LINE_DEF const char* format );
CORE_API ch_string ch_str_copy( STR_FILE_LINE_DEF const char* format, u64 len );

// Automatically appends a null terminator
CORE_API ch_string ch_str_realloc( STR_FILE_LINE_DEF char* data, u64 len );
CORE_API ch_string ch_str_realloc( STR_FILE_LINE_DEF char* data, const char* string );
CORE_API ch_string ch_str_realloc( STR_FILE_LINE_DEF char* data, const char* string, u64 len );

// Returns true if the string was reallocated, false if it wasn't, data is also empty if false
// CORE_API bool ch_str_realloc( ch_string& data, u64 len );
// CORE_API bool ch_str_realloc( ch_string& data, const char* string );
// CORE_API bool ch_str_realloc( ch_string& data, const char* string, u64 len );

// malloc a string with string formatting
CORE_API ch_string ch_str_copy_f( STR_FILE_LINE_DEF const char* format, ... );
CORE_API ch_string ch_str_copy_v( STR_FILE_LINE_DEF const char* format, va_list args );

CORE_API ch_string ch_str_realloc_f( STR_FILE_LINE_DEF char* data, const char* format, ... );
CORE_API ch_string ch_str_realloc_v( STR_FILE_LINE_DEF char* data, const char* format, va_list args );

// Concatinates a string together, pass in data to realloc that instead of allocating new memory
// TODO: rename to ch_str_concat and ch_str_join
CORE_API ch_string ch_str_concat( STR_FILE_LINE_DEF u64 count, const char** strings, char* data = nullptr );
CORE_API ch_string ch_str_join( STR_FILE_LINE_DEF u64 count, const char** strings, const char* space = " ", char* data = nullptr );

CORE_API ch_string ch_str_concat( STR_FILE_LINE_DEF u64 count, const char** strings, const u64* lengths, char* data = nullptr );
CORE_API ch_string ch_str_join( STR_FILE_LINE_DEF u64 count, const char** strings, const u64* lengths, const char* space = " ", char* data = nullptr );

// hmm
// CORE_API ch_string ch_str_concat( u64 count, const std::vector< const char* >& strings, const std::vector< u64 >& lengths, char* data = nullptr );
CORE_API ch_string ch_str_concat( STR_FILE_LINE_DEF u64 count, const ch_string* strings, char* data = nullptr );
CORE_API ch_string ch_str_join( STR_FILE_LINE_DEF u64 count, const ch_string* strings, const char* space = " ", char* data = nullptr );

// data can be nullptr, if it is it will allocate new memory
// if it isn't it will realloc that memory
CORE_API ch_string ch_str_concat( STR_FILE_LINE_DEF char* data, u64 count, const char* string, ... );

// USE THIS AS STRING ALLOCATIONS ARE TRACKED!
// also so the string is freed in the same dll, would probably cause issues otherwise
CORE_API void      ch_str_free( char* string );
CORE_API void      ch_str_free( char** strings, u64 count );
CORE_API void      ch_str_free( ch_string* strings, u64 count );
CORE_API void      ch_str_free( std::vector< ch_string >& files );

CORE_API void      ch_str_add( STR_FILE_LINE_DEF const char* string );
CORE_API void      ch_str_add( STR_FILE_LINE_DEF const char* string, u64 size );
CORE_API void      ch_str_add( STR_FILE_LINE_DEF const ch_string& string );

CORE_API void      ch_str_remove( const char* string );

// String equality functions
CORE_API bool      ch_str_equals( const char* str1, const char* str2 );
CORE_API bool      ch_str_equals( const char* str1, const char* str2, u64 count );

CORE_API bool      ch_str_equals( const char* str1, u64 str1Len, const char* str2 );
CORE_API bool      ch_str_equals( const char* str1, u64 str1Len, const char* str2, u64 str2Len );

CORE_API bool      ch_str_equals( const ch_string& str1, const char* str2 );
CORE_API bool      ch_str_equals( const ch_string& str1, const char* str2, u64 str2Len );

CORE_API bool      ch_str_equals( const ch_string& str1, const ch_string& str2 );

CORE_API bool      ch_str_equals( const ch_string_auto& str1, const char* str2 );
CORE_API bool      ch_str_equals( const ch_string_auto& str1, const char* str2, u64 str2Len );

CORE_API bool      ch_str_equals( const ch_string_auto& str1, const ch_string_auto& str2 );
CORE_API bool      ch_str_equals( const ch_string& str1, const ch_string_auto& str2 );


// Compare a string to a list of strings
CORE_API bool      ch_str_equals_any( const char* str1, u64 strLen, u64 count, const char** strings );
CORE_API bool      ch_str_equals_any( const char* str1, u64 strLen, u64 count, const char** strings, u64* lengths );

CORE_API bool      ch_str_equals_any( const ch_string& str1, u64 count, const char** strings );
CORE_API bool      ch_str_equals_any( const ch_string& str1, u64 count, const char** strings, u64* lengths );

CORE_API bool      ch_str_equals_any( const ch_string& str1, u64 count, const ch_string* strings );

// -----------------------------------------------------------------
// Wide String Functions

#if 0  // _WIN32
CORE_API wchar_t*   ch_str_copy( const wchar_t* format );
CORE_API wchar_t*   ch_str_copy( const wchar_t* format, u64 len );

CORE_API wchar_t*   ch_str_realloc( wchar_t* data, u64 len );
CORE_API wchar_t*   ch_str_realloc( wchar_t* data, const wchar_t* format );
CORE_API wchar_t*   ch_str_realloc( wchar_t* data, const wchar_t* format, u64 len );

CORE_API ch_ustring ch_str_concat( u64 count, const wchar_t** strings, wchar_t* data = nullptr );
CORE_API ch_ustring ch_str_join( u64 count, const wchar_t** strings, const wchar_t* space = (wchar_t*)L" ", wchar_t* data = nullptr );

CORE_API ch_ustring ch_str_concat( u64 count, const wchar_t** strings, const u64* lengths, wchar_t* data = nullptr );
CORE_API ch_ustring ch_str_join( u64 count, const wchar_t** strings, const u64* lengths, const wchar_t* space = (wchar_t*)L" ", wchar_t* data = nullptr );

// CORE_API wchar_t*   ch_str_concat( wchar_t* data, u64 count, const wchar_t* string, ... );
CORE_API ch_ustring ch_str_concat( wchar_t* data, u64 count, const wchar_t* string, ... );

CORE_API wchar_t*   ch_str_copy_f( const wchar_t* format, ... );
CORE_API wchar_t*   ch_str_copy_v( const wchar_t* format, va_list args );

CORE_API void       ch_str_free( wchar_t* string );

CORE_API void       ch_str_add( const wchar_t* string );
CORE_API void       ch_str_remove( const wchar_t* string );

CORE_API bool       ch_streq( const wchar_t* str1, const wchar_t* str2 );
CORE_API bool       ch_strneq( const wchar_t* str1, const wchar_t* str2, u64 count );

// compare a wide string to a normal string
CORE_API bool       ch_streq( const char* str1, const wchar_t* str2 );
CORE_API bool       ch_strneq( const char* str1, const wchar_t* str2, u64 count );

CORE_API bool       ch_streq( const wchar_t* str1, const char* str2 );
CORE_API bool       ch_strneq( const wchar_t* str1, const char* str2, u64 count );
#endif

// Get the total amount of strings allocated
CORE_API u64        Util_GetStringAllocCount();

// -----------------------------------------------------------------------------------------------------
// Creation and Destruction

CORE_API bool       ch_str_create( ch_string& s, const char* str );
// CORE_API bool       ch_str_create( ch_ustring& s, const uchar* str );

CORE_API bool       ch_str_create( ch_string& s, const char* str, u64 len );
// CORE_API bool       ch_str_create( ch_ustring& s, const uchar* str, u32 len );

// check if this has a nullptr in data to see if it was successful or not
CORE_API ch_string  ch_str_create( const char* str );
// CORE_API ch_ustring ch_str_create( const uchar* str );

CORE_API ch_string  ch_str_create( const char* str, u64 len );
// CORE_API ch_ustring ch_str_create( const uchar* str, u32 len );

#if 0

CORE_API void       ch_str_destroy( ch_string& s );
CORE_API void       ch_str_destroy( ch_ustring& s );

// -----------------------------------------------------------------------------------------------------
// Comparisons

CORE_API bool       ch_str_compare( const ch_string& s1, const ch_string& s2 );
CORE_API bool       ch_str_compare( const ch_ustring& s1, const ch_ustring& s2 );

CORE_API bool       ch_str_compare( const ch_string& s1, const char* s2 );
CORE_API bool       ch_str_compare( const ch_ustring& s1, const uchar* s2 );

CORE_API bool       ch_str_compare( const ch_string& s1, const char* s2, u64 len );
CORE_API bool       ch_str_compare( const ch_ustring& s1, const uchar* s2, u32 len );

// -----------------------------------------------------------------------------------------------------
// Appending

CORE_API bool       ch_str_append( ch_string& dest, const ch_string& src );
CORE_API bool       ch_str_append( ch_ustring& dest, const ch_ustring& src );

CORE_API bool       ch_str_append( ch_string& dest, const char* src );
CORE_API bool       ch_str_append( ch_ustring& dest, const uchar* src );

CORE_API bool       ch_str_append( ch_string& dest, const char* src, u64 len );
CORE_API bool       ch_str_append( ch_ustring& dest, const uchar* src, u32 len );

CORE_API bool       ch_str_append( ch_string& dest, const ch_string* srcList, const size_t sCount );
CORE_API bool       ch_str_append( ch_ustring& dest, const ch_ustring* srcList, const size_t sCount );

// -----------------------------------------------------------------------------------------------------
// Assigning

CORE_API bool       ch_str_assign( ch_string& dest, const ch_string& src );
CORE_API bool       ch_str_assign( ch_ustring& dest, const ch_ustring& src );

CORE_API bool       ch_str_assign( ch_string& dest, const char* src );
CORE_API bool       ch_str_assign( ch_ustring& dest, const uchar* src );

CORE_API bool       ch_str_assign( ch_string& dest, const char* src, u64 len );
CORE_API bool       ch_str_assign( ch_ustring& dest, const uchar* src, u32 len );

// -----------------------------------------------------------------------------------------------------
// Other

// Convert Between Unicode and ANSI
CORE_API bool       ch_str_convert( ch_string& dest, const ch_ustring& src );
CORE_API bool       ch_str_convert( ch_ustring& dest, const ch_string& src );


inline bool         ch_str_copy( ch_string& dest, const ch_string& src )
{
	if ( dest.data )
		ch_str_destroy( dest );

	return ch_str_create( dest, src.data, src.size );
}


inline bool ch_str_copy( ch_ustring& dest, const ch_ustring& src )
{
	if ( dest.data )
		ch_str_destroy( dest );

	return ch_str_create( dest, src.data, src.size );
}

#endif


// maybe change u64 to s64, so you can pass in -1 to calc the string length in the function?
CORE_API bool ch_str_starts_with( const char* s, u64 len, const char* start );
CORE_API bool ch_str_starts_with( const char* s, u64 len, const char* start, u64 startLen );

CORE_API bool ch_str_starts_with( const char* s, const char* start );
CORE_API bool ch_str_starts_with( const char* s, const char* start, u64 startLen );

CORE_API bool ch_str_starts_with( const ch_string& s, const char* start );
CORE_API bool ch_str_starts_with( const ch_string& s, const char* start, u64 startLen );
CORE_API bool ch_str_starts_with( const ch_string& s, const ch_string& start );

CORE_API bool ch_str_ends_with( const char* s, u64 len, const char* end );
CORE_API bool ch_str_ends_with( const char* s, u64 len, const char* end, u64 endLen );

CORE_API bool ch_str_ends_with( const char* s, const char* end );
CORE_API bool ch_str_ends_with( const char* s, const char* end, u64 endLen );

CORE_API bool ch_str_ends_with( const ch_string& s, const char* end );
CORE_API bool ch_str_ends_with( const ch_string& s, const char* end, u64 endLen );
CORE_API bool ch_str_ends_with( const ch_string& s, const ch_string& end );

// returns the index of the first instance of find in s
// returns SIZE_MAX if not found
CORE_API u64 ch_str_contains( const char* s, u64 len, const char* find );
CORE_API u64 ch_str_contains( const char* s, u64 len, const char* find, u64 findLen );

CORE_API u64 ch_str_contains( const char* s, const char* find );
CORE_API u64 ch_str_contains( const char* s, const char* find, u64 findLen );


#if CH_STRING_MEM_TRACKING
	#define ch_str_copy( ... )       ch_str_copy( STR_FILE_LINE __VA_ARGS__ )
	#define ch_str_realloc( ... )     ch_str_realloc( STR_FILE_LINE __VA_ARGS__ )

	#define ch_str_copy_f( ... )      ch_str_copy_f( STR_FILE_LINE __VA_ARGS__ )
	#define ch_str_copy_v( ... )      ch_str_copy_v( STR_FILE_LINE __VA_ARGS__ )

	#define ch_str_realloc_f( ... )    ch_str_realloc_f( STR_FILE_LINE __VA_ARGS__ )
	#define ch_str_realloc_v( ... )    ch_str_realloc_v( STR_FILE_LINE __VA_ARGS__ )

	#define ch_str_concat( ... ) ch_str_concat( STR_FILE_LINE __VA_ARGS__ )
	#define ch_str_join( ... )   ch_str_join( STR_FILE_LINE __VA_ARGS__ )

	#define ch_str_add( ... )    ch_str_add( STR_FILE_LINE __VA_ARGS__ )
#endif

