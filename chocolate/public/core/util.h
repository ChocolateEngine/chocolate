#pragma once

/*
 * 
 * File for assorted utility functions
 * 
 */

#include "core/platform.h"
#include "core/asserts.h"

#include <cstdarg>
#include <vector>
#include <string>
#include <algorithm>
#include <atomic>
#include <unordered_map>

#include <time.h>

#include <glm/vec3.hpp>
#include <glm/gtc/quaternion.hpp>

#if __linux__
#include <sys/stat.h>
#endif /* __linux__  */


// ==============================================================================
// Helper Macros

#define CH_ARR_SIZE( arr ) (sizeof(arr) / sizeof(arr[0]))
#define ARR_SIZE           CH_ARR_SIZE

// need macro for constant expression
#define CH_ALIGN_VALUE( val, alignment ) ( ( val + alignment - 1 ) & ~( alignment - 1 ) ) 

// msvc
#if _MSC_VER
  #define CH_STACK_ALLOC( size )  _malloca( CH_ALIGN_VALUE( size, 16 ) )
  #define CH_STACK_FREE( data )   _freea( data )

	// obtain size of block of memory allocated from heap
  #define CH_MALLOC_SIZE( block ) ( _msize( block ) )

  #define CH_FUNC_NAME            __func__
  #define CH_FUNC_NAME_CLASS      __FUNCTION__
  #define CH_FUNC_SIG             __PRETTY_FUNCTION__

  #define ch_strncasecmp          _strnicmp
  #define ch_strcasecmp           _stricmp
#else
  #define CH_STACK_ALLOC( size )      alloca( CH_ALIGN_VALUE( size, 16 ) )
  #define CH_STACK_FREE( data )       
  
  // obtain size of block of memory allocated from heap
  #define CH_MALLOC_SIZE( block )     ( malloc_usable_size( block ) )
  
  #define CH_FUNC_NAME                __func__
  #define CH_FUNC_NAME_CLASS          __FUNCTION__
  #define CH_FUNC_SIG                 __PRETTY_FUNCTION__

  #define ch_strncasecmp              strncasecmp
  #define ch_strcasecmp               strcasecmp
#endif
 

// #define CH_STACK_NEW( type, count ) ( type* ) CH_STACK_ALLOC( sizeof( type ) * count )
#define CH_STACK_NEW( type, count ) static_cast< type* >( CH_STACK_ALLOC( sizeof( type ) * count ) )


// ==============================================================================
// Short Types


using s8  = char;
using s16 = short;
using s32 = int;
using s64 = long long;

using u8  = unsigned char;
using u16 = unsigned short;
using u32 = unsigned int;
using u64 = unsigned long long;

using f32 = float;
using f64 = double;

using bolean = bool;  // funny


// ==============================================================================


// dynamic_cast with a check for if it returns a nullptr
template< typename To, typename From >
inline To* assert_cast( From* in )
{
#ifdef _DEBUG
	To* to = dynamic_cast< To >( in );
	CH_ASSERT( to != nullptr );
	return to;
#else
	return static_cast< To >( in );
#endif
}


// Checks for a nullptr and casts it
template< typename To, typename From >
inline To* ch_pointer_cast( From* in )
{
	CH_ASSERT( in );
	return static_cast< To* >( in );
}


// TODO: add in memory tracking here
inline void ch_free( void* data )
{
	free( data );
}


//template< typename Type >
//inline void ch_free( Type* data )
//{
//	free( data );
//}


// Allocate X Amount of a specific type, could be named better
template< typename To >
inline To* ch_malloc( u64 sCount )
{
	To* data = static_cast< To* >( malloc( sizeof( To ) * sCount ) );
	CH_ASSERT( data );
	return data;
}


// Reallocate X Amount of a specific type, could be named better
template< typename To >
inline To* ch_realloc( To* spData, u64 sCount )
{
	To* data = static_cast< To* >( realloc( spData, sizeof( To ) * sCount ) );
	CH_ASSERT( data );
	return data;
}


// Reallocate X Amount of a specific type, automatically reassigns the pointer
template< typename To >
inline bool ch_realloc_auto( To*& spData, u64 sCount )
{
	To* data = static_cast< To* >( realloc( spData, sizeof( To ) * sCount ) );
	
	CH_ASSERT( data );
	if ( !data )
		return false;

	spData = data;
	return true;
}


// Allocate X Amount of a specific type and zero the memory, could be named better
template< typename To >
inline To* ch_calloc( u64 sCount )
{
	To* data = static_cast< To* >( calloc( sCount, sizeof( To ) ) );
	CH_ASSERT( data );
	return data;
}


template< typename T >
T* ch_recalloc( T* data, size_t count, size_t add_count )
{
	T* new_data = (T*)realloc( data, ( count + add_count ) * sizeof( T ) );

	if ( new_data )
		memset( &new_data[ count ], 0, add_count * sizeof( T ) );

	return new_data;
}


// Allocate X Amount of a specific type on the stack
template< typename To >
inline To* ch_stack_alloc( u64 sCount )
{
	// To* data = static_cast< To* >( _malloca( CH_ALIGN_VALUE( sCount * sizeof( To ), 16 ) ) );
	To* data = static_cast< To* >( _malloca( sCount * sizeof( To ) ) );
	CH_ASSERT( data );
	return data;
}


#define CH_MALLOC( type, count )        ch_malloc< type >( count )
#define CH_CALLOC( type, count )        ch_calloc< type >( count )
#define CH_REALLOC( type, data, count ) ch_realloc< type >( data, count )


// inline int ch_strcasecmp( const char* spStr1, const char* spStr2 )
// {
// #ifdef _MSC_VER
// 	return _stricmp( spStr1, spStr1 );
// #else
// 	return strcasecmp( spStr1, spStr1 );
// #endif
// }


//inline int ch_strncasecmp( const char* spStr1, const char* spStr2, u64 sCount )
//{
//#ifdef _MSC_VER
//	return _strnicmp( spStr1, spStr1, sCount );
//#else
//	return strncasecmp( spStr1, spStr1, sCount );
//#endif
//}


// removes the element and shifts everything back, and memsets the last item with 0
template< typename T, typename COUNT_TYPE >
void util_array_remove_element( T* data, COUNT_TYPE& count, COUNT_TYPE index )
{
	if ( index >= count )
		return;

	memcpy( &data[ index ], &data[ index + 1 ], sizeof( T ) * ( count - index ) );
	count--;

	if ( count == 0 )
		return;

	memset( &data[ count ], 0, sizeof( T ) );
}


template< typename T >
bool util_array_append( T*& data, u32 count )
{
#if 1
	T* new_data = ch_recalloc< T >( data, count, 1 );

	if ( !new_data )
		return true;

	data = new_data;
#else
	T* new_data = ch_realloc< T >( data, count + 1 );

	if ( !new_data )
		return true;

	data = new_data;
	memset( &data[ count ], 0, sizeof( T ) );
#endif

	return false;
}


// Allocates X amount more space in the array
template< typename T >
bool util_array_extend( T*& data, u32 count, u32 extend_amount )
{
#if 1
	T* new_data = ch_recalloc< T >( data, count, extend_amount );

	if ( !new_data )
		return true;

	data = new_data;
#else
	T* new_data = ch_realloc< T >( data, count + extend_amount );

	if ( !new_data )
		return true;

	data = new_data;
	memset( &data[ count ], 0, sizeof( T ) );
#endif

	return false;
}


template< typename T >
bool util_array_append_err( T*& data, u32 count, const char* msg )
{
#if 1
	T* new_data = ch_recalloc< T >( data, count, 1 );

	if ( !new_data )
	{
		fputs( msg, stdout );
		return true;
	}

	data = new_data;
#else
	T* new_data = ch_realloc< T >( data, count + 1 );

	if ( !new_data )
		return true;

	data = new_data;
	memset( &data[ count ], 0, sizeof( T ) );
#endif

	return false;
}


template< typename T >
void util_zeromem( T* data )
{
	if ( !data )
		return;

	memset( data, 0, sizeof( T ) );
}


// ==============================================================================
// String Types
// These ideally are never used in function arguments, but are used for return values
// There is no reason to have these as arguments anyway, as they are just pointers and sizes
// this way we don't have to construct an entire object just to pass it to a function


CORE_API void ch_str_free( char* string );
CORE_API bool ch_str_equals( const char* str1, size_t str1Len, const char* str2, size_t str2Len );
CORE_API bool ch_str_equals( const char* str1, size_t str1Len, const char* str2 );


// This is an easy way to pass ch_string to functions that take a char* and a size
#define CH_STR_UNROLL( str ) str.data, str.size
#define CH_STR_UR( str ) str.data, str.size


struct ch_string
{
	char*  data = nullptr;
	size_t size = 0;

	constexpr ch_string()
	{
	}

	constexpr ch_string( char* spData, size_t sSize )
		: data( spData ), size( sSize )
	{
	}

	constexpr ch_string( const char* spData, size_t sSize )
		: data( (char*)spData ), size( sSize )
	{
	}
};


// inline bool operator==( const ch_string& srString, const char* spString )
// {
// 	return ( ch_str_equals( srString.data, srString.size, spString ) );
// }


inline bool operator==( const ch_string& srString, const ch_string& other )
{
	return ( ch_str_equals( srString.data, srString.size, other.data, other.size ) );
}


// this one automatically frees the memory upon destruction
struct ch_string_auto
{
	char*  data = nullptr;
	size_t size = 0;

	ch_string_auto()
	{
	}

	// we don't have a const char* version,
	// as that usually means the string is a literal,
	// and we don't want to free that
	ch_string_auto( char* spData, size_t sSize )
		: data( spData ), size( sSize )
	{
	}

	ch_string_auto( const ch_string& srString )
		: data( srString.data ), size( srString.size )
	{
	}

	~ch_string_auto()
	{
		if ( data )
			ch_str_free( data );
	}

	// auto convert to ch_string
	operator ch_string()
	{
		return ch_string( data, size );
	}

	// if i don't have this here, then sometimes it will call the deconstructor, and sometimes it won't
	void operator=( const ch_string& srString )
	{
		data = srString.data;
		size = srString.size;
	}

	// copying
	void assign( const ch_string_auto& other )
	{
		this->data = other.data;
		this->size = other.size;
	}

	// moving
	void assign( ch_string_auto&& other )
	{
		this->data = other.data;
		this->size = other.size;
	}

	ch_string_auto& operator=( const ch_string_auto& other )
	{
		assign( other );
		return *this;
	}

	ch_string_auto& operator=( ch_string_auto&& other )
	{
		assign( other );
		return *this;
	}

	// copying
	ch_string_auto( const ch_string_auto& other )
	{
		assign( other );
	}

	// moving
	ch_string_auto( ch_string_auto&& other )
	{
		// assign( std::move( other ) );
		assign( other );
	}
};


// Hashing Support for ch_string and ch_string_auto
namespace std
{
template<>
struct hash< ch_string >
{
	size_t operator()( ch_string const& string ) const
	{
		size_t value = ( hash< size_t >()( string.size ) );

		for ( size_t i = 0; i < string.size; i++ )
			value ^= ( hash< char >()( string.data[ i ] ) );

		return value;
	}
};

template<>
struct hash< ch_string_auto >
{
	size_t operator()( ch_string_auto const& string ) const
	{
		size_t value = ( hash< size_t >()( string.size ) );

		for ( size_t i = 0; i < string.size; i++ )
			value ^= ( hash< char >()( string.data[ i ] ) );

		return value;
	}
};
}


// uchar is used for unicode strings
// uchar is wchar_t on windows, and char on linux
// struct ch_ustring
// {
// 	uchar* data = nullptr;
// 	u32    size = 0;
// };


// ==============================================================================
// Helper Functions for std::vector

#define NEW_VEC_INDEX 1

template <class T>
constexpr size_t vec_index( std::vector< T >& vec, T item, size_t fallback = SIZE_MAX )
{
#if NEW_VEC_INDEX
	auto it = std::find( vec.begin(), vec.end(), item );
	if ( it != vec.end() )
		return it - vec.begin();
#else
	for ( size_t i = 0; i < vec.size(); i++ )
	{
		if (vec[i] == item)
			return i;
	}
#endif

	return fallback;
}


template <class T>
constexpr size_t vec_index( const std::vector< T >& vec, T item, size_t fallback = SIZE_MAX )
{
#if NEW_VEC_INDEX
	auto it = std::find( vec.begin(), vec.end(), item );
	if ( it != vec.end() )
		return it - vec.begin();
#else
	for ( size_t i = 0; i < vec.size(); i++ )
	{
		if (vec[i] == item)
			return i;
	}
#endif

	return fallback;
}


template <class T>
constexpr void vec_remove( std::vector<T> &vec, T item )
{
	vec.erase( vec.begin() + vec_index( vec, item ) );
}


// Remove item if it exists
template <class T>
constexpr void vec_remove_if( std::vector<T> &vec, T item )
{
	size_t index = vec_index( vec, item );
	if ( index != SIZE_MAX )
		vec.erase( vec.begin() + index );
}


template <class T>
constexpr void vec_remove_index( std::vector< T >& vec, size_t index )
{
	vec.erase( vec.begin() + index );
}


template <class T>
constexpr bool vec_contains(std::vector<T> &vec, T item)
{
	return (std::find(vec.begin(), vec.end(), item) != vec.end());
}


template <class T>
constexpr bool vec_contains(const std::vector<T> &vec, T item)
{
	return (std::find(vec.begin(), vec.end(), item) != vec.end());
}


// ==============================================================================
// String Tools

CORE_API void         str_upper( std::string& string );
CORE_API void         str_lower( std::string& string );

// would like better names for this, but oh well
CORE_API std::string  str_upper2( const std::string& in );
CORE_API std::string  str_lower2( const std::string& in );

#ifdef _WIN32
// Find the first occurrence of find in s while ignoring case
CORE_API char* strcasestr( const char* s, const char* find );
#endif

// don't need to worry about any resizing with these, but is a little slower
CORE_API void         vstring( std::string& output, const char* format, ... );
CORE_API void         vstringV( std::string& output, const char* format, va_list args );

CORE_API std::string  vstring( const char* format, ... );
CORE_API std::string  vstringV( const char* format, va_list args );


#define VSTRING( out, format )     \
	va_list args;                  \
	va_start( args, format );      \
	vstringV( out, format, args ); \
	va_end( args )


// ==============================================================================
// Assorted Number/String Functions

CORE_API std::string ch_vec3_to_str( const glm::vec3& in );
CORE_API std::string ch_quat_to_str( const glm::quat& in );

// TODO: write this
// CORE_API void ch_vec3_to_str( char* buffer, size_t buffer_size, const glm::vec3& in );
// CORE_API void ch_quat_to_str( char* buffer, size_t buffer_size, const glm::quat& in );

CORE_API bool        ch_to_float( const char* spValue, float& srOut );
CORE_API bool        ch_to_double( const char* spValue, double& srOut );
CORE_API bool        ch_to_long( const char* spValue, long& srOut );

CORE_API std::string ch_to_string( float value );


template< typename T >
inline T rand_num( T min, T max )
{
	return min + rand() % ( max + 1 - min );
}


inline int rand_int( int min, int max )
{
	return min + rand() % ( max + 1 - min );
}


inline u8 rand_u8( u8 min, u8 max )
{
	return min + rand() % ( max + 1 - min );
}


inline u16 rand_u16( u16 min, u16 max )
{
	return min + rand() % ( max + 1 - min );
}


inline u32 rand_u32( u32 min, u32 max )
{
	return min + rand() % ( max + 1 - min );
}


inline u64 rand_u64( u64 min, u64 max )
{
	return min + rand() % ( max + 1 - min );
}


inline float rand_float( float min, float max )
{
	return min + ( ( static_cast< float >( rand() ) / static_cast< float >( RAND_MAX ) ) * ( max - min ) );
}


template< typename KEY, typename VALUE >
inline u64 ch_size_of_umap( const std::unordered_map< KEY, VALUE >& srMap )
{
	return 0;

	// return ( srMap.size() * ( sizeof( std::unordered_map< KEY, VALUE >::value_type ) + sizeof( void* ) ) +  // data list
	//          srMap.bucket_count() * ( sizeof( void* ) + sizeof( u64 ) ) )                                // bucket index
	//        * 1.5;                                                                                           // estimated allocation overheads
}


inline float ch_bytes_to_mb( u64 bytes )
{
	return bytes * 0.000001;
}


inline float ch_bytes_to_kb( u64 bytes )
{
	return bytes * 0.001;
}


inline float ch_bytes_to_mb_sys( u64 bytes )
{
#ifdef _WIN32
	return bytes * 0.00000095367432;  // 1024 multiples for windows
#else
	return bytes * 0.000001;
#endif
}


inline float ch_bytes_to_kb_sys( u64 bytes )
{
#ifdef _WIN32
	return bytes * 0.000976563;  // 1024 multiples for windows
#else
	return bytes * 0.001;
#endif
}


// print a string to stdout, shortcut to fputs( str, stdout );
inline void print( const char* str )
{
	fputs( str, stdout );
}


struct ref_count_t
{
	virtual ~ref_count_t() = default;

	inline ref_count_t& operator=( const ref_count_t& srRef )
	{
		return *this;
	}

	// Add or release a reference to this object
	inline void add_ref() const
	{
		aRefCount++;
	}

	// Returns true if this was deleted
	inline bool release() const
	{
		if ( --aRefCount == 0 )
		{
			delete this;
			return true;
		}

		return false;
	}

	mutable std::atomic< u32 > aRefCount = 0;  // Current reference count
};
