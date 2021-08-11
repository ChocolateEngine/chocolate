#include "../../inc/shared/platform.h"

#include <stdio.h>
#include <stdlib.h>

#ifdef _WIN32
	// #include <winbase.h>  // FormatMessage
	#include <Windows.h>
#endif
	
void PrintLastError( const char* userErrorMessage )
{
	
#ifdef _WIN32
	DWORD dLastError = GetLastError();
	LPCTSTR strErrorMessage = NULL;

	FormatMessage(
		FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS | FORMAT_MESSAGE_ARGUMENT_ARRAY | FORMAT_MESSAGE_ALLOCATE_BUFFER,
		NULL,
		dLastError,
		0,
		(LPSTR) &strErrorMessage,
		0,
		NULL);

	fprintf( stderr, "Error: %s\nWin32 API Error %d: %s\n", userErrorMessage, dLastError, strErrorMessage );

#elif __linux__

	fprintf( stderr, "Error: %s\n", userErrorMessage );

#else

#endif
}

