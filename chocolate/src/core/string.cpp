#include "string.h"
#include "core/platform.h"
#include "core/util.h"

#include <map>


// these defines are used to add file, line, and function arguments to the string functions
#if CH_STRING_MEM_TRACKING
	#undef ch_str_copy
	#undef ch_str_realloc

	#undef ch_str_copy_f
	#undef ch_str_copy_v

	#undef ch_str_realloc_f
	#undef ch_str_realloc_v

	#undef ch_str_concat

	#undef ch_str_join
	#undef ch_str_join_space

	#undef ch_str_join_arr
	#undef ch_str_join_list

	#undef ch_str_add
#endif

// memory allocation stat tracking
#ifdef _DEBUG
	#if CH_STRING_MEM_TRACKING


struct ch_string_track_data
{
	const char* file;
	const char* func;
	u32         line;
	size_t         size;
};

// using ch_string_track_map = std::unordered_map< char*, ch_string_track_data >;
using ch_string_track_map = std::map< char*, ch_string_track_data >;


ch_string_track_map& str_track_get()
{
	static ch_string_track_map trackedStrings;
	return trackedStrings;
}

static void str_track_alloc( const char* file, u32 line, const char* func, const char* string, size_t len )
{
	PROF_SCOPE();

	ch_string_track_data& data = str_track_get()[ (char*)string ];
	data.file                  = file;
	data.func                  = func;
	data.line                  = line;
	data.size                  = len;
}


static void str_track_realloc( const char* file, u32 line, const char* func, char* data, size_t len, char* oldPtr )
{
	PROF_SCOPE();

	if ( oldPtr && oldPtr != data )
	{
		auto it = str_track_get().find( oldPtr );

		if ( it != str_track_get().end() )
		{
			str_track_get().erase( it );
		}
		else
		{
			print( "Failed to find old string pointer in tracking data!\n" );
		}
	}

	str_track_alloc( file, line, func, data, len );
}


static void str_track_free( const char* string )
{
	PROF_SCOPE();

	if ( string == nullptr )
	{
		print( "Attempted to free nullptr string!\n" );
		return;
	}

	if ( str_track_get().empty() )
	{
		print( "No strings tracked to free!\n" );
	}
	else
	{
		auto it = str_track_get().find( (char*)string );

		if ( it != str_track_get().end() )
		{
			str_track_get().erase( it );
		}
		else
		{
			print( "Failed to find string pointer in tracking data to erase!\n" );
		}
	}
}

	#else
		#define str_track_alloc( string, len )
		#define str_track_realloc( data, len, oldPtr )
		#define str_track_free( string )
	#endif

#else
	#define str_track_alloc( string, len )
	#define str_track_realloc( data, len, oldPtr )
	#define str_track_free( string )

	#define CH_STRING_MEM_TRACKING 0
#endif


// ===========================================================================================
// Core String Allocation System
// ===========================================================================================


char* ch_str_copy_base( const char* string, size_t len )
{
	char* out = ch_malloc< char >( len + 1 );  // allocate extra byte for null terminator

	if ( out == nullptr )
	{
		printf( "Failed to allocate %zu bytes for string!\n", ( len + 1 ) * sizeof( char ) );
		return nullptr;
	}

	memcpy( out, string, len );
	out[ len ] = '\0';

	return out;
}


ch_string ch_str_copy( STR_FILE_LINE_DEF const char* string )
{
	ch_string out_string;
	out_string.data = nullptr;
	out_string.size = 0;

	if ( string == nullptr )
	{
		print( "Attempted to copy nullptr string!\n" );
		return out_string;
	}

	size_t   len = strlen( string );
	char* out = ch_str_copy_base( string, len );

	if ( out == nullptr )
		return out_string;

	str_track_alloc( STR_FILE_LINE_INT out, len + 1 );

	out_string.data = out;
	out_string.size = len;
	return out_string;
}


ch_string ch_str_copy( STR_FILE_LINE_DEF const char* string, size_t len )
{
	ch_string out_string;
	out_string.data = nullptr;
	out_string.size = 0;

	if ( string == nullptr )
	{
		print( "Attempted to copy nullptr string!\n" );
		return out_string;
	}

	char* out = ch_str_copy_base( string, len );

	if ( out == nullptr )
		return out_string;

	str_track_alloc( STR_FILE_LINE_INT out, len + 1 );

	out_string.data = out;
	out_string.size = len;
	return out_string;
}


// --------------------------------------------------------------------------
// Reallocating Strings


ch_string ch_str_realloc( STR_FILE_LINE_DEF char* data, size_t len )
{
	ch_string out_string;
	out_string.data = nullptr;
	out_string.size = 0;

	char* out       = ch_realloc< char >( data, len + 1 );

	if ( out == nullptr )
		return out_string;

	str_track_realloc( STR_FILE_LINE_INT out, len + 1, data );

	out_string.data = out;
	out_string.size = len;
	return out_string;
}


ch_string ch_str_realloc( STR_FILE_LINE_DEF char* data, const char* string, size_t len )
{
	ch_string out_string;
	out_string.data = nullptr;
	out_string.size = 0;

	if ( string == nullptr )
	{
		printf( "Attempted to copy nullptr string!\n" );
		return out_string;
	}

	char* out = ch_realloc< char >( data, len + 1 );

	if ( out == nullptr )
		return out_string;

	memcpy( out, string, len );
	out[ len ] = '\0';

	str_track_realloc( STR_FILE_LINE_INT out, len + 1, data );

	out_string.data = out;
	out_string.size = len;
	return out_string;
}


ch_string ch_str_realloc( STR_FILE_LINE_DEF char* data, const char* string )
{
	ch_string out_string;
	out_string.data = nullptr;
	out_string.size = 0;

	if ( string == nullptr )
	{
		printf( "Attempted to copy nullptr string!\n" );
		return out_string;
	}

	size_t len = strlen( string );

	if ( len == 0 )
	{
		printf( "Attempted to copy empty string!\n" );
		return out_string;
	}

	char* out = ch_realloc< char >( data, len + 1 );

	if ( out == nullptr )
		return out_string;

	memcpy( out, string, len );
	out[ len ] = '\0';

	str_track_realloc( STR_FILE_LINE_INT out, len + 1, data );

	out_string.data = out;
	out_string.size = len;
	return out_string;
}


CORE_API ch_string ch_str_realloc_f( STR_FILE_LINE_DEF char* data, const char* format, ... )
{
	ch_string out_string;
	out_string.data = nullptr;
	out_string.size = 0;

	char*   result  = nullptr;
	va_list args, args_copy;

	va_start( args, format );
	va_copy( args_copy, args );

	int len = vsnprintf( nullptr, 0, format, args );
	if ( len < 0 )
	{
		va_end( args_copy );
		va_end( args );
		print( "ch_str_realloc_f va_args: vsnprintf failed\n" );
		return out_string;
	}

	if ( len > 0 )
	{
		result = ch_realloc< char >( data, len + 1 );

		if ( result == nullptr )
		{
			printf( "Failed to allocate %zu bytes for string!\n", ( len + 1 ) * sizeof( char ) );
			va_end( args_copy );
			va_end( args );
			return out_string;
		}

		vsnprintf( result, len + 1, format, args_copy );
		result[ len ] = '\0';

		str_track_alloc( STR_FILE_LINE_INT result, len + 1 );
	}

	va_end( args_copy );
	va_end( args );

	out_string.data = result;
	out_string.size = len;
	return out_string;
}


CORE_API ch_string ch_str_realloc_v( STR_FILE_LINE_DEF char* data, const char* format, va_list args )
{
	ch_string out_string;
	out_string.data = nullptr;
	out_string.size = 0;

	va_list copy;
	va_copy( copy, args );
	int len = std::vsnprintf( nullptr, 0, format, copy );
	va_end( copy );

	if ( len >= 0 )
	{
		char* result = ch_realloc< char >( data, len + 1 );

		if ( result == nullptr )
		{
			printf( "Failed to allocate %zu bytes for string!\n", ( len + 1 ) * sizeof( char ) );
			return out_string;
		}

		std::vsnprintf( result, len + 1, format, args );
		result[ len ] = '\0';

		str_track_alloc( STR_FILE_LINE_INT result, len + 1 );

		out_string.data = result;
		out_string.size = len;
		return out_string;
	}

	return out_string;
}


// --------------------------------------------------------------------------
// Allocating Strings with printf Formatting


ch_string ch_str_copy_f( STR_FILE_LINE_DEF const char* format, ... )
{
	ch_string out_string;
	out_string.data = nullptr;
	out_string.size = 0;

	char*   result  = nullptr;
	va_list args, args_copy;

	va_start( args, format );
	va_copy( args_copy, args );

	int len = vsnprintf( nullptr, 0, format, args );
	if ( len < 0 )
	{
		va_end( args_copy );
		va_end( args );
		print( "ch_str_copy_f va_args: vsnprintf failed\n" );
		return out_string;
	}

	if ( len > 0 )
	{
		result = ch_malloc< char >( len + 1 );
		vsnprintf( result, len + 1, format, args_copy );
		result[ len ] = '\0';

		str_track_alloc( STR_FILE_LINE_INT result, len + 1 );
	}

	va_end( args_copy );
	va_end( args );

	out_string.data = result;
	out_string.size = len;
	return out_string;
}


ch_string ch_str_copy_v( STR_FILE_LINE_DEF const char* format, va_list args )
{
	ch_string out_string;
	out_string.data = nullptr;
	out_string.size = 0;

	va_list copy;
	va_copy( copy, args );
	int len = std::vsnprintf( nullptr, 0, format, copy );
	va_end( copy );

	if ( len >= 0 )
	{
		char* result = ch_malloc< char >( len + 1 );
		std::vsnprintf( result, len + 1, format, args );
		result[ len ] = '\0';

		str_track_alloc( STR_FILE_LINE_INT result, len + 1 );

		out_string.data = result;
		out_string.size = len;
		return out_string;
	}

	return out_string;
}


// --------------------------------------------------------------------------
// String Concatenation
// TODO: there's a lot of duplicated code here, need to refactor


// simple single string concatenation
ch_string ch_str_concat( STR_FILE_LINE_DEF char* dest, const char* string )
{
	ch_string outString;
	outString.data = nullptr;
	outString.size = 0;

	if ( string == nullptr )
	{
		print( "Attempted to concatenate nullptr string!\n" );
		return outString;
	}

	size_t   destLen = strlen( dest );
	size_t   strLen  = strlen( string );

	char* out     = ch_realloc< char >( dest, destLen + strLen + 1 );

	if ( out == nullptr )
	{
		printf( "Failed to allocate %zu bytes for string!\n", ( destLen + strLen + 1 ) * sizeof( char ) );
		return outString;
	}

	// char* rOut = out;
	// 
	// while ( *rOut )
	// 	rOut++;
	// 
	// while ( *rOut++ = *string++ )
	// 	;

	memcpy( out + destLen, string, strLen );
	out[ destLen + strLen ] = '\0';

	str_track_realloc( STR_FILE_LINE_INT out, destLen + strLen + 1, dest );

	outString.data = out;
	outString.size = destLen + strLen;
	return outString;
}


ch_string ch_str_concat( STR_FILE_LINE_DEF char* dest, size_t destLen, const char* string, size_t stringLen )
{
	ch_string outString;
	outString.data = nullptr;
	outString.size = 0;

	if ( string == nullptr )
	{
		print( " *** Attempted to concatenate nullptr string!\n" );
		return outString;
	}

	if ( !ch_str_check_len( dest, destLen ) && !ch_str_check_len( string, stringLen ) )
	{
		print( " *** Both strings to concat are empty!\n" );
		return outString;
	}

	char* out = ch_realloc< char >( dest, destLen + stringLen + 1 );

	if ( out == nullptr )
	{
		printf( "Failed to allocate %zu bytes for string!\n", ( destLen + stringLen + 1 ) * sizeof( char ) );
		return outString;
	}

	memcpy( out + destLen, string, stringLen );
	out[ destLen + stringLen ] = '\0';

	str_track_realloc( STR_FILE_LINE_INT out, destLen + stringLen + 1, dest );

	outString.data = out;
	outString.size = destLen + stringLen;
	return outString;
}


// Concatenates an array of strings together, and appends it to the destination
ch_string ch_str_concat( STR_FILE_LINE_DEF char* dest, size_t destLen, size_t count, const char** strings )
{
	ch_string outString;
	outString.data = nullptr;
	outString.size = 0;

	size_t  totalLen  = destLen;

	size_t* lengths   = ch_malloc< size_t >( count );

	if ( lengths == nullptr )
	{
		printf( "Failed to allocate %zu bytes for string length array!\n", count * sizeof( size_t ) );
		return outString;
	}

	for ( size_t i = 0; i < count; i++ )
	{
		if ( !strings[ i ] )
		{
			lengths[ i ] = 0;
			continue;
		}

		lengths[ i ] = strlen( strings[ i ] );
		totalLen += lengths[ i ];
	}

	// check the strings list to see if the data pointer is in there

	for ( size_t i = 0; i < count && dest; i++ )
	{
		if ( strings[ i ] == dest )
		{
			// Technically, we could allocate a new string here and copy the data into it
			// im not sure if that would be a good idea or not
			print( "Attempted to concatenate string with itself!\n" );
			ch_free( lengths );
			return outString;
		}
	}

	char* out = ch_realloc< char >( dest, totalLen + 1 );

	if ( out == nullptr )
	{
		printf( "Failed to allocate %zu bytes for string!\n", ( totalLen + 1 ) * sizeof( char ) );
		ch_free( lengths );
		return outString;
	}

	size_t offset = 0;
	for ( size_t i = 0; i < count; i++ )
	{
		if ( lengths[ i ] == 0 || !strings[ i ] )
			continue;

		memcpy( out + destLen + offset, strings[ i ], lengths[ i ] );
		offset += lengths[ i ];
	}

	out[ totalLen ] = '\0';

	str_track_realloc( STR_FILE_LINE_INT out, totalLen + 1, dest );

	ch_free( lengths );

	outString.data = out;
	outString.size = totalLen;
	return outString;
}


ch_string ch_str_concat( STR_FILE_LINE_DEF char* dest, size_t destLen, size_t count, const char** strings, const size_t* lengths )
{
	ch_string outString;
	outString.data = nullptr;
	outString.size = 0;

	size_t totalLen   = destLen;

	for ( size_t i = 0; i < count; i++ )
		totalLen += lengths[ i ];

	char* out = ch_realloc< char >( dest, totalLen + 1 );

	if ( out == nullptr )
	{
		printf( "Failed to allocate %zu bytes for string!\n", ( totalLen + 1 ) * sizeof( char ) );
		return outString;
	}

	size_t offset = 0;

	for ( size_t i = 0; i < count; i++ )
	{
		memcpy( out + destLen + offset, strings[ i ], lengths[ i ] );
		offset += lengths[ i ];
	}

	out[ totalLen ] = '\0';

	str_track_realloc( STR_FILE_LINE_INT out, totalLen + 1, dest );

	outString.data = out;
	outString.size = totalLen;
	return outString;
}


ch_string ch_str_concat( STR_FILE_LINE_DEF char* dest, size_t destLen, size_t count, const ch_string* strings )
{
	ch_string outString;
	outString.data = nullptr;
	outString.size = 0;

	size_t totalLen   = destLen;

	for ( size_t i = 0; i < count; i++ )
		totalLen += strings[ i ].size;

	char* out = ch_realloc< char >( dest, totalLen + 1 );

	if ( out == nullptr )
	{
		printf( "Failed to allocate %zu bytes for string!\n", ( totalLen + 1 ) * sizeof( char ) );
		return outString;
	}

	size_t offset = 0;

	for ( size_t i = 0; i < count; i++ )
	{
		memcpy( out + destLen + offset, strings[ i ].data, strings[ i ].size );
		offset += strings[ i ].size;
	}

	out[ totalLen ] = '\0';

	str_track_realloc( STR_FILE_LINE_INT out, totalLen + 1, dest );

	outString.data = out;
	outString.size = totalLen;
	return outString;
}


// --------------------------------------------------------------------------
// String Joining
// TODO: there's a lot of duplicated code here, need to refactor


ch_string ch_str_join( STR_FILE_LINE_DEF const char* strLeft, const char* strRight )
{
	ch_string outString;
	outString.data = nullptr;
	outString.size = 0;

	if ( strLeft == nullptr || strRight == nullptr )
	{
		print( "Attempted to join nullptr string!\n" );
		return outString;
	}

	size_t   leftLen = strlen( strLeft );
	size_t   rightLen = strlen( strRight );

	char* out      = ch_malloc< char >( leftLen + rightLen + 1 );

	if ( out == nullptr )
	{
		printf( "Failed to allocate %zu bytes for string!\n", ( leftLen + rightLen + 1 ) * sizeof( char ) );
		return outString;
	}

	memcpy( out, strLeft, rightLen );
	memcpy( out + leftLen, strRight, rightLen );
	out[ leftLen + rightLen ] = '\0';

	str_track_alloc( STR_FILE_LINE_INT out, leftLen + rightLen + 1 );

	outString.data = out;
	outString.size = leftLen + rightLen;
	return outString;
}


ch_string ch_str_join( STR_FILE_LINE_DEF const char* strLeft, size_t leftLen, const char* strRight, size_t rightLen )
{
	ch_string outString;
	outString.data = nullptr;
	outString.size = 0;

	if ( strRight == nullptr )
	{
		print( "Attempted to join nullptr string!\n" );
		return outString;
	}

	if ( !ch_str_check_len( strLeft, leftLen ) || !ch_str_check_len( strRight, rightLen ) )
	{
		print( "Invalid string length!\n" );
		return outString;
	}

	char* out = ch_malloc< char >( leftLen + rightLen + 1 );

	if ( out == nullptr )
	{
		printf( "Failed to allocate %zu bytes for string!\n", ( leftLen + rightLen + 1 ) * sizeof( char ) );
		return outString;
	}
	
	memcpy( out, strLeft, rightLen );
	memcpy( out + leftLen, strRight, rightLen );
	out[ leftLen + rightLen ] = '\0';

	str_track_alloc( STR_FILE_LINE_INT out, leftLen + rightLen + 1 );

	outString.data = out;
	outString.size = leftLen + rightLen;
	return outString;
}


ch_string ch_str_join( STR_FILE_LINE_DEF size_t count, const char** strings, char* data )
{
	ch_string outString;
	outString.data = nullptr;
	outString.size = 0;

	size_t  totalLen  = 0;

	size_t* lengths   = ch_malloc< size_t >( count );

	if ( lengths == nullptr )
	{
		printf( "Failed to allocate %zu bytes for string length array!\n", count * sizeof( size_t ) );
		return outString;
	}

	for ( size_t i = 0; i < count; i++ )
	{
		if ( !strings[ i ] )
		{
			lengths[ i ] = 0;
			continue;
		}

		lengths[ i ] = strlen( strings[ i ] );
		totalLen += lengths[ i ];
	}

	// check the strings list to see if the data pointer is in there

	for ( size_t i = 0; i < count && data; i++ )
	{
		if ( strings[ i ] == data )
		{
			// Technically, we could allocate a new string here and copy the data into it
			// im not sure if that would be a good idea or not
			print( "Attempted to concatenate string with itself!\n" );
			ch_free( lengths );
			return outString;
		}
	}

	char* out = ch_realloc< char >( data, totalLen + 1 );

	if ( out == nullptr )
	{
		printf( "Failed to allocate %zu bytes for string!\n", ( totalLen + 1 ) * sizeof( char ) );
		ch_free( lengths );
		return outString;
	}

	size_t offset = 0;
	for ( size_t i = 0; i < count; i++ )
	{
		if ( lengths[ i ] == 0 || !strings[ i ] )
			continue;

		memcpy( out + offset, strings[ i ], lengths[ i ] );
		offset += lengths[ i ];
	}

	out[ totalLen ] = '\0';

	str_track_realloc( STR_FILE_LINE_INT out, totalLen + 1, data );

	ch_free( lengths );

	outString.data = out;
	outString.size = totalLen;
	return outString;
}


ch_string ch_str_join( STR_FILE_LINE_DEF size_t count, const char** strings, const size_t* lengths, char* data )
{
	ch_string outString;
	outString.data = nullptr;
	outString.size = 0;

	size_t totalLen   = 0;

	for ( size_t i = 0; i < count; i++ )
		totalLen += lengths[ i ];

	char* out = ch_realloc< char >( data, totalLen + 1 );

	if ( out == nullptr )
	{
		printf( "Failed to allocate %zu bytes for string!\n", ( totalLen + 1 ) * sizeof( char ) );
		return outString;
	}

	size_t offset = 0;

#if 1
	for ( size_t i = 0; i < count; i++ )
	{
		memcpy( out + offset, strings[ i ], lengths[ i ] );
		offset += lengths[ i ];
	}
#elif 0  // slightly slower than memcpy
	for ( size_t i = 0; i < count; i++ )
	{
		char* outOffset = out + offset;
		for ( size_t j = 0; j < lengths[ i ]; j++ )
		{
			// memcpy( out + offset, strings[ i ], lengths[ i ] );
			outOffset[ j ] = strings[ i ][ j ];
		}
		offset += lengths[ i ];

		// memcpy( out + offset, strings[ i ], lengths[ i ] );
		offset += lengths[ i ];
	}
#else    // slowest
	// adding this here so i know to check for this
	CH_ASSERT( 0 );

	char* outOffset = out;
	size_t   remaining = count;
	while ( remaining-- )
	{
		size_t   strRemain = *lengths;
		char* str       = (char*)*strings;
		while ( strRemain-- )
		{
			*outOffset++ = *str++;
		}

		lengths++;
		strings++;
	}
#endif

	out[ totalLen ] = '\0';

	str_track_realloc( STR_FILE_LINE_INT out, totalLen + 1, data );

	outString.data = out;
	outString.size = totalLen;
	return outString;
}


ch_string ch_str_join( STR_FILE_LINE_DEF size_t count, std::vector< const char* >& strings, const std::vector< size_t >& lengths, char* data )
{
	return ch_str_join( STR_FILE_LINE_INT count, strings.data(), lengths.data(), data );
}


ch_string ch_str_join( STR_FILE_LINE_DEF size_t count, const ch_string* strings, char* data )
{
	ch_string outString;
	outString.data = nullptr;
	outString.size = 0;

	size_t totalLen   = 0;

	for ( size_t i = 0; i < count; i++ )
		totalLen += strings[ i ].size;

	char* out = ch_realloc< char >( data, totalLen + 1 );

	if ( out == nullptr )
	{
		printf( "Failed to allocate %zu bytes for string!\n", ( totalLen + 1 ) * sizeof( char ) );
		return outString;
	}

	size_t offset = 0;

	for ( size_t i = 0; i < count; i++ )
	{
		memcpy( out + offset, strings[ i ].data, strings[ i ].size );
		offset += strings[ i ].size;
	}

	out[ totalLen ] = '\0';

	str_track_realloc( STR_FILE_LINE_INT out, totalLen + 1, data );

	outString.data = out;
	outString.size = totalLen;
	return outString;
}


// --------------------------------------------------------------------------


ch_string ch_str_join_space( STR_FILE_LINE_DEF size_t count, const char** strings, const char* space, char* data )
{
	ch_string outString;
	outString.data = nullptr;
	outString.size = 0;

	if ( !space )
		space = " ";

	size_t  totalLen = 0;

	size_t* lengths  = ch_malloc< size_t >( count );

	if ( lengths == nullptr )
	{
		printf( "Failed to allocate %zu bytes for string length array!\n", count * sizeof( size_t ) );
		return outString;
	}

	for ( size_t i = 0; i < count; i++ )
	{
		lengths[ i ] = strlen( strings[ i ] );
		totalLen += lengths[ i ];
	}

	size_t spaceLen = strlen( space );
	totalLen += ( count - 1 ) * spaceLen;

	char* out = ch_realloc< char >( data, totalLen + 1 );

	if ( out == nullptr )
	{
		printf( "Failed to allocate %zu bytes for string!\n", ( totalLen + 1 ) * sizeof( char ) );
		return outString;
	}

	size_t offset = 0;
	for ( size_t i = 0; i < count; i++ )
	{
		memcpy( out + offset, strings[ i ], lengths[ i ] );
		offset += lengths[ i ];

		if ( i < count - 1 )
		{
			memcpy( out + offset, space, spaceLen );
			offset += spaceLen;
		}
	}

	out[ totalLen ] = '\0';

	str_track_realloc( STR_FILE_LINE_INT out, totalLen + 1, data );

	outString.data = out;
	outString.size = totalLen;
	return outString;
}


ch_string ch_str_join_space( STR_FILE_LINE_DEF size_t count, const char** strings, const size_t* lengths, const char* space, char* data )
{
	ch_string outString;
	outString.data = nullptr;
	outString.size = 0;

	if ( !space )
		space = " ";

	size_t totalLen = 0;
	for ( size_t i = 0; i < count; i++ )
		totalLen += lengths[ i ];

	size_t spaceLen = strlen( space );
	totalLen += ( count - 1 ) * spaceLen;

	char* out = ch_realloc< char >( data, totalLen + 1 );

	if ( out == nullptr )
	{
		printf( "Failed to allocate %zu bytes for string!\n", ( totalLen + 1 ) * sizeof( char ) );
		return outString;
	}

	size_t offset = 0;
	for ( size_t i = 0; i < count; i++ )
	{
		memcpy( out + offset, strings[ i ], lengths[ i ] );
		offset += lengths[ i ];

		if ( i < count - 1 )
		{
			memcpy( out + offset, space, spaceLen );
			offset += spaceLen;
		}
	}

	out[ totalLen ] = '\0';

	str_track_realloc( STR_FILE_LINE_INT out, totalLen + 1, data );

	outString.data = out;
	outString.size = totalLen;
	return outString;
}


ch_string ch_str_join_space( STR_FILE_LINE_DEF size_t count, const ch_string* strings, const char* space, char* data )
{
	ch_string outString;
	outString.data = nullptr;
	outString.size = 0;

	if ( !space )
		space = " ";

	size_t totalLen = 0;
	for ( size_t i = 0; i < count; i++ )
		totalLen += strings[ i ].size;

	size_t spaceLen = strlen( space );
	totalLen += ( count - 1 ) * spaceLen;

	char* out = ch_realloc< char >( data, totalLen + 1 );

	if ( out == nullptr )
	{
		printf( "Failed to allocate %zu bytes for string!\n", ( totalLen + 1 ) * sizeof( char ) );
		return outString;
	}

	size_t offset = 0;
	for ( size_t i = 0; i < count; i++ )
	{
		memcpy( out + offset, strings[ i ].data, strings[ i ].size );
		offset += strings[ i ].size;

		if ( i < count - 1 )
		{
			memcpy( out + offset, space, spaceLen );
			offset += spaceLen;
		}
	}

	out[ totalLen ] = '\0';

	str_track_realloc( STR_FILE_LINE_INT out, totalLen + 1, data );

	outString.data = out;
	outString.size = totalLen;
	return outString;
}


ch_string ch_str_join_arr( STR_FILE_LINE_DEF char* data, size_t count, const char* string, ... )
{
	ch_string outString;
	outString.data = nullptr;
	outString.size = 0;

	if ( count == 0 )
	{
		print( "No strings in array to concatenate - count of 0!\n" );
		return outString;
	}

	size_t  totalLen  = 0;
	size_t* lengths   = ch_malloc< size_t >( count );

	if ( lengths == nullptr )
	{
		printf( "Failed to allocate %zu bytes for string length array!\n", count * sizeof( size_t ) );
		return outString;
	}

	va_list args;
	va_list args_copy;

	va_start( args, string );
	va_copy( args_copy, args );

	lengths[ 0 ] = strlen( string );
	totalLen += lengths[ 0 ];

	for ( size_t i = 1; i < count; i++ )
	{
		const char* arg = va_arg( args_copy, const char* );

		if ( arg == nullptr )
		{
			Log_WarnF( "Attempted to concatenate nullptr string at index %d!\n", i );
			lengths[ i ] = 0;
			continue;
		}

		lengths[ i ] = strlen( arg );
		totalLen += lengths[ i ];
	}

	va_end( args_copy );

	char* out = ch_realloc< char >( data, totalLen + 1 );

	if ( out == nullptr )
	{
		printf( "Failed to allocate %zu bytes for string!\n", ( totalLen + 1 ) * sizeof( char ) );
		ch_free( lengths );
		//va_end( args_copy );
		return outString;
	}

	memcpy( out, string, lengths[ 0 ] );

	size_t offset = lengths[ 0 ];
	for ( size_t i = 1; i < count; i++ )
	{
		const char* arg = va_arg( args, const char* );

		if ( arg == nullptr )
			continue;

		memcpy( out + offset, arg, lengths[ i ] );
		offset += lengths[ i ];
	}

	va_end( args );

	out[ totalLen ] = '\0';

	outString.data  = out;
	outString.size  = totalLen;

	str_track_realloc( STR_FILE_LINE_INT outString.data, outString.size + 1, data );

	return outString;
}


ch_string ch_str_join_list( STR_FILE_LINE_DEF char* data, std::initializer_list< const char* > strings )
{
	ch_string outString;
	outString.data = nullptr;
	outString.size = 0;

	size_t  totalLen  = 0;
	size_t* lengths   = ch_malloc< size_t >( strings.size() );

	if ( lengths == nullptr )
	{
		printf( "Failed to allocate %zu bytes for string length array!\n", strings.size() * sizeof( size_t ) );
		return outString;
	}

	size_t i = 0;
	for ( const char* arg : strings )
	{
		if ( arg == nullptr )
		{
			Log_WarnF( "Attempted to concatenate nullptr string at index %d!\n", i );
			lengths[ i ] = 0;
			continue;
		}

		lengths[ i ] = strlen( arg );
		totalLen += lengths[ i ];
		i++;
	}

	char* out = ch_realloc< char >( data, totalLen + 1 );

	if ( out == nullptr )
	{
		printf( "Failed to allocate %zu bytes for string!\n", ( totalLen + 1 ) * sizeof( char ) );
		ch_free( lengths );
		return outString;
	}

	size_t offset = 0;
	i = 0;
	for ( const char* arg : strings )
	{
		if ( arg == nullptr )
			continue;

		memcpy( out + offset, arg, lengths[ i ] );
		offset += lengths[ i ];
		i++;
	}

	out[ totalLen ] = '\0';

	outString.data  = out;
	outString.size  = totalLen;

	str_track_realloc( STR_FILE_LINE_INT outString.data, outString.size + 1, data );

	return outString;
}


ch_string ch_str_join_list( STR_FILE_LINE_DEF char* data, std::initializer_list< ch_string > strings )
{
	ch_string outString;
	outString.data = nullptr;
	outString.size = 0;

	size_t  totalLen  = 0;

	size_t i = 0;
	for ( const ch_string& arg : strings )
	{
		if ( arg.data == nullptr )
		{
			Log_WarnF( "Attempted to concatenate nullptr string at index %d!\n", i );
			continue;
		}

		totalLen += arg.size;
		i++;
	}

	char* out = ch_realloc< char >( data, totalLen + 1 );

	if ( out == nullptr )
	{
		printf( "Failed to allocate %zu bytes for string!\n", ( totalLen + 1 ) * sizeof( char ) );
		return outString;
	}

	size_t offset = 0;
	i = 0;
	for ( const ch_string& arg : strings )
	{
		memcpy( out + offset, arg.data, arg.size );
		offset += arg.size;
		i++;
	}

	out[ totalLen ] = '\0';

	outString.data  = out;
	outString.size  = totalLen;

	str_track_realloc( STR_FILE_LINE_INT outString.data, outString.size + 1, data );

	return outString;
}


// --------------------------------------------------------------------------
// Free Strings


void ch_str_free( char* string )
{
	if ( !string )
		return;

	str_track_free( string );
	ch_free( string );
}


void ch_str_free( ch_string string )
{
	if ( !string.data )
		return;

	str_track_free( string.data );
	ch_free( string.data );
}


void ch_str_free( char** strings, size_t count )
{
	for ( size_t i = 0; i < count; i++ )
	{
		ch_str_free( strings[ i ] );
	}
}


void ch_str_free( ch_string* strings, size_t count )
{
	for ( size_t i = 0; i < count; i++ )
	{
		ch_str_free( strings[ i ].data );
	}
}


void ch_str_free( std::vector< ch_string >& strings )
{
	for ( ch_string& string : strings )
		ch_str_free( string.data );

	strings.clear();
}


// --------------------------------------------------------------------------
// Manual String Allocation Tracking


void ch_str_add( STR_FILE_LINE_DEF const char* string )
{
	if ( string == nullptr )
	{
		print( "Attempted to add nullptr string to tracking!\n" );
		return;
	}

	size_t len = strlen( string );

	if ( len == 0 )
	{
		print( "Attempted to add empty string to tracking!\n" );
		return;
	}

	str_track_alloc( STR_FILE_LINE_INT string, len );
}


void ch_str_add( STR_FILE_LINE_DEF const char* string, size_t len )
{
	str_track_alloc( STR_FILE_LINE_INT string, len );
}


void ch_str_add( STR_FILE_LINE_DEF const ch_string& string )
{
	str_track_alloc( STR_FILE_LINE_INT string.data, string.size );
}


void ch_str_remove( const char* string )
{
	str_track_free( string );
}


size_t ch_str_get_alloc_count()
{
#if CH_STRING_MEM_TRACKING
	return (size_t)str_track_get().size();
#else
	return 0;
#endif
}


size_t ch_str_get_alloc_size()
{
#if CH_STRING_MEM_TRACKING
	size_t size = 0;

	for ( auto& [ ptr, track_data ] : str_track_get() )
	{
		size += track_data.size;
	}

	return size;
#else
	return 0;
#endif
}


void ch_str_free_all()
{
#if CH_STRING_MEM_TRACKING
	ch_string_track_map& strings = str_track_get();
	u32                  size    = strings.size();

	if ( size == 0 )
		return;

	printf( " *** WARNING: %d STRINGS NOT FREED ON SHUTDOWN!\n", size );

	for ( auto& [ ptr, track_data ] : strings )
	{
		ch_free( ptr );
	}

	strings.clear();
#endif
}


// ===========================================================================================
// String Equality


bool ch_str_equals_base( const char* s1, const char* s2, size_t len )

{
#if 01
	const char*       cur1 = s1;
	const char*       cur2 = s2;
	const char* const end  = len + s1;

	for ( ; cur1 < end; ++cur1, ++cur2 )
	{
		if ( *cur1 != *cur2 )
			return false;
	}

	return true;
#else
	return strncmp( s1, s2, len ) == 0;
#endif
}


// would strcmp just be faster in this case?
bool ch_str_equals( const char* str1, const char* str2 )
{
	if ( str1 == nullptr || str2 == nullptr )
		return false;

	// is this the same pointer?
	if ( str1 == str2 )
		return true;

	size_t str1Len = strlen( str1 );
	size_t str2Len = strlen( str2 );

	if ( str1Len != str2Len )
		return false;

	return ch_str_equals_base( str1, str2, str1Len );
}


bool ch_str_equals( const char* str1, const char* str2, size_t count )
{
	if ( str1 == nullptr || str2 == nullptr )
		return false;

	// is this the same pointer?
	if ( str1 == str2 )
		return true;

	size_t str1Len = strlen( str1 );

	if ( str1Len != count )
		return false;

	return ch_str_equals_base( str1, str2, count );
}


bool ch_str_equals( const char* str1, size_t str1Len, const char* str2 )
{
	// TODO: technically, if both string lengths are 0, they are equal
	if ( str1 == nullptr || str2 == nullptr /*|| str1Len == 0*/ )
		return false;

	// is this the same pointer?
	if ( str1 == str2 )
		return true;

	size_t str2Len = strlen( str2 );

	if ( str1Len != str2Len )
		return false;

	return ch_str_equals_base( str1, str2, str1Len );
}


bool ch_str_equals( const char* str1, size_t str1Len, const char* str2, size_t str2Len )
{
	if ( str1 == nullptr || str2 == nullptr || str1Len != str2Len )
		return false;

	// is this the same pointer?
	if ( str1 == str2 )
		return true;

	// both are 0
	if ( str1Len == 0 )
		return true;

	return ch_str_equals_base( str1, str2, str1Len );
}


bool ch_str_equals( const ch_string& str1, const char* str2 )
{
	return ch_str_equals( str1.data, str1.size, str2 );
}


bool ch_str_equals( const ch_string& str1, const char* str2, size_t str2Len )
{
	return ch_str_equals( str1.data, str1.size, str2, str2Len );
}


bool ch_str_equals( const ch_string& str1, const ch_string& str2 )
{
	return ch_str_equals( str1.data, str1.size, str2.data, str1.size );
}


bool ch_str_equals( const ch_string_auto& str1, const char* str2 )
{
	return ch_str_equals( str1.data, str1.size, str2 );
}


bool ch_str_equals( const ch_string_auto& str1, const char* str2, size_t str2Len )
{
	return ch_str_equals( str1.data, str1.size, str2, str2Len );
}


bool ch_str_equals( const ch_string_auto& str1, const ch_string_auto& str2 )
{
	return ch_str_equals( str1.data, str1.size, str2.data, str2.size );
}


bool ch_str_equals( const ch_string& str1, const ch_string_auto& str2 )
{
	return ch_str_equals( str1.data, str1.size, str2.data, str2.size );
}


// --------------------------------------------------------------------------
// Case insensitive comparison


CORE_API bool ch_str_case_equals( const char* str1, size_t str1Len, const char* str2, size_t str2Len )
{
	if ( str1Len != str2Len )
		return false;

	return ch_strncasecmp( str1, str2, str1Len ) == 0;
}


// --------------------------------------------------------------------------


bool ch_str_equals_any( const ch_string& str1, size_t count, const char** strings )
{
	if ( !strings )
		return false;

	for ( size_t i = 0; i < count; i++ )
	{
		if ( ch_str_equals( str1.data, str1.size, strings[ i ] ) )
			return true;
	}

	return false;
}


bool ch_str_equals_any( const char* str1, size_t strLen, size_t count, const char** strings )
{
	if ( !strings )
		return false;

	for ( size_t i = 0; i < count; i++ )
	{
		if ( ch_str_equals( str1, strLen, strings[ i ] ) )
			return true;
	}

	return false;
}


bool ch_str_equals_any( const char* str1, size_t strLen, size_t count, const char** strings, size_t* lengths )
{
	if ( !lengths )
		return false;

	if ( !strings )
		return false;

	for ( size_t i = 0; i < count; i++ )
	{
		if ( ch_str_equals( str1, strLen, strings[ i ], lengths[ i ] ) )
			return true;
	}

	return false;
}


bool ch_str_equals_any( const ch_string& str1, size_t count, const char** strings, size_t* lengths )
{
	if ( !lengths )
		return false;

	if ( !strings )
		return false;

	for ( size_t i = 0; i < count; i++ )
	{
		if ( ch_str_equals( str1.data, str1.size, strings[ i ], lengths[ i ] ) )
			return true;
	}

	return false;
}


bool ch_str_equals_any( const ch_string& str1, size_t count, const ch_string* strings )
{
	if ( !strings )
		return false;

	for ( size_t i = 0; i < count; i++ )
	{
		if ( ch_str_equals( str1.data, str1.size, strings[ i ].data, strings[ i ].size ) )
			return true;
	}

	return false;
}


// ----------------------------------------------------------------------------------------
// Comparison
// ----------------------------------------------------------------------------------------


bool ch_str_compare( const ch_string& s1, const ch_string& s2 )
{
	if ( s1.size != s2.size )
		return 1;

	return ch_str_equals_base( s1.data, s2.data, s1.size );
	//	return strncmp( s1.data, s2.data, s1.size );
}


bool ch_str_compare( const ch_string& s1, const char* s2 )
{
	if ( !s2 )
		return 1;

	size_t s2_len = strlen( s2 );

	if ( s1.size != s2_len )
		return 1;

	return ch_str_equals_base( s1.data, s2, s1.size );
	//	return strncmp( s1.data, s2, s1.size );
}


bool ch_str_compare( const ch_string& s1, const char* s2, size_t len )
{
	if ( !s2 )
		return 1;

	if ( s1.size != len )
		return 1;

	return ch_str_equals_base( s1.data, s2, len );
	//	return strncmp( s1.data, s2, len );
}


// ----------------------------------------------------------------------------------------
// Appending Strings Together
// ----------------------------------------------------------------------------------------

/*
bool ch_str_append( ch_string& dest, const ch_string& src )
{
	size_t oldSize = dest.size;
	dest.size += src.size;

	// ReallocString adds the extra bytes for the null terminator
	char* newData = ch_str_realloc( dest.data, dest.size );

	if ( !newData )
		return false;

	dest.data = newData;
	memcpy( dest.data + oldSize, src.data, src.size );
	dest.data[ dest.size ] = '\0';
	return true;
}


bool ch_str_append( ch_ustring& dest, const ch_ustring& src )
{
	size_t oldSize = dest.size;
	dest.size += src.size;

	uchar* newData = ch_str_realloc( dest.data, dest.size );

	if ( !newData )
		return false;

	dest.data = newData;
	memcpy( dest.data + oldSize, src.data, src.size );
	dest.data[ dest.size ] = USTR('\0');
	return true;
}


bool ch_str_append( ch_string& dest, const char* src, size_t len )
{
	size_t oldSize = dest.size;
	dest.size += len;

	char* newData = ch_str_realloc( dest.data, dest.size );

	if ( !newData )
		return false;

	dest.data = newData;
	memcpy( dest.data + oldSize, src, len );
	dest.data[ dest.size ] = '\0';
	return true;
}


bool ch_str_append( ch_ustring& dest, const uchar* src, u32 len )
{
	size_t oldSize = dest.size;
	dest.size += len;

	uchar* newData = ch_str_realloc( dest.data, dest.size );

	if ( !newData )
		return false;

	dest.data = newData;
	memcpy( dest.data + oldSize, src, len );
	dest.data[ dest.size ] = USTR('\0');
	return true;
}


bool ch_str_append( ch_string& dest, const char* src )
{
	if ( !src )
		return false;

	return ch_str_append( dest, src, strlen( src ) );
}


bool ch_str_append( ch_ustring& dest, const uchar* src )
{
	if ( !src )
		return false;

	return ch_str_append( dest, src, ch_ustrlen( src ) );
}


template< typename CH_STR, typename STR_TYPE >
bool ch_str_append_base( CH_STR& dest, const CH_STR* srcList, const size_t sCount, STR_TYPE nullTerminator )
{
	if ( srcList == nullptr )
		return false;

	if ( sCount == 0 )
		return true;

	size_t totalLen = 0;
	size_t oldLen   = dest.size;

	for ( size_t i = 0; i < sCount; i++ )
	{
		totalLen += srcList[ i ].size;
	}

	STR_TYPE* newData = ch_str_realloc( dest.data, totalLen + 1 );

	if ( !newData )
		return false;

	dest.data = newData;
	dest.size = totalLen;

	size_t offset = oldLen * sizeof( STR_TYPE );
	for ( size_t i = 0; i < sCount; i++ )
	{
		memcpy( dest.data + offset, srcList[ i ].data, srcList[ i ].size * sizeof( STR_TYPE ) );
		offset += srcList[ i ].size * * sizeof( STR_TYPE );
	}

	dest.data[ dest.size ] = nullTerminator;
	return true;
}


bool ch_str_append( ch_string& dest, const ch_string* srcList, const size_t sCount )
{
	return ch_str_append_base( dest, srcList, sCount, '\0' );
}


bool ch_str_append( ch_ustring& dest, const ch_ustring* srcList, const size_t sCount )
{
	return ch_str_append_base( dest, srcList, sCount, USTR('\0') );
}


// ----------------------------------------------------------------------------------------
// Assigning
// ----------------------------------------------------------------------------------------


template< typename CH_STR, typename STR_TYPE >
bool ch_str_assign_base( CH_STR& dest, const STR_TYPE* src, size_t len, STR_TYPE nullTerminator )
{
	if ( !src )
		return false;

	if ( len == 0 )
		return false;

	STR_TYPE* newData = ch_str_realloc( dest.data, len );

	if ( !newData )
		return false;

	dest.data = newData;
	dest.size = len;

	memcpy( dest.data, src, len );
	dest.data[ dest.size ] = nullTerminator;

	return true;
}


bool ch_str_assign( ch_string& dest, const ch_string& src )
{
	return ch_str_assign_base( dest, src.data, src.size, '\0' );
}


bool ch_str_assign( ch_ustring& dest, const ch_ustring& src )
{
	return ch_str_assign_base( dest, src.data, src.size, USTR('\0') );
}


bool ch_str_assign( ch_string& dest, const char* src )
{
	if ( !src )
		return false;

	return ch_str_assign_base( dest, src, strlen( src ), '\0' );
}


bool ch_str_assign( ch_ustring& dest, const uchar* src )
{
	if ( !src )
		return false;

	return ch_str_assign_base( dest, src, ch_ustrlen( src ), USTR('\0') );
}


bool ch_str_assign( ch_string& dest, const char* src, size_t len )
{
	return ch_str_assign_base( dest, src, len, '\0' );
}


bool ch_str_assign( ch_ustring& dest, const uchar* src, u32 len )
{
	return ch_str_assign_base( dest, src, len, USTR('\0') );
}


// ----------------------------------------------------------------------------------------
// Conversion
// ----------------------------------------------------------------------------------------


// Convert Between Unicode and ANSI
CORE_API bool ch_str_convert( ch_string& dest, const ch_ustring& src )
{
	char* out = Sys_ToMultiByte( src.data, src.size );
	
	if ( !out )
		return false;

	dest.size = src.size;
	dest.data = out;

	return true;
}


CORE_API bool ch_str_convert( ch_ustring& dest, const ch_string& src )
{
	uchar* out = Sys_ToWideChar( src.data, src.size );

	if ( !out )
		return false;

	dest.size = src.size;
	dest.data = out;

	return true;
}
*/



bool ch_str_starts_with( const char* s, size_t len, const char* start )
{
	if ( !s || !start )
		return false;

	size_t startLen = strlen( start );

	if ( startLen == 0 )
		return false;

	if ( len == 0 )
		len = strlen( s );

	if ( len < startLen )
		return false;

	return ch_str_equals_base( s, start, startLen );
}


bool ch_str_starts_with( const char* s, size_t len, const char* start, size_t startLen )
{
	if ( !s || !start )
		return false;

	if ( startLen == 0 )
		return false;

	if ( len == 0 )
		len = strlen( s );

	if ( len < startLen )
		return false;

	return ch_str_equals_base( s, start, startLen );
}

bool ch_str_starts_with( const char* s, const char* start )
{
	if ( !s || !start )
		return false;

	size_t startLen = strlen( start );

	if ( startLen == 0 )
		return false;

	return ch_str_equals_base( s, start, startLen );
}


bool ch_str_starts_with( const char* s, const char* start, size_t startLen )
{
	if ( !s || !start )
		return false;

	if ( startLen == 0 )
		return false;

	return ch_str_equals_base( s, start, startLen );
}


bool ch_str_starts_with( const ch_string& s, const char* start )
{
	return ch_str_starts_with( s.data, s.size, start );
}


bool ch_str_starts_with( const ch_string& s, const char* start, size_t startLen )
{
	return ch_str_starts_with( s.data, s.size, start, startLen );
}


bool ch_str_starts_with( const ch_string& s, const ch_string& start )
{
	return ch_str_starts_with( s.data, s.size, start.data, start.size );
}


// ----------------------------------------------------------------------------------------


bool ch_str_ends_with( const char* s, size_t len, const char* end )
{
	if ( !s || !end )
		return false;

	size_t endLen = strlen( end );

	if ( len == 0 )
		len = strlen( s );

	if ( len < endLen )
		return false;

	return ch_str_equals_base( s + len - endLen, end, endLen );
}


bool ch_str_ends_with( const char* s, size_t len, const char* end, size_t endLen )
{
	if ( !s || !end )
		return false;

	if ( len == 0 )
		len = strlen( s );

	if ( len == 0 )
		return false;

	if ( endLen == 0 )
		endLen = strlen( end );

	if ( endLen == 0 )
		return false;

	if ( len < endLen )
		return false;

	return ch_str_equals_base( s + len - endLen, end, endLen );
}


bool ch_str_ends_with( const char* s, const char* end )
{
	if ( !s || !end )
		return false;

	size_t len = strlen( s );
	size_t endLen = strlen( end );

	if ( len < endLen )
		return false;

	return ch_str_equals_base( s + len - endLen, end, endLen );
}


bool ch_str_ends_with( const char* s, const char* end, size_t endLen )
{
	if ( !s || !end )
		return false;

	size_t len = strlen( s );

	if ( len < endLen )
		return false;

	return ch_str_equals_base( s + len - endLen, end, endLen );
}


bool ch_str_ends_with( const ch_string& s, const char* end )
{
	return ch_str_ends_with( s.data, s.size, end );
}


bool ch_str_ends_with( const ch_string& s, const char* end, size_t endLen )
{
	return ch_str_ends_with( s.data, s.size, end, endLen );
}


bool ch_str_ends_with( const ch_string& s, const ch_string& end )
{
	return ch_str_ends_with( s.data, s.size, end.data, end.size );
}


// ----------------------------------------------------------------------------------------


// returns the index of the first instance of find in s
// returns SIZE_MAX if not found
size_t ch_str_contains_base( const char* s, size_t len, const char* find, size_t findLen )
{
	if ( !s || !find || len == 0 || findLen == 0 || len < findLen )
		return SIZE_MAX;

	for ( size_t i = 0; i < len - findLen; i++ )
	{
		if ( ch_str_equals_base( s + i, find, findLen ) )
			return i;
	}

	return SIZE_MAX;
}


size_t ch_str_contains( const char* s, size_t len, const char* find, size_t findLen )
{
	if ( !s || !find )
		return SIZE_MAX;

	if ( len == 0 )
		len = strlen( s );

	if ( len == 0 )
		return SIZE_MAX;

	if ( findLen == 0 )
		findLen = strlen( find );

	return ch_str_contains_base( s, len, find, findLen );
}


size_t ch_str_contains( const char* s, size_t len, const char* find )
{
	if ( !s || !find )
		return SIZE_MAX;

	size_t findLen = strlen( find );

	return ch_str_contains_base( s, len, find, findLen );
}


size_t ch_str_contains( const char* s, const char* find )
{
	if ( !s || !find )
		return SIZE_MAX;

	size_t len = strlen( s );
	size_t findLen = strlen( find );

	return ch_str_contains_base( s, len, find, findLen );
}


size_t ch_str_contains( const char* s, const char* find, size_t findLen )
{
	if ( !s || !find )
		return SIZE_MAX;

	size_t len = strlen( s );

	return ch_str_contains_base( s, len, find, findLen );
}


// ----------------------------------------------------------------------------------------
// Hash
// ----------------------------------------------------------------------------------------


// https://stackoverflow.com/a/7666577
// http://www.cse.yorku.ca/~oz/hash.html
u64 ch_str_hash( const char* str, size_t len )
{
	u64 hash = 5381;
	int           c;

	while ( c = *str++ )
		hash = ( ( hash << 5 ) + hash ) + c; /* hash * 33 + c */

	return hash;
}


// ----------------------------------------------------------------------------------------
// ConCommands
// ----------------------------------------------------------------------------------------


CONCMD( ch_dump_string_allocations )
{
#if CH_STRING_MEM_TRACKING
	u32 size = str_track_get().size();

	if ( size == 0 )
	{
		// this is most likely never going to hit
		Log_Msg( "No string allocations to dump!\n" );
		return;
	}

	ch_string_track_map& trackedStrings = str_track_get();

	// hopefully this should never hit SIZE_MAX
	// how would you even allocate that much memory?
	size_t                  totalSize      = 0;

	for ( const auto& [ ptr, track_data ] : trackedStrings )
	{
		// add to total string size
		totalSize += track_data.size;
	}

	Log_MsgF( "%d Strings Allocated - %.6f KB\n", size, ch_bytes_to_kb( totalSize ) );
#else
	Log_Msg( "String allocation tracking is disabled!\n" );
#endif
}


CONCMD( ch_dump_string_allocations_extended )
{
#if CH_STRING_MEM_TRACKING
	u32 size = str_track_get().size();

	if ( size == 0 )
	{
		// this is most likely never going to hit
		Log_Msg( "No string allocations to dump!\n" );
		return;
	}

	Log_MsgF( "Dumping %d string allocations:\n\n", size );

	ch_string_track_map& trackedStrings = str_track_get();

	// hopefully this should never hit SIZE_MAX
	// how would you even allocate that much memory?
	size_t                  totalSize      = 0;

	for ( const auto& [ ptr, track_data ] : trackedStrings )
	{
		// add to total string size
		totalSize += track_data.size;

		Log_MsgF(
			ANSI_CLR_CYAN "%s" ANSI_CLR_DEFAULT ":" ANSI_CLR_DARK_YELLOW "%d " ANSI_CLR_DARK_GREEN "%s - %zu bytes\n" ANSI_CLR_DEFAULT "\"%s\"\n",
			track_data.file, track_data.line, track_data.func, track_data.size, ptr
		);
	}

	Log_MsgF( "\nDumped %d string allocations\n", size );
	Log_MsgF( "Total string memory: %.6f KB\n", ch_bytes_to_kb( totalSize ) );
#else
	Log_Msg( "String allocation tracking is disabled!\n" );
#endif
}

