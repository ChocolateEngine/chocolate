#pragma once

#include <vector>
#include <string>
#include <algorithm>

#include <glm/vec3.hpp>
#include <glm/gtc/quaternion.hpp>


#define MALLOC_NEW( type ) (type*)malloc(sizeof(struct type))
#define MALLOC_NEW2( type, var ) type* var = (type*)malloc(sizeof(struct type))


template <class T>
size_t vec_index(std::vector<T> &vec, T item)
{
	for (size_t i = 0; i < vec.size(); i++)
	{
		if (vec[i] == item)
			return i;
	}

	return SIZE_MAX;
}


template <class T>
size_t vec_index(const std::vector<T> &vec, T item)
{
	for (size_t i = 0; i < vec.size(); i++)
	{
		if (vec[i] == item)
			return i;
	}

	return SIZE_MAX;
}


template <class T>
void vec_remove(std::vector<T> &vec, T item)
{
	vec.erase(vec.begin() + vec_index(vec, item));
}


template <class T>
bool vec_contains(std::vector<T> &vec, T item)
{
	return (std::find(vec.begin(), vec.end(), item) != vec.end());
}


template <class T>
bool vec_contains(const std::vector<T> &vec, T item)
{
	return (std::find(vec.begin(), vec.end(), item) != vec.end());
}


void str_upper(std::string &string);
void str_lower(std::string &string);

// don't need to worry about any resizing with these
std::string vstring( const char* format, ... );
std::string vstring( const char* format, va_list args );


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


void Print( const char* str, ... );

std::string Vec2Str( const glm::vec3& in );
std::string Quat2Str( const glm::quat& in );


inline bool FileExists( const char* path )
{
	struct stat buffer;   
	return (stat (path, &buffer) == 0); 
}


double          ToDouble( const std::string& value, double prev );
std::string     ToString( float value );

inline float Round( float val, size_t precision = 100 )
{
	return roundf( val * precision ) / precision;
}


