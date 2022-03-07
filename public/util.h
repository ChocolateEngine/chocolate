#pragma once

/*
 * 
 * File for assorted utility functions
 * 
 */

#include "core/platform.h"

#include <vector>
#include <string>
#include <algorithm>

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
constexpr void vec_remove(std::vector<T> &vec, T item)
{
	vec.erase(vec.begin() + vec_index(vec, item));
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

// don't need to worry about any resizing with these, but is a little slower
void           CORE_API vstring( std::string& output, const char* format, ... );
void           CORE_API vstring( std::string& output, const char* format, va_list args );

std::string    CORE_API vstring( const char* format, ... );
std::string    CORE_API vstring( const char* format, va_list args );


#define VSTRING( out, format ) \
	va_list args; \
	va_start( args, format ); \
	out = vstring( format, args ); \
	va_end( args )


// don't pass `const char* out` for out when using this?
// some dangling pointer warning idk
#define VCSTRING( out, format ) \
	va_list args; \
	va_start( args, format ); \
	out = vstring( format, args ).c_str(); \
	va_end( args )


// ==============================================================================
// Assorted Number/String Functions

std::string     CORE_API Vec2Str( const glm::vec3& in );
std::string     CORE_API Quat2Str( const glm::quat& in );

double          CORE_API ToDouble( const std::string& value, double prev );
long            CORE_API ToLong( const std::string& value, int prev );

bool            CORE_API ToDouble2( const std::string &value, double &out );
bool            CORE_API ToLong2( const std::string &value, long &out );

std::string     CORE_API ToString( float value );


inline float Round( float val, size_t precision = 100 )
{
	return roundf( val * precision ) / precision;
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
// Assorted Functions


/*
 *    Time in seconds since the chocolate epoch.
 *    Same as the seconds from the unix epoch from the first commit.
 * 
 *    @return s64
 *        The number of seconds since the chocolate epoch. 
 *        time since 2/15/2021 14:11:26 from the current time.
 */
static s64 GetBuildNumber() {
	return 014012500416;
}



