#include "core/console.h"
#include "core/log.h"
#include "util.h"

#include <stdarg.h>
#include <algorithm>
#include <stdexcept>
#include <sstream>
#include <iomanip>
#include <filesystem>

#include <sys/stat.h>


#ifdef _WIN32
	//#include <direct.h>
	//#include <sysinfoapi.h>
	#include <io.h>

#elif __linux__
	#include <stdlib.h>
	#include <unistd.h>
#endif

LOG_CHANNEL_REGISTER( KeyValue, ELogColor_DarkGray );


void str_upper( std::string &string )
{
	std::transform( string.begin(), string.end(), string.begin(), ::toupper );
}

void str_lower( std::string &string )
{
	std::transform( string.begin(), string.end(), string.begin(), ::tolower);
}


std::string str_upper2( const std::string &in )
{
	std::string string = in;
	std::transform( string.begin(), string.end(), string.begin(), ::toupper );
	return string;
}

std::string str_lower2( const std::string &in )
{
	std::string string = in;
	std::transform( string.begin(), string.end(), string.begin(), ::tolower );
	return string;
}


#ifdef _WIN32
// Find the first occurrence of find in s while ignoring case
char* strcasestr( const char* s, const char* find )
{
	char c, sc;

	if ( ( c = *find++ ) == 0 )
		return ( (char*)s );

	// convert to lower case character
	c          = tolower( (unsigned char)c );
	size_t len = strlen( find );
	do
	{
		// compare lower case character
		do
		{
			if ( ( sc = *s++ ) == 0 )
				return nullptr;

		} while ( (char)tolower( (unsigned char)sc ) != c );
	} while ( _strnicmp( s, find, len ) != 0 );
	s--;

	return ( (char*)s );
}
#endif


// very cool
// https://stackoverflow.com/questions/55424746/is-there-an-analogous-function-to-vsnprintf-that-works-with-stdstring
void vstring( std::string& result, const char* format, ... )
{
	PROF_SCOPE();

	va_list args, args_copy;

	va_start( args, format );
	va_copy( args_copy, args );

	int len = vsnprintf( nullptr, 0, format, args );
	if ( len < 0 )
	{
		va_end( args_copy );
		va_end( args );
		Log_Error( "vstring va_args: vsnprintf failed\n" );
		return;
	}

	if ( len > 0 )
	{
		result.resize( len );
		vsnprintf( result.data(), len+1, format, args_copy );
	}

	va_end( args_copy );
	va_end( args );
}

void vstringV( std::string& s, const char* format, va_list args )
{
	PROF_SCOPE();

	va_list copy;
	va_copy( copy, args );
	int len = std::vsnprintf( nullptr, 0, format, copy );
	va_end( copy );

	if ( len >= 0 )
	{
		//std::string s( std::size_t(len) + 1, '\0' );
		s.resize( std::size_t(len) + 1, '\0' );
		std::vsnprintf( s.data(), s.size(), format, args );
		s.resize( len );
		return;
	}

	Log_Error( "vstring va_list: vsnprintf failed\n" );
}


// very cool
// https://stackoverflow.com/questions/55424746/is-there-an-analogous-function-to-vsnprintf-that-works-with-stdstring
std::string vstring( const char* format, ... )
{
	PROF_SCOPE();

	std::string result;
	va_list     args, args_copy;

	va_start( args, format );
	va_copy( args_copy, args );

	int len = vsnprintf( nullptr, 0, format, args );
	if ( len < 0 )
	{
		va_end( args_copy );
		va_end( args );
		Log_Error( "vstring va_args: vsnprintf failed\n" );
		return "";
	}

	if ( len > 0 )
	{
		result.resize( len );
		vsnprintf( result.data(), len + 1, format, args_copy );
	}

	va_end( args_copy );
	va_end( args );

	return result;
}

std::string vstringV( const char* format, va_list args )
{
	PROF_SCOPE();

	va_list copy;
	va_copy( copy, args );
	int len = std::vsnprintf( nullptr, 0, format, copy );
	va_end( copy );

	if ( len >= 0 )
	{
		std::string s( std::size_t(len) + 1, '\0' );
		std::vsnprintf( s.data(), s.size(), format, args );
		s.resize( len );
		return s;
	}

	Log_Error( "vstring va_list: vsnprintf failed\n" );
	return "";
}


// --------------------------------------------------------------------------


std::string Vec2Str( const glm::vec3& in )
{
	return vstring("(%.4f, %.4f, %.4f)", in.x, in.y, in.z);
}

std::string Quat2Str( const glm::quat& in )
{
	return vstring("(%.4f, %.4f, %.4f, %.4f)", in.w, in.x, in.y, in.z);
}


double ToDouble( const char* value, double prev )
{
	if ( value == nullptr )
		return prev;

	char* end = nullptr;
	double result = strtod( value, &end );

	return end == value ? prev : result;
}


long ToLong( const std::string& value, int prev )
{
	if ( value.empty() )
		return prev;

	char* end = nullptr;
	long result = strtol( value.c_str(), &end, 10 );

	return end == value.c_str() ? prev : result;
}


bool ToDouble2( const std::string &value, double &out )
{
	if ( value.empty() )
		return false;

	char* end = nullptr;
	out = strtod( value.c_str(), &end );

	return end != value.c_str();
}


bool ToLong2( const std::string &value, long &out )
{
	if ( value.empty() )
		return false;

	char* end = nullptr;
	out = strtol( value.c_str(), &end, 10 );

	return end != value.c_str();
}


bool ToFloat( const char* spValue, float& srOut )
{
	if ( !spValue )
		return false;

	char* end = nullptr;
	srOut = strtof( spValue, &end );
	
	return end != spValue;
}


bool ToDouble3( const char* spValue, double& srOut )
{
	if ( !spValue )
		return false;

	char* end = nullptr;
	srOut = strtod( spValue, &end );

	return end != spValue;
}


bool ToLong3( const char* spValue, long &srOut )
{
	if ( !spValue )
		return false;

	char* end = nullptr;
	srOut = strtol( spValue, &end, 10 );

	return end != spValue;
}


// very cool: https://stackoverflow.com/a/46424921/12778316
std::string ToString( float value )
{
	std::ostringstream oss;
	oss << std::setprecision(8) << std::noshowpoint << value;
	return oss.str();
}

