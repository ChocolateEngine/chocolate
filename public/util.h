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

#define MALLOC_NEW( type ) (type*)malloc(sizeof(struct type))

#define CH_ARR_SIZE( arr ) (sizeof(arr) / sizeof(arr[0]))
#define ARR_SIZE           CH_ARR_SIZE

// much faster alternative to dynamic_cast
#define IS_TYPE( var1, var2 ) typeid(var1) == typeid(var2)
#define IS_NOT_TYPE( var1, var2 ) typeid(var1) != typeid(var2)

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


using u8  = unsigned char;
using u16 = unsigned short;
using u32 = unsigned int;
using u64 = unsigned long long;

using s8  = char;
using s16 = short;
using s32 = int;
using s64 = long long;

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
	if ( !data )
		return;

	free( data );
}


template< typename Type >
inline void ch_free( Type* data )
{
	if ( !data )
		return;

	free( data );
}


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


// Reallocate X Amount of a specific type, could be named better
template< typename To >
inline bool ch_realloc2( To*& spData, u64 sCount )
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


// Allocate X Amount of a specific type on the stack
template< typename To >
inline To* ch_stack_alloc( u64 sCount )
{
	// To* data = static_cast< To* >( _malloca( CH_ALIGN_VALUE( sCount * sizeof( To ), 16 ) ) );
	To* data = static_cast< To* >( _malloca( sCount * sizeof( To ) ) );
	CH_ASSERT( data );
	return data;
}


// old names
// TODO: replace this
#define ch_malloc_count                 ch_malloc
#define ch_calloc_count                 ch_calloc
#define ch_realloc_count                ch_realloc
#define ch_realloc_count2               ch_realloc2

#define CH_MALLOC( type, count )        ch_malloc< type >( count )
#define CH_CALLOC( type, count )        ch_calloc< type >( count )
#define CH_REALLOC( type, data, count ) ch_realloc2< type >( data, count )

#define CH_MALLOC2( type, count )       static_cast< Type* >( malloc( count * sizeof( type ) )


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


// ==============================================================================
// String Types
// These ideally are never used in function arguments, but are used for return values
// There is no reason to have these as arguments anyway, as they are just pointers and sizes
// this way we don't have to construct an entire object just to pass it to a function


CORE_API void ch_str_free( char* string );
CORE_API bool ch_str_equals( const char* str1, u64 str1Len, const char* str2, u64 str2Len );
CORE_API bool ch_str_equals( const char* str1, u64 str1Len, const char* str2 );


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

void CORE_API        str_upper( std::string& string );
void CORE_API        str_lower( std::string& string );

// would like better names for this, but oh well
std::string CORE_API str_upper2( const std::string& in );
std::string CORE_API str_lower2( const std::string& in );

#ifdef _WIN32
// Find the first occurrence of find in s while ignoring case
char CORE_API* strcasestr( const char* s, const char* find );
#endif

// don't need to worry about any resizing with these, but is a little slower
void CORE_API        vstring( std::string& output, const char* format, ... );
void CORE_API        vstringV( std::string& output, const char* format, va_list args );

std::string CORE_API vstring( const char* format, ... );
std::string CORE_API vstringV( const char* format, va_list args );


#define VSTRING( out, format )     \
	va_list args;                  \
	va_start( args, format );      \
	vstringV( out, format, args ); \
	va_end( args )


// ==============================================================================
// String Allocation System


// ==============================================================================
// Assorted Number/String Functions

std::string     CORE_API Vec2Str( const glm::vec3& in );
std::string     CORE_API Quat2Str( const glm::quat& in );

double          CORE_API ToDouble( const char* value, double prev );
long            CORE_API ToLong( const std::string& value, int prev );

bool            CORE_API ToDouble2( const std::string &value, double &out );
bool            CORE_API ToLong2( const std::string &value, long &out );

bool CORE_API            ToFloat( const char* spValue, float& srOut );
bool            CORE_API ToDouble3( const char* spValue, double &srOut );
bool            CORE_API ToLong3( const char* spValue, long &srOut );

std::string     CORE_API ToString( float value );

std::string     CORE_API ch_to_string( float value );


inline float Round( float val, u64 precision = 100 )
{
	return roundf( val * precision ) / precision;
}

inline int RandomInt( int sMin, int sMax )
{
	return sMin + rand() % ( sMax + 1 - sMin );
}

inline u8 RandomU8( u8 sMin, u8 sMax )
{
	return sMin + rand() % ( sMax + 1 - sMin );
}

inline u16 RandomU16( u16 sMin, u16 sMax )
{
	return sMin + rand() % ( sMax + 1 - sMin );
}

inline u32 RandomU32( u32 sMin, u32 sMax )
{
	return sMin + rand() % ( sMax + 1 - sMin );
}

inline u64 RandomU64( u64 sMin, u64 sMax )
{
	return sMin + rand() % ( sMax + 1 - sMin );
}

inline float RandomFloat( float sMin, float sMax )
{
	return sMin + ( ( static_cast<float>( rand() ) / static_cast<float>( RAND_MAX ) ) * ( sMax - sMin ) );
}

template< typename KEY, typename VALUE >
inline u64 Util_SizeOfUnordredMap( const std::unordered_map< KEY, VALUE >& srMap )
{
	return 0;

	// return ( srMap.size() * ( sizeof( std::unordered_map< KEY, VALUE >::value_type ) + sizeof( void* ) ) +  // data list
	//          srMap.bucket_count() * ( sizeof( void* ) + sizeof( u64 ) ) )                                // bucket index
	//        * 1.5;                                                                                           // estimated allocation overheads
}


inline float Util_BytesToMB( u64 bytes )
{
	return bytes * 0.000001;
}


inline float Util_BytesToKB( u64 bytes )
{
	return bytes * 0.001;
}


inline float Sys_BytesToMB( u64 bytes )
{
#ifdef _WIN32
	return bytes * 0.00000095367432;  // 1024 multiples for windows
#else
	return bytes * 0.000001;
#endif
}


inline float Sys_BytesToKB( u64 bytes )
{
#ifdef _WIN32
	return bytes * 0.000976563;  // 1024 multiples for windows
#else
	return bytes * 0.001;
#endif
}


// ==============================================================================
// Helper Functions for SpeedyKeyV
// mainly until speedykeyv can parse lists, one day

std::vector< std::string >	CORE_API KV_GetVec( const std::string& value );

std::vector< int >          CORE_API KV_GetVecInt( const std::string& value );
std::vector< float >        CORE_API KV_GetVecFloat( const std::string& value );

glm::vec2                   CORE_API KV_GetVec2( const std::string& value, const glm::vec2& fallback = {} );
glm::vec3                   CORE_API KV_GetVec3( const std::string& value, const glm::vec3& fallback = {} );
glm::vec4                   CORE_API KV_GetVec4( const std::string& value, const glm::vec4& fallback = {} );


// ==============================================================================
// Ref Counter Class


struct RefCounted
{
	virtual ~RefCounted() = default;

	inline RefCounted& operator=( const RefCounted& srRef )
	{
		return *this;
	}

	u32 GetRefCount() const
	{
		return aRefCount;
	}

	// Add or release a reference to this object
	inline void AddRef() const
	{
		aRefCount++;
	}

	// Returns true if this was deleted
	inline bool Release() const
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


template <class T>
class RefTarget
{
public:
	virtual            ~RefTarget() = default;

	inline RefTarget&   operator=( const RefTarget &srRef )        { return *this; }
	
	u32                 GetRefCount() const                         { return aRefCount; }

	// Add or release a reference to this object
	inline void         AddRef() const                              { ++aRefCount; }
	inline void         Release() const                             { if (--aRefCount == 0) delete static_cast<const T *>(this); }

private:

	mutable std::atomic< u32 > aRefCount = 0;  // Current reference count
};


// ==============================================================================
// Assorted Functions





