#pragma once

/*
 * 
 * File for assorted utility functions
 * 
 */

#include "core/platform.h"

#include <cstdarg>
#include <vector>
#include <string>
#include <algorithm>
#include <atomic>

#include <time.h>

#include <glm/vec3.hpp>
#include <glm/gtc/quaternion.hpp>

#if __linux__
#include <sys/stat.h>
#endif /* __linux__  */


// ==============================================================================
// Helper Macros

#define MALLOC_NEW( type ) (type*)malloc(sizeof(struct type))

#define ARR_SIZE( arr ) (sizeof(arr) / sizeof(arr[0]))

// much faster alternative to dynamic_cast
#define IS_TYPE( var1, var2 ) typeid(var1) == typeid(var2)
#define IS_NOT_TYPE( var1, var2 ) typeid(var1) != typeid(var2)

// need macro for constant expression
#define CH_ALIGN_VALUE( val, alignment ) ( ( val + alignment - 1 ) & ~( alignment - 1 ) ) 

// msvc
#if _MSC_VER
	#define CH_STACK_ALLOC( size )      _malloca( CH_ALIGN_VALUE( size, 16 ) )
	#define CH_STACK_FREE( data )       _freea( data )

	// obtain size of block of memory allocated from heap
	#define CH_MALLOC_SIZE( block )     ( _msize( block ) )
#else
	#define CH_STACK_ALLOC( size )      alloca( CH_ALIGN_VALUE( size, 16 ) )
	#define CH_STACK_FREE( data )       

	// obtain size of block of memory allocated from heap
	#define CH_MALLOC_SIZE( block )     ( malloc_usable_size( block ) )
#endif

// #define CH_STACK_NEW( type, count ) ( type* ) CH_STACK_ALLOC( sizeof( type ) * count )
#define CH_STACK_NEW( type, count ) static_cast< type* >( CH_STACK_ALLOC( sizeof( type ) * count ) )

template< typename To, typename From >
inline To* assert_cast( From* in )
{
#ifdef _DEBUG
	To* to = dynamic_cast< To >( in );
	Assert( to != nullptr );
	return to;
#else
	return static_cast< To >( in );
#endif
}

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


// ==============================================================================
// Helper Functions for std::vector

#define NEW_VEC_INDEX 1

template <class T>
constexpr size_t vec_index( std::vector<T> &vec, T item, size_t fallback = SIZE_MAX )
{
#if NEW_VEC_INDEX
	auto it = std::find( vec.begin(), vec.end(), item );
	if ( it != vec.end() )
		return it - vec.begin();
#else
	for (size_t i = 0; i < vec.size(); i++)
	{
		if (vec[i] == item)
			return i;
	}
#endif

	return fallback;
}


template <class T>
constexpr size_t vec_index( const std::vector<T> &vec, T item, size_t fallback = SIZE_MAX )
{
#if NEW_VEC_INDEX
	auto it = std::find( vec.begin(), vec.end(), item );
	if ( it != vec.end() )
		return it - vec.begin();
#else
	for (size_t i = 0; i < vec.size(); i++)
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
constexpr void vec_remove_index( std::vector<T> &vec, size_t index )
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

void           CORE_API str_upper( std::string &string );
void           CORE_API str_lower( std::string &string );

// would like better names for this, but oh well
std::string    CORE_API str_upper2( const std::string &in );
std::string    CORE_API str_lower2( const std::string &in );

#ifdef _WIN32
// Find the first occurrence of find in s while ignoring case
char          CORE_API *strcasestr( const char* s, const char* find );
#endif

// don't need to worry about any resizing with these, but is a little slower
void           CORE_API vstring( std::string& output, const char* format, ... );
void           CORE_API vstringV( std::string& output, const char* format, va_list args );

std::string    CORE_API vstring( const char* format, ... );
std::string    CORE_API vstringV( const char* format, va_list args );


#define VSTRING( out, format ) \
	va_list args; \
	va_start( args, format ); \
	vstringV( out, format, args ); \
	va_end( args )


// ==============================================================================
// Assorted Number/String Functions

std::string     CORE_API Vec2Str( const glm::vec3& in );
std::string     CORE_API Quat2Str( const glm::quat& in );

double          CORE_API ToDouble( const char* value, double prev );
long            CORE_API ToLong( const std::string& value, int prev );

bool            CORE_API ToDouble2( const std::string &value, double &out );
bool            CORE_API ToLong2( const std::string &value, long &out );

bool            CORE_API ToDouble3( const char* spValue, double &srOut );
bool            CORE_API ToLong3( const char* spValue, long &srOut );

std::string     CORE_API ToString( float value );


inline float Round( float val, size_t precision = 100 )
{
	return roundf( val * precision ) / precision;
}

inline int RandomInt( int sMin, int sMax )
{
	return sMin + rand() % ( sMax + 1 - sMin );
}

inline size_t RandomSizeT( size_t sMin, size_t sMax )
{
	return sMin + rand() % ( sMax + 1 - sMin );
}

inline float RandomFloat( float sMin, float sMax )
{
	return sMin + ( ( static_cast<float>( rand() ) / static_cast<float>( RAND_MAX ) ) * ( sMax - sMin ) );
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

	inline void Release() const
	{
		if ( --aRefCount == 0 ) delete this;
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





