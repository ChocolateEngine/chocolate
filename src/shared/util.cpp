#include "../../inc/shared/util.h"
#include "../../inc/shared/console.h"

#include <stdarg.h>
#include <algorithm>

extern Console* g_console;

void str_upper(std::string &string)
{
	std::transform(string.begin(), string.end(), string.begin(), ::toupper);
}

void str_lower(std::string &string)
{
	std::transform(string.begin(), string.end(), string.begin(), ::tolower);
}


void Print( const char* str, ... )
{
	char buffer[8192];
	va_list args;
	va_start( args, str );

	vsnprintf( buffer, 8192, str, args );

	if ( g_console )
	{
		g_console->Print( buffer );
	}
	else
	{
		printf( buffer );
	}

	va_end( args );
}

