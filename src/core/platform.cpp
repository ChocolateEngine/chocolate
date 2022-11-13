#ifdef _WIN32

#include <stdio.h>
#include <stdlib.h>
//#include <string>

#include <wtypes.h>  // HWND
#include <libloaderapi.h>
//#include <errhandlingapi.h>
//#include <VersionHelpers.h>
#include <winbase.h>  // FormatMessage

// bruh: https://stackoverflow.com/questions/25267272/win32-enable-visual-styles-in-dll
#pragma comment(linker,"\"/manifestdependency:type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*'    publicKeyToken='6595b64144ccf1df' language='*'\"")


#include "core/platform.h"


bool            gIsWindows10 = false;
OSVERSIONINFOEX gOSVer{};

HMODULE ghInst = 0;

HANDLE gConOut = INVALID_HANDLE_VALUE;
HANDLE gConIn  = INVALID_HANDLE_VALUE;

HANDLE ghActCtx = INVALID_HANDLE_VALUE;
ACTCTX gActCtx;
ULONG_PTR gActCookie;


// super based
typedef void (WINAPI * RtlGetVersion_FUNC) (OSVERSIONINFOEXW *);

BOOL win32_get_version( OSVERSIONINFOEX* os )
{
	HMODULE hMod;
	RtlGetVersion_FUNC func;

#ifdef UNICODE
	OSVERSIONINFOEXW * osw = os;
#else
	OSVERSIONINFOEXW o;
	OSVERSIONINFOEXW * osw = &o;
#endif

	hMod = LoadLibrary(TEXT("ntdll.dll"));

	if (hMod)
	{
		func = (RtlGetVersion_FUNC)GetProcAddress(hMod, "RtlGetVersion");
		if (func == 0) {
			FreeLibrary(hMod);
			return FALSE;
		}
		ZeroMemory(osw, sizeof (*osw));
		osw->dwOSVersionInfoSize = sizeof (*osw);
		func(osw);

#ifndef UNICODE
		os->dwBuildNumber = osw->dwBuildNumber;
		os->dwMajorVersion = osw->dwMajorVersion;
		os->dwMinorVersion = osw->dwMinorVersion;
		os->dwPlatformId = osw->dwPlatformId;
		os->dwOSVersionInfoSize = sizeof (*os);
		DWORD sz = sizeof (os->szCSDVersion);
		WCHAR * src = osw->szCSDVersion;
		unsigned char * dtc = (unsigned char *)os->szCSDVersion;
		while (*src)
			*dtc++ = (unsigned char)* src++;
		*dtc = '\0';
#endif

	}
	else
	{
		fputs( "Platform: Failed to get ntdll.dll to check windows version\n", stdout );
		return FALSE;
	}

	FreeLibrary(hMod);
	return TRUE;
}


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

const char* sys_get_error()
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
	fprintf( stderr, "Error: %s\n%s\n", userErrorMessage, sys_get_error() );
}


// https://stackoverflow.com/a/31411628/12778316
static NTSTATUS(__stdcall *NtDelayExecution)(BOOL Alertable, PLARGE_INTEGER DelayInterval) = (NTSTATUS(__stdcall*)(BOOL, PLARGE_INTEGER)) GetProcAddress(GetModuleHandle("ntdll.dll"), "NtDelayExecution");
static NTSTATUS(__stdcall *ZwSetTimerResolution)(IN ULONG RequestedResolution, IN BOOLEAN Set, OUT PULONG ActualResolution) = (NTSTATUS(__stdcall*)(ULONG, BOOLEAN, PULONG)) GetProcAddress(GetModuleHandle("ntdll.dll"), "ZwSetTimerResolution");


// sleep for x milliseconds
void sys_sleep( float ms )
{
	static bool once = true;
	if ( once )
	{
		ULONG actualResolution;
		ZwSetTimerResolution( 1, true, &actualResolution );
		once = false;
	}

	LARGE_INTEGER interval{};
	interval.QuadPart = -1 * (int)( ms * 10000.0f );
	NtDelayExecution( false, &interval );
}


bool win32_init_console( HANDLE conOut, HANDLE conIn )
{
	// DOESN'T WORK YET
#if 0
	if ( !gIsWindows10 )
		return false;

	DWORD dwOriginalOutMode = 0;
	DWORD dwOriginalInMode = 0;
	if (!GetConsoleMode(conOut, &dwOriginalOutMode))
	{
		return false;
	}
	if (!GetConsoleMode(conIn, &dwOriginalInMode))
	{
		return false;
	}

	DWORD dwRequestedOutModes = 0;
	DWORD dwRequestedInModes = ENABLE_WINDOW_INPUT | ENABLE_QUICK_EDIT_MODE | ENABLE_LINE_INPUT | ENABLE_ECHO_INPUT;

	if ( gIsWindows10 )
	{
		dwRequestedOutModes |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
		dwRequestedInModes |= ENABLE_VIRTUAL_TERMINAL_INPUT;
	}

	DWORD dwOutMode = dwOriginalOutMode | dwRequestedOutModes;
	if (!SetConsoleMode(conOut, dwOutMode))
	{
		// we failed to set both modes, try to step down mode gracefully.
#if 0
		dwRequestedOutModes = ENABLE_VIRTUAL_TERMINAL_PROCESSING;
		dwOutMode = dwOriginalOutMode | dwRequestedOutModes;
		if (!SetConsoleMode(conOut, dwOutMode))
#endif
		{
			// Failed to set any VT mode, can't do anything here.
			return false;
		}
	}

	DWORD dwInMode = dwOriginalInMode | dwRequestedInModes;
	if (!SetConsoleMode(conIn, dwInMode))
	{
		// Failed to set VT input mode, can't do anything here.
		return false;
	}
#endif
	return true;
}


// should probably use AllocConsole later, idk
// on linux though, idk
void* sys_get_console_window()
{
	return gConOut;
}


int sys_allow_console_input()
{
	if ( gConOut == INVALID_HANDLE_VALUE )
		return false;



	




	//SetConsoleMode( con, );

	return true;
}


void sys_wait_for_debugger()
{
	Log_Msg( "WAITING FOR DEBUGGER\n" );

	while( !::IsDebuggerPresent() )
		::Sleep( 100 ); // to avoid 100% CPU load

	DebugBreak();
}


void sys_init()
{
	// detect windows version
	BOOL ret = win32_get_version( &gOSVer );

	gIsWindows10 = gOSVer.dwMajorVersion >= 10;

	// Get Module Handle because we don't use WinMain at the moment (probably should tbh, idk)
	// ghInst = GetModuleHandle( 0 );
	ghInst = GetModuleHandle( "core.dll" );


	// Init Console
	gConOut = GetStdHandle( STD_OUTPUT_HANDLE );
	if ( gConOut == INVALID_HANDLE_VALUE )
	{
		sys_print_last_error( "Failed to get stdout console\n" );
		return;
	}

	gConIn = GetStdHandle( STD_INPUT_HANDLE );
	if ( gConIn == INVALID_HANDLE_VALUE )
	{
		sys_print_last_error( "Failed to get stdin console\n" );
		return;
	}

	if ( !win32_init_console( gConOut, gConIn ) )
		sys_print_last_error( "Failed to Init System Console\n" );

	
	// Init Theme Context (why windows???)
	ZeroMemory( &gActCtx, sizeof( gActCtx ) );

	gActCtx.cbSize = sizeof(gActCtx);
	gActCtx.hModule = ghInst;
	gActCtx.lpResourceName = MAKEINTRESOURCE(2);
	gActCtx.dwFlags = ACTCTX_FLAG_HMODULE_VALID | ACTCTX_FLAG_RESOURCE_NAME_VALID;

	ghActCtx = CreateActCtx(&gActCtx);
	if (ghActCtx != INVALID_HANDLE_VALUE)
	{
		ActivateActCtx(ghActCtx, &gActCookie);
		// MessageBox(NULL, TEXT("Styled message box"), NULL, MB_OK | MB_ICONERROR);
	}
	else
	{
		sys_print_last_error( "Failed to create theme context" );
	}
}


void sys_shutdown()
{
	// Close Theme Context (why windows???)
	if (ghActCtx != INVALID_HANDLE_VALUE)
	{
		DeactivateActCtx(0, gActCookie);
		ReleaseActCtx(ghActCtx);
	}
}


size_t stackavail()
{
	// page range
	MEMORY_BASIC_INFORMATION mbi;

	// get range
	VirtualQuery( (PVOID)&mbi, &mbi, sizeof( mbi ) );

	// subtract from top (stack grows downward on win)
	return (UINT_PTR)&mbi - (UINT_PTR)mbi.AllocationBase;
}


CONCMD_VA( sys_stack_info, "Print the current stack usage" )
{
	// only windows 7 or later
	NT_TIB* tib        = (NT_TIB*)NtCurrentTeb();
	DWORD   stackBase  = (DWORD)tib->StackBase;
	DWORD   stackLimit = (DWORD)tib->StackLimit;

	Log_DevF( 1, "Stack Base: %zu bytes - Stack Limit: %zu\n", stackBase, stackLimit );

	Log_DevF( 1, "stackavail: %zu bytes\n", stackavail() );
}


#endif // _WIN32