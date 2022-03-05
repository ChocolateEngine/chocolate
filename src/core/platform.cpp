#ifdef _WIN32

#include "core/platform.h"

#include <stdio.h>
#include <stdlib.h>
//#include <string>

#include <wtypes.h>  // HWND
#include <libloaderapi.h>
//#include <errhandlingapi.h>
#include <winbase.h>  // FormatMessage


// ooo yeah
#ifdef _WIN32
	#define LOAD_LIBRARY(path) LoadLibrary(path)
	#define CLOSE_LIBRARY(handle) FreeLibrary(handle)
	// #define GET_ERROR() GetLastError()
	#define GET_ERROR() "*dies of cringe*"
	#define LOAD_FUNC GetProcAddress

#elif __linux__
	#include <dlfcn.h>

	#define LOAD_LIBRARY(path) dlopen(path, RTLD_LAZY)
	#define CLOSE_LIBRARY(handle) dlclose(handle)
	#define GET_ERROR() dlerror()
	#define LOAD_FUNC dlsym

#else
	#error "Library loading not setup for this platform"
#endif


Module sys_load_library( const char* path )
{
	return (Module)LoadLibrary( path );
}

void sys_close_library( Module mod )
{
	FreeLibrary( (HMODULE)mod );
}

void* sys_load_func( Module mod, const char* name )
{
	return GetProcAddress( (HMODULE)mod, name );
}

const char* sys_get_error(  )
{
	DWORD errorID = GetLastError();

	if( errorID == 0 )
		return ""; // No error message

	LPSTR strErrorMessage = NULL;

	FormatMessage(
		FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS | FORMAT_MESSAGE_ARGUMENT_ARRAY | FORMAT_MESSAGE_ALLOCATE_BUFFER,
		NULL,
		errorID,
		0,
		(LPSTR)&strErrorMessage,
		0,
		NULL);

	//std::string message;
	//message.resize(512);
	//snprintf( message.data(), 512, "Win32 API Error %d: %s", errorID, strErrorMessage );

	static char message[512];
	memset( message, 512, 0 );
	snprintf( message, 512, "Win32 API Error %d: %s", errorID, strErrorMessage );

	// Free the Win32 string buffer.
	LocalFree( strErrorMessage );

	return message;
}

void sys_print_last_error( const char* userErrorMessage )
{
	fprintf( stderr, "Error: %s\n%s\n", userErrorMessage, sys_get_error(  ) );
}


// https://stackoverflow.com/a/31411628/12778316
static NTSTATUS(__stdcall *NtDelayExecution)(BOOL Alertable, PLARGE_INTEGER DelayInterval) = (NTSTATUS(__stdcall*)(BOOL, PLARGE_INTEGER)) GetProcAddress(GetModuleHandle("ntdll.dll"), "NtDelayExecution");
static NTSTATUS(__stdcall *ZwSetTimerResolution)(IN ULONG RequestedResolution, IN BOOLEAN Set, OUT PULONG ActualResolution) = (NTSTATUS(__stdcall*)(ULONG, BOOLEAN, PULONG)) GetProcAddress(GetModuleHandle("ntdll.dll"), "ZwSetTimerResolution");


// sleep for x milliseconds
void sys_sleep( float ms )
{
	static bool once = true;
	if (once)
	{
		ULONG actualResolution;
		ZwSetTimerResolution(1, true, &actualResolution);
		once = false;
	}

	LARGE_INTEGER interval{};
	interval.QuadPart = -1 * (int)(ms * 10000.0f);
	NtDelayExecution(false, &interval);
}

// very uh, windows only-ish i think
void* sys_get_console_window()
{
	// idk what this is actually doing lmao
	return GetStdHandle( -11 );
}


#endif // _WIN32