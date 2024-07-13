#include "string.h"
#include "core/platform.h"
#include "util.h"

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
	u64         size;
};

// using ch_string_track_map = std::unordered_map< char*, ch_string_track_data >;
using ch_string_track_map = std::map< char*, ch_string_track_data >;


ch_string_track_map& GetTrackedStrings()
{
	static ch_string_track_map trackedStrings;
	return trackedStrings;
}

static void TrackString_Alloc( const char* file, u32 line, const char* func, const char* string, u64 len )
{
	PROF_SCOPE();

	ch_string_track_data& data = GetTrackedStrings()[ (char*)string ];
	data.file                  = file;
	data.func                  = func;
	data.line                  = line;
	data.size                  = len;
}


static void TrackString_Realloc( const char* file, u32 line, const char* func, char* data, u64 len, char* oldPtr )
{
	PROF_SCOPE();

	if ( oldPtr && oldPtr != data )
	{
		auto it = GetTrackedStrings().find( oldPtr );

		if ( it != GetTrackedStrings().end() )
		{
			GetTrackedStrings().erase( it );
		}
		else
		{
			Log_ErrorF( "Failed to find old string pointer in tracking data!\n" );
		}
	}

	TrackString_Alloc( file, line, func, data, len );
}


static void TrackString_Free( const char* string )
{
	PROF_SCOPE();

	if ( string == nullptr )
	{
		Log_Error( "Attempted to free nullptr string!\n" );
		return;
	}

	if ( GetTrackedStrings().empty() )
	{
		Log_Error( "No strings tracked to free!\n" );
	}
	else
	{
		auto it = GetTrackedStrings().find( (char*)string );

		if ( it != GetTrackedStrings().end() )
		{
			GetTrackedStrings().erase( it );
		}
		else
		{
			Log_ErrorF( "Failed to find string pointer in tracking data to erase!\n" );
		}
	}
}

	#else
		#define TrackString_Alloc( string, len )
		#define TrackString_Realloc( data, len, oldPtr )
		#define TrackString_Free( string )
	#endif

#else
	#define TrackString_Alloc( string, len )
	#define TrackString_Realloc( data, len, oldPtr )
	#define TrackString_Free( string )

	#define CH_STRING_MEM_TRACKING 0
#endif


// ===========================================================================================
// Core String Allocation System
// ===========================================================================================


char* Util_AllocStringBase( const char* string, u64 len )
{
	// Check if we are already null-termianted
	//bool  nullTerminated = string[ len ] == nullTerminator;

	// u64   memSize        = nullTerminated ? len : len + 1;  // allocate extra byte for null terminator
	u64   memSize = len + 1;  // allocate extra byte for null terminator
	char* out     = ch_malloc< char >( memSize );

	if ( out == nullptr )
	{
		Log_ErrorF( "Failed to allocate %d bytes for string!\n", memSize );
		return nullptr;
	}

	memset( out, 0, memSize );

	memcpy( out, string, len );

	//if ( !nullTerminated )
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
		Log_Error( "Attempted to copy nullptr string!\n" );
		return out_string;
	}

	u64   len = strlen( string );
	char* out = Util_AllocStringBase( string, len );

	if ( out == nullptr )
		return out_string;

	TrackString_Alloc( STR_FILE_LINE_INT out, len + 1 );

	out_string.data = out;
	out_string.size = len;
	return out_string;
}


ch_string ch_str_copy( STR_FILE_LINE_DEF const char* string, u64 len )
{
	ch_string out_string;
	out_string.data = nullptr;
	out_string.size = 0;

	if ( string == nullptr )
	{
		Log_Error( "Attempted to copy nullptr string!\n" );
		return out_string;
	}

	char* out = Util_AllocStringBase( string, len );

	if ( out == nullptr )
		return out_string;

	TrackString_Alloc( STR_FILE_LINE_INT out, len + 1 );

	out_string.data = out;
	out_string.size = len;
	return out_string;
}


// --------------------------------------------------------------------------
// Reallocating Strings


// unused?
#if 0
char* Util_ReallocStringBase( char* data, const char* string, u64 len )
{
	u64   memSize = len + sizeof( char );  // allocate extra byte for null terminator
	char* out     = ch_malloc< char >( memSize );

	if ( out == nullptr )
	{
		Log_ErrorF( "Failed to allocate %d bytes for string!\n", memSize );
		return nullptr;
	}

	char* out = ch_realloc< char >( data, len + 1 );

	if ( out == nullptr )
		return nullptr;

	memcpy( out, string, len );
	out[ len ] = '\0';
}
#endif


ch_string ch_str_realloc( STR_FILE_LINE_DEF char* data, u64 len )
{
	ch_string out_string;
	out_string.data = nullptr;
	out_string.size = 0;

	char* out       = ch_realloc< char >( data, len + 1 );

	if ( out == nullptr )
		return out_string;

	TrackString_Realloc( STR_FILE_LINE_INT out, len + 1, data );

	out_string.data = out;
	out_string.size = len;
	return out_string;
}


ch_string ch_str_realloc( STR_FILE_LINE_DEF char* data, const char* string, u64 len )
{
	ch_string out_string;
	out_string.data = nullptr;
	out_string.size = 0;

	if ( string == nullptr )
	{
		Log_ErrorF( "Attempted to copy nullptr string!\n" );
		return out_string;
	}

	char* out = ch_realloc< char >( data, len + 1 );

	if ( out == nullptr )
		return out_string;

	memcpy( out, string, len );
	out[ len ] = '\0';

	TrackString_Realloc( STR_FILE_LINE_INT out, len + 1, data );

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
		Log_ErrorF( "Attempted to copy nullptr string!\n" );
		return out_string;
	}

	u64 len = strlen( string );

	if ( len == 0 )
	{
		Log_ErrorF( "Attempted to copy empty string!\n" );
		return out_string;
	}

	char* out = ch_realloc< char >( data, len + 1 );

	if ( out == nullptr )
		return out_string;

	memcpy( out, string, len );
	out[ len ] = '\0';

	TrackString_Realloc( STR_FILE_LINE_INT out, len + 1, data );

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
		Log_Error( "vstring va_args: vsnprintf failed\n" );
		return out_string;
	}

	if ( len > 0 )
	{
		result = ch_realloc< char >( data, len + 1 );

		if ( result == nullptr )
		{
			Log_ErrorF( "Failed to allocate %d bytes for string!\n", ( len + 1 ) * sizeof( char ) );
			va_end( args_copy );
			va_end( args );
			return out_string;
		}

		vsnprintf( result, len, format, args_copy );
		result[ len ] = '\0';

		TrackString_Alloc( STR_FILE_LINE_INT result, len + 1 );
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
			Log_ErrorF( "Failed to allocate %d bytes for string!\n", ( len + 1 ) * sizeof( char ) );
			return out_string;
		}

		std::vsnprintf( result, len, format, args );
		result[ len ] = '\0';

		TrackString_Alloc( STR_FILE_LINE_INT result, len + 1 );

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
		Log_Error( "vstring va_args: vsnprintf failed\n" );
		return out_string;
	}

	if ( len > 0 )
	{
		result = ch_malloc< char >( len + 1 );
		vsnprintf( result, len, format, args_copy );
		result[ len ] = '\0';

		TrackString_Alloc( STR_FILE_LINE_INT result, len + 1 );
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
		std::vsnprintf( result, len, format, args );
		result[ len ] = '\0';

		TrackString_Alloc( STR_FILE_LINE_INT result, len + 1 );

		out_string.data = result;
		out_string.size = len;
		return out_string;
	}

	return out_string;
}


// --------------------------------------------------------------------------
// Basic String Joining
// TODO: there's a lot of duplicated code here, need to refactor


ch_string ch_str_concat( STR_FILE_LINE_DEF u64 count, const char** strings, char* data )
{
	ch_string outString;
	outString.data = nullptr;
	outString.size = 0;

	u64  totalLen  = 0;

	u64* lengths   = ch_malloc< u64 >( count );

	if ( lengths == nullptr )
	{
		Log_ErrorF( "Failed to allocate %d bytes for string length array!\n", count * sizeof( u64 ) );
		return outString;
	}

	for ( u64 i = 0; i < count; i++ )
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

	for ( u64 i = 0; i < count && data; i++ )
	{
		if ( strings[ i ] == data )
		{
			// Technically, we could allocate a new string here and copy the data into it
			// im not sure if that would be a good idea or not
			Log_Error( "Attempted to concatenate string with itself!\n" );
			ch_free( lengths );
			return outString;
		}
	}

	char* out = ch_realloc< char >( data, totalLen + 1 );

	if ( out == nullptr )
	{
		Log_ErrorF( "Failed to allocate %d bytes for string!\n", ( totalLen + 1 ) * sizeof( char ) );
		ch_free( lengths );
		return outString;
	}

	u64 offset = 0;
	for ( u64 i = 0; i < count; i++ )
	{
		if ( lengths[ i ] == 0 || !strings[ i ] )
			continue;

		memcpy( out + offset, strings[ i ], lengths[ i ] );
		offset += lengths[ i ];
	}

	out[ totalLen ] = '\0';

	TrackString_Realloc( STR_FILE_LINE_INT out, totalLen + 1, data );

	ch_free( lengths );

	outString.data = out;
	outString.size = totalLen;
	return outString;
}


ch_string ch_str_concat( STR_FILE_LINE_DEF u64 count, const char** strings, const u64* lengths, char* data )
{
	ch_string outString;
	outString.data = nullptr;
	outString.size = 0;

	u64 totalLen   = 0;

	for ( u64 i = 0; i < count; i++ )
		totalLen += lengths[ i ];

	char* out = ch_realloc< char >( data, totalLen + 1 );

	if ( out == nullptr )
	{
		Log_ErrorF( "Failed to allocate %d bytes for string!\n", ( totalLen + 1 ) * sizeof( char ) );
		return outString;
	}

	u64 offset = 0;

#if 1
	for ( u64 i = 0; i < count; i++ )
	{
		memcpy( out + offset, strings[ i ], lengths[ i ] );
		offset += lengths[ i ];
	}
#elif 0  // slightly slower than memcpy
	for ( u64 i = 0; i < count; i++ )
	{
		char* outOffset = out + offset;
		for ( u64 j = 0; j < lengths[ i ]; j++ )
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
	u64   remaining = count;
	while ( remaining-- )
	{
		u64   strRemain = *lengths;
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

	TrackString_Realloc( STR_FILE_LINE_INT out, totalLen + 1, data );

	outString.data = out;
	outString.size = totalLen;
	return outString;
}


ch_string ch_str_concat( STR_FILE_LINE_DEF u64 count, std::vector< const char* >& strings, const std::vector< u64 >& lengths, char* data )
{
	return ch_str_concat( STR_FILE_LINE_INT count, strings.data(), lengths.data(), data );
}


ch_string ch_str_concat( STR_FILE_LINE_DEF u64 count, const ch_string* strings, char* data )
{
	ch_string outString;
	outString.data = nullptr;
	outString.size = 0;

	u64 totalLen   = 0;

	for ( u64 i = 0; i < count; i++ )
		totalLen += strings[ i ].size;

	char* out = ch_realloc< char >( data, totalLen + 1 );

	if ( out == nullptr )
	{
		Log_ErrorF( "Failed to allocate %d bytes for string!\n", ( totalLen + 1 ) * sizeof( char ) );
		return outString;
	}

	u64 offset = 0;

	for ( u64 i = 0; i < count; i++ )
	{
		memcpy( out + offset, strings[ i ].data, strings[ i ].size );
		offset += strings[ i ].size;
	}

	out[ totalLen ] = '\0';

	TrackString_Realloc( STR_FILE_LINE_INT out, totalLen + 1, data );

	outString.data = out;
	outString.size = totalLen;
	return outString;
}


// --------------------------------------------------------------------------


ch_string ch_str_join( STR_FILE_LINE_DEF u64 count, char** strings, const char* space, char* data )
{
	ch_string outString;
	outString.data = nullptr;
	outString.size = 0;

	if ( !space )
		space = " ";

	u64  totalLen = 0;

	u64* lengths  = ch_malloc< u64 >( count );

	if ( lengths == nullptr )
	{
		Log_ErrorF( "Failed to allocate %d bytes for string length array!\n", count * sizeof( u64 ) );
		return outString;
	}

	for ( u64 i = 0; i < count; i++ )
	{
		lengths[ i ] = strlen( strings[ i ] );
		totalLen += lengths[ i ];
	}

	u64 spaceLen = strlen( space );
	totalLen += ( count - 1 ) * spaceLen;

	char* out = ch_realloc< char >( data, totalLen + 1 );

	if ( out == nullptr )
	{
		Log_ErrorF( "Failed to allocate %d bytes for string!\n", ( totalLen + 1 ) * sizeof( char ) );
		return outString;
	}

	u64 offset = 0;
	for ( u64 i = 0; i < count; i++ )
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

	TrackString_Realloc( STR_FILE_LINE_INT out, totalLen + 1, data );

	outString.data = out;
	outString.size = totalLen;
	return outString;
}


ch_string ch_str_join( STR_FILE_LINE_DEF u64 count, char** strings, const u64* lengths, const char* space, char* data )
{
	ch_string outString;
	outString.data = nullptr;
	outString.size = 0;

	if ( !space )
		space = " ";

	u64 totalLen = 0;
	for ( u64 i = 0; i < count; i++ )
		totalLen += lengths[ i ];

	u64 spaceLen = strlen( space );
	totalLen += ( count - 1 ) * spaceLen;

	char* out = ch_realloc< char >( data, totalLen + 1 );

	if ( out == nullptr )
	{
		Log_ErrorF( "Failed to allocate %d bytes for string!\n", ( totalLen + 1 ) * sizeof( char ) );
		return outString;
	}

	u64 offset = 0;
	for ( u64 i = 0; i < count; i++ )
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

	TrackString_Realloc( STR_FILE_LINE_INT out, totalLen + 1, data );

	outString.data = out;
	outString.size = totalLen;
	return outString;
}


ch_string ch_str_join( STR_FILE_LINE_DEF u64 count, const ch_string* strings, const char* space, char* data )
{
	ch_string outString;
	outString.data = nullptr;
	outString.size = 0;

	if ( !space )
		space = " ";

	u64 totalLen = 0;
	for ( u64 i = 0; i < count; i++ )
		totalLen += strings[ i ].size;

	u64 spaceLen = strlen( space );
	totalLen += ( count - 1 ) * spaceLen;

	char* out = ch_realloc< char >( data, totalLen + 1 );

	if ( out == nullptr )
	{
		Log_ErrorF( "Failed to allocate %d bytes for string!\n", ( totalLen + 1 ) * sizeof( char ) );
		return outString;
	}

	u64 offset = 0;
	for ( u64 i = 0; i < count; i++ )
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

	TrackString_Realloc( STR_FILE_LINE_INT out, totalLen + 1, data );

	outString.data = out;
	outString.size = totalLen;
	return outString;
}


template< typename CH_STRING, typename STR_TYPE >
CH_STRING Util_AllocStringConcatBase( STR_TYPE* data, u64 count, const STR_TYPE* string, va_list args, u64 ( *strlen_func )( const STR_TYPE* ) )
{
	CH_STRING outString;
	outString.data = nullptr;
	outString.size = 0;

	u64  totalLen  = 0;
	u64* lengths   = ch_malloc< u64 >( count );

	if ( lengths == nullptr )
	{
		Log_ErrorF( "Failed to allocate %d bytes for string length array!\n", count * sizeof( u64 ) );
		return outString;
	}

	va_list args_copy;
	va_copy( args_copy, args );

	for ( u64 i = 0; i < count; i++ )
	{
		const char* arg = va_arg( args_copy, const STR_TYPE* );
		lengths[ i ]    = strlen_func( arg );
		totalLen += lengths[ i ];
	}

	va_end( args_copy );

	STR_TYPE* out = ch_realloc< STR_TYPE >( data, totalLen + 1 );

	if ( out == nullptr )
	{
		Log_ErrorF( "Failed to allocate %d bytes for string!\n", ( totalLen + 1 ) * sizeof( STR_TYPE ) );
		ch_free( lengths );
		//va_end( args_copy );
		return outString;
	}

	u64 offset = 0;
	for ( u64 i = 0; i < count; i++ )
	{
		const STR_TYPE* arg = va_arg( args, const STR_TYPE* );
		memcpy( out + offset, arg, lengths[ i ] );
		offset += lengths[ i ];
	}

	out[ totalLen ] = '\0';

	outString.data  = out;
	outString.size  = totalLen;

	return outString;
}


#if 0
char* ch_str_concat( char* data, u64 count, const char* string, ... )
{
	va_list args;
	va_start( args, string );

	#if CH_STRING_MEM_TRACKING
	u64 index = 0;

	if ( data )
		index = GetAllocatedStrings().index( data );
	#endif

	ch_string out = Util_AllocStringConcatBase< ch_string >( data, count, string, args, strlen );

	#if CH_STRING_MEM_TRACKING
	Util_AllocStringConcatTrack( out.data, index );
	#endif

	va_end( args );
	return out.data;
}
#endif


ch_string ch_str_concat( STR_FILE_LINE_DEF char* data, u64 count, const char* string, ... )
{
	va_list args;
	va_start( args, string );

	ch_string out = Util_AllocStringConcatBase< ch_string >( data, count, string, args, strlen );

	TrackString_Realloc( STR_FILE_LINE_INT out.data, out.size + 1, data );

	va_end( args );
	return out;
}


// --------------------------------------------------------------------------
// Free Strings


void ch_str_free( char* string )
{
	if ( string == nullptr )
	{
		Log_Error( "Attempted to free nullptr string!\n" );
		return;
	}

	TrackString_Free( string );
	ch_free( string );
}


void ch_str_free( char** strings, u64 count )
{
	for ( u64 i = 0; i < count; i++ )
	{
		ch_str_free( strings[ i ] );
	}
}


void ch_str_free( ch_string* strings, u64 count )
{
	for ( u64 i = 0; i < count; i++ )
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
		Log_Error( "Attempted to add nullptr string to tracking!\n" );
		return;
	}

	u64 len = strlen( string );

	if ( len == 0 )
	{
		Log_Error( "Attempted to add empty string to tracking!\n" );
		return;
	}

	TrackString_Alloc( STR_FILE_LINE_INT string, len );
}


void ch_str_add( STR_FILE_LINE_DEF const char* string, u64 len )
{
	TrackString_Alloc( STR_FILE_LINE_INT string, len );
}


void ch_str_add( STR_FILE_LINE_DEF const ch_string& string )
{
	TrackString_Alloc( STR_FILE_LINE_INT string.data, string.size );
}


void ch_str_remove( const char* string )
{
	TrackString_Free( string );
}


u64 ch_str_get_alloc_count()
{
#if CH_STRING_MEM_TRACKING
	return (u64)GetTrackedStrings().size();
#else
	return 0;
#endif
}


u64 ch_str_get_alloc_size()
{
#if CH_STRING_MEM_TRACKING
	u64 size = 0;

	for ( auto& [ ptr, track_data ] : GetTrackedStrings() )
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
	ch_string_track_map& strings = GetTrackedStrings();
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


bool ch_str_equals_base( const char* s1, const char* s2, u64 len )
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

	u64 str1Len = strlen( str1 );
	u64 str2Len = strlen( str2 );

	if ( str1Len != str2Len )
		return false;

	return ch_str_equals_base( str1, str2, str1Len );
}


bool ch_str_equals( const char* str1, const char* str2, u64 count )
{
	if ( str1 == nullptr || str2 == nullptr )
		return false;

	u64 str1Len = strlen( str1 );

	if ( str1Len != count )
		return false;

	return ch_str_equals_base( str1, str2, count );
}


bool ch_str_equals( const char* str1, u64 str1Len, const char* str2 )
{
	// TODO: technically, if both string lengths are 0, they are equal
	if ( str1 == nullptr || str2 == nullptr /*|| str1Len == 0*/ )
		return false;

	u64 str2Len = strlen( str2 );

	if ( str1Len != str2Len )
		return false;

	return ch_str_equals_base( str1, str2, str1Len );
}


bool ch_str_equals( const char* str1, u64 str1Len, const char* str2, u64 str2Len )
{
	if ( str1 == nullptr || str2 == nullptr || str1Len != str2Len )
		return false;

	// both are 0
	if ( str1Len == 0 )
		return true;

	return ch_str_equals_base( str1, str2, str1Len );
}


bool ch_str_equals( const ch_string& str1, const char* str2 )
{
	return ch_str_equals( str1.data, str1.size, str2 );
}


bool ch_str_equals( const ch_string& str1, const char* str2, u64 str2Len )
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


bool ch_str_equals( const ch_string_auto& str1, const char* str2, u64 str2Len )
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


bool ch_str_equals_any( const ch_string& str1, u64 count, const char** strings )
{
	if ( !strings )
		return false;

	for ( u64 i = 0; i < count; i++ )
	{
		if ( ch_str_equals( str1.data, str1.size, strings[ i ] ) )
			return true;
	}

	return false;
}


bool ch_str_equals_any( const char* str1, u64 strLen, u64 count, const char** strings )
{
	if ( !strings )
		return false;

	for ( u64 i = 0; i < count; i++ )
	{
		if ( ch_str_equals( str1, strLen, strings[ i ] ) )
			return true;
	}

	return false;
}


bool ch_str_equals_any( const char* str1, u64 strLen, u64 count, const char** strings, u64* lengths )
{
	if ( !lengths )
		return false;

	if ( !strings )
		return false;

	for ( u64 i = 0; i < count; i++ )
	{
		if ( ch_str_equals( str1, strLen, strings[ i ], lengths[ i ] ) )
			return true;
	}

	return false;
}


bool ch_str_equals_any( const ch_string& str1, u64 count, const char** strings, u64* lengths )
{
	if ( !lengths )
		return false;

	if ( !strings )
		return false;

	for ( u64 i = 0; i < count; i++ )
	{
		if ( ch_str_equals( str1.data, str1.size, strings[ i ], lengths[ i ] ) )
			return true;
	}

	return false;
}


bool ch_str_equals_any( const ch_string& str1, u64 count, const ch_string* strings )
{
	if ( !strings )
		return false;

	for ( u64 i = 0; i < count; i++ )
	{
		if ( ch_str_equals( str1.data, str1.size, strings[ i ].data, strings[ i ].size ) )
			return true;
	}

	return false;
}


#if 0  // _WIN32
bool ch_streq( const wchar_t* str1, const wchar_t* str2 )
{
	if ( str1 == nullptr || str2 == nullptr )
		return false;

	u64 len = wcslen( str1 );

	// if ( len != wcslen( str2 ) )
	// 	return false;

	return ch_streq_base( str1, str2, len );
}


bool ch_strneq( const wchar_t* str1, const wchar_t* str2, u64 count )
{
	if ( str1 == nullptr || str2 == nullptr )
		return false;

	return ch_streq_base( str1, str2, count );
}


bool ch_streq( const char* str1, const wchar_t* str2 )
{
	if ( str1 == nullptr || str2 == nullptr )
		return false;

	u64 len = strlen( str1 );

	// if ( len != wcslen( str2 ) )
	// 	return false;

	wchar_t* wStr1 = Sys_ToWideChar( str1, len );

	bool ret = ch_streq_base( wStr1, str2, len );

	ch_free( wStr1 );

	return ret;
}


bool ch_strneq( const char* str1, const wchar_t* str2, u64 count )
{
	if ( str1 == nullptr || str2 == nullptr )
		return false;

	wchar_t* wStr1 = Sys_ToWideChar( str1, count );

	bool ret = ch_streq_base( wStr1, str2, count );

	ch_free( wStr1 );

	return ret;
}


bool ch_streq( const wchar_t* str1, const char* str2 )
{
	return ch_streq( str2, str1 );
}


bool ch_strneq( const wchar_t* str1, const char* str2, u64 count )
{
	return ch_strneq( str2, str1, count );
}
#endif


// ----------------------------------------------------------------------------------------
// Creation and Deletion
// ----------------------------------------------------------------------------------------


template <typename CH_STR_TYPE, typename STR_TYPE>
bolean ch_str_create_base( CH_STR_TYPE& s, const STR_TYPE* str, const size_t strLen )
{
	s = ch_str_copy( STR_FILE_LINE str, strLen );

	if ( !s.data )
		return false;

	return true;
}


CORE_API bool ch_str_create( ch_string& s, const char* str )
{
	if ( !str )
		return false;

	return ch_str_create_base( s, str, strlen( str ) );
}

/*
CORE_API bool ch_str_create( ch_ustring& s, const uchar* str )
{
	if ( !str )
		return false;

	return ch_str_create_base( s, str, ch_ustrlen( str ) );
}*/


CORE_API bool ch_str_create( ch_string& s, const char* str, u64 len )
{
	if ( !str )
		return false;

	return ch_str_create_base( s, str, len );
}

/*
CORE_API bool ch_str_create( ch_ustring& s, const uchar* str, u32 len )
{
	if ( !str )
		return false;

	return ch_str_create_base( s, str, len );
}*/


CORE_API ch_string ch_str_create( const char* str )
{
	ch_string s;
	ch_str_create( s, str );
	return s;
}

/*
CORE_API ch_ustring ch_str_create( const uchar* str )
{
	ch_ustring s;
	ch_str_create( s, str );
	return s;
}
*/

CORE_API ch_string ch_str_create( const char* str, u64 len )
{
	ch_string s;
	ch_str_create( s, str, len );
	return s;
}

/*
CORE_API ch_ustring ch_str_create( const uchar* str, u32 len )
{
	ch_ustring s;
	ch_str_create( s, str, len );
	return s;
}
*/

CORE_API void ch_str_destroy( ch_string& s )
{
	if ( s.data )
		ch_str_free( s.data );

	s.data = 0;
	s.size = 0;
}

/*
CORE_API void ch_str_destroy( ch_ustring& s )
{
	if ( s.data )
		ch_str_free( s.data );

	s.data = 0;
	s.size = 0;
}
*/

// ----------------------------------------------------------------------------------------
// Comparison
// ----------------------------------------------------------------------------------------


bool ch_str_compare_base( const char* s1, const char* s2, u64 len )
{
	const char*       cur1 = s1;
	const char*       cur2 = s2;
	const char* const end  = len + s1;

	for ( ; cur1 < end; ++cur1, ++cur2 )
	{
		if ( *cur1 != *cur2 )
			return false;
	}

	return true;
}


bool ch_str_compare( const ch_string& s1, const ch_string& s2 )
{
	if ( s1.size != s2.size )
		return 1;

	return ch_str_compare_base( s1.data, s2.data, s1.size );
//	return strncmp( s1.data, s2.data, s1.size );
}

/*
bool ch_str_compare( const ch_ustring& s1, const ch_ustring& s2 )
{
	if ( s1.size != s2.size )
		return 1;
	
	return ch_str_compare_base< uchar >( s1.data, s2.data, s1.size );
//	return ch_ustrncmp( s1.data, s2.data, s1.size );
}
*/

bool ch_str_compare( const ch_string& s1, const char* s2 )
{
	if ( !s2 )
		return 1;

	size_t s2_len = strlen( s2 );

	if ( s1.size != s2_len )
		return 1;

	return ch_str_compare_base( s1.data, s2, s1.size );
//	return strncmp( s1.data, s2, s1.size );
}

/*
bool ch_str_compare( const ch_ustring& s1, const uchar* s2 )
{
	if ( !s2 )
		return 1;

	size_t s2_len = ch_ustrlen( s2 );

	if ( s1.size != s2_len )
		return 1;

	return ch_str_compare_base< uchar >( s1.data, s2, s1.size );
//	return ch_ustrncmp( s1.data, s2, s1.size );
}
*/

bool ch_str_compare( const ch_string& s1, const char* s2, u64 len )
{
	if ( !s2 )
		return 1;

	if ( s1.size != len )
		return 1;

	return ch_str_compare_base( s1.data, s2, len );
//	return strncmp( s1.data, s2, len );
}

/*
bool ch_str_compare( const ch_ustring& s1, const uchar* s2, u32 len )
{
	if ( !s2 )
		return 1;

	if ( s1.size != len )
		return 1;

	return ch_str_compare_base< uchar >( s1.data, s2, len );
//	return ch_ustrncmp( s1.data, s2, len );
}*/


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


bool ch_str_append( ch_string& dest, const char* src, u64 len )
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
bool ch_str_assign_base( CH_STR& dest, const STR_TYPE* src, u64 len, STR_TYPE nullTerminator )
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


bool ch_str_assign( ch_string& dest, const char* src, u64 len )
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



bool ch_str_starts_with( const char* s, u64 len, const char* start )
{
	if ( !s || !start )
		return false;

	u64 startLen = strlen( start );

	if ( startLen == 0 )
		return false;

	if ( len == 0 )
		len = strlen( s );

	if ( len < startLen )
		return false;

	return ch_str_compare_base( s, start, startLen );
}


bool ch_str_starts_with( const char* s, u64 len, const char* start, u64 startLen )
{
	if ( !s || !start )
		return false;

	if ( startLen == 0 )
		return false;

	if ( len == 0 )
		len = strlen( s );

	if ( len < startLen )
		return false;

	return ch_str_compare_base( s, start, startLen );
}

bool ch_str_starts_with( const char* s, const char* start )
{
	if ( !s || !start )
		return false;

	u64 startLen = strlen( start );

	if ( startLen == 0 )
		return false;

	return ch_str_compare_base( s, start, startLen );
}


bool ch_str_starts_with( const char* s, const char* start, u64 startLen )
{
	if ( !s || !start )
		return false;

	if ( startLen == 0 )
		return false;

	return ch_str_compare_base( s, start, startLen );
}


bool ch_str_starts_with( const ch_string& s, const char* start )
{
	return ch_str_starts_with( s.data, s.size, start );
}


bool ch_str_starts_with( const ch_string& s, const char* start, u64 startLen )
{
	return ch_str_starts_with( s.data, s.size, start, startLen );
}


bool ch_str_starts_with( const ch_string& s, const ch_string& start )
{
	return ch_str_starts_with( s.data, s.size, start.data, start.size );
}


// ----------------------------------------------------------------------------------------


bool ch_str_ends_with( const char* s, u64 len, const char* end )
{
	if ( !s || !end )
		return false;

	u64 endLen = strlen( end );

	if ( len == 0 )
		len = strlen( s );

	if ( len < endLen )
		return false;

	return ch_str_compare_base( s + len - endLen, end, endLen );
}


bool ch_str_ends_with( const char* s, u64 len, const char* end, u64 endLen )
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

	return ch_str_compare_base( s + len - endLen, end, endLen );
}


bool ch_str_ends_with( const char* s, const char* end )
{
	if ( !s || !end )
		return false;

	u64 len = strlen( s );
	u64 endLen = strlen( end );

	if ( len < endLen )
		return false;

	return ch_str_compare_base( s + len - endLen, end, endLen );
}


bool ch_str_ends_with( const char* s, const char* end, u64 endLen )
{
	if ( !s || !end )
		return false;

	u64 len = strlen( s );

	if ( len < endLen )
		return false;

	return ch_str_compare_base( s + len - endLen, end, endLen );
}


bool ch_str_ends_with( const ch_string& s, const char* end )
{
	return ch_str_ends_with( s.data, s.size, end );
}


bool ch_str_ends_with( const ch_string& s, const char* end, u64 endLen )
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
u64 ch_str_contains_base( const char* s, u64 len, const char* find, u64 findLen )
{
	if ( !s || !find || len == 0 || findLen == 0 || len < findLen )
		return SIZE_MAX;

	for ( u64 i = 0; i < len - findLen; i++ )
	{
		if ( ch_str_compare_base( s + i, find, findLen ) )
			return i;
	}

	return SIZE_MAX;
}


u64 ch_str_contains( const char* s, u64 len, const char* find, u64 findLen )
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


u64 ch_str_contains( const char* s, u64 len, const char* find )
{
	if ( !s || !find )
		return SIZE_MAX;

	u64 findLen = strlen( find );

	return ch_str_contains_base( s, len, find, findLen );
}


u64 ch_str_contains( const char* s, const char* find )
{
	if ( !s || !find )
		return SIZE_MAX;

	u64 len = strlen( s );
	u64 findLen = strlen( find );

	return ch_str_contains_base( s, len, find, findLen );
}


u64 ch_str_contains( const char* s, const char* find, u64 findLen )
{
	if ( !s || !find )
		return SIZE_MAX;

	u64 len = strlen( s );

	return ch_str_contains_base( s, len, find, findLen );
}


// ----------------------------------------------------------------------------------------
// ConCommands
// ----------------------------------------------------------------------------------------


CONCMD( ch_dump_string_allocations )
{
#if CH_STRING_MEM_TRACKING
	u32 size = GetTrackedStrings().size();

	if ( size == 0 )
	{
		// this is most likely never going to hit
		Log_Msg( "No string allocations to dump!\n" );
		return;
	}

	ch_string_track_map& trackedStrings = GetTrackedStrings();

	// hopefully this should never hit SIZE_MAX
	// how would you even allocate that much memory?
	u64                  totalSize      = 0;

	for ( const auto& [ ptr, track_data ] : trackedStrings )
	{
		// add to total string size
		totalSize += track_data.size;
	}

	Log_MsgF( "%d Strings Allocated - %.6f KB\n", size, Util_BytesToKB( totalSize ) );
#else
	Log_Msg( "String allocation tracking is disabled!\n" );
#endif
}


CONCMD( ch_dump_string_allocations_extended )
{
#if CH_STRING_MEM_TRACKING
	u32 size = GetTrackedStrings().size();

	if ( size == 0 )
	{
		// this is most likely never going to hit
		Log_Msg( "No string allocations to dump!\n" );
		return;
	}

	Log_MsgF( "Dumping %d string allocations:\n\n", size );

	ch_string_track_map& trackedStrings = GetTrackedStrings();

	// hopefully this should never hit SIZE_MAX
	// how would you even allocate that much memory?
	u64                  totalSize      = 0;

	for ( const auto& [ ptr, track_data ] : trackedStrings )
	{
		// add to total string size
		totalSize += track_data.size;

		Log_MsgF(
			ANSI_CLR_CYAN "%s" ANSI_CLR_DEFAULT ":" ANSI_CLR_DARK_YELLOW "%d " ANSI_CLR_DARK_GREEN "%s - %d bytes\n" ANSI_CLR_DEFAULT "\"%s\"\n",
			track_data.file, track_data.line, track_data.func, track_data.size, ptr
		);
	}

	Log_MsgF( "\nDumped %d string allocations\n", size );
	Log_MsgF( "Total string memory: %.6f KB\n", Util_BytesToKB( totalSize ) );
#else
	Log_Msg( "String allocation tracking is disabled!\n" );
#endif
}

