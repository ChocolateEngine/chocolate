#include "util.h"

#include <stdarg.h>
#include <algorithm>
#include <stdexcept>
#include <sstream>
#include <iomanip>


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


// very cool
// https://stackoverflow.com/questions/55424746/is-there-an-analogous-function-to-vsnprintf-that-works-with-stdstring
std::string vstring( const char* format, ... )
{
	std::string result;
	va_list args, args_copy;

	va_start( args, format );
	va_copy( args_copy, args );

	int len = vsnprintf( nullptr, 0, format, args );
	if (len < 0)
	{
		va_end(args_copy);
		va_end(args);
		throw std::runtime_error("vsnprintf error");
	}

	if ( len > 0 )
	{
		result.resize( len );
		vsnprintf( result.data(), len+1, format, args_copy );
	}

	va_end( args_copy );
	va_end( args );

	return result;
}

std::string vstring( const char* format, va_list args )
{
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
	
	throw std::runtime_error( "vsnprintf error" );

	//const auto err = errno;
	//const auto ec = std::error_code(err, std::generic_category());
	//throw std::system_error(ec);
}


std::string Vec2Str( const glm::vec3& in )
{
	return vstring("(%.4f, %.4f, %.4f)", in.x, in.y, in.z);
}

std::string Quat2Str( const glm::quat& in )
{
	return vstring("(%.4f, %.4f, %.4f, %.4f)", in.w, in.x, in.y, in.z);
}


double ToDouble( const std::string& value, double prev )
{
	if ( value.empty() )
		return prev;

	char* end;
	double result = strtod( value.c_str(), &end );

	return end == value.c_str() ? prev : result;
}


long ToLong( const std::string& value, int prev )
{
	if ( value.empty() )
		return prev;

	char* end;
	long result = strtol( value.c_str(), &end, 10 );

	return end == value.c_str() ? prev : result;
}


// very cool: https://stackoverflow.com/a/46424921/12778316
std::string ToString( float value )
{
	std::ostringstream oss;
	oss << std::setprecision(8) << std::noshowpoint << value;
	return oss.str();
}

