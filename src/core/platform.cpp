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


#include "core/core.h"


static bool     gExceptionDebugger = false;


bool            g_win_10_or_later  = false;
OSVERSIONINFOEX gOSVer{};

HMODULE         ghInst   = 0;
ATOM            gWindowClass = 0;

HANDLE          gConOut  = INVALID_HANDLE_VALUE;
HANDLE          gConIn   = INVALID_HANDLE_VALUE;

HANDLE          ghActCtx = INVALID_HANDLE_VALUE;
ACTCTX          gActCtx;
ULONG_PTR       gActCookie;

#define CH_MEM_DEBUG 0


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
		// DWORD sz = sizeof (os->szCSDVersion);
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


static const char* Win32_GetExceptionName( DWORD code )
{
	static char buf[ 26 ];

	switch ( code )
	{
		case EXCEPTION_ACCESS_VIOLATION:         return "ACCESS_VIOLATION";
		case EXCEPTION_DATATYPE_MISALIGNMENT:    return "DATATYPE_MISALIGNMENT";
		case EXCEPTION_ARRAY_BOUNDS_EXCEEDED:    return "ARRAY_BOUNDS_EXCEEDED";
		case EXCEPTION_PRIV_INSTRUCTION:         return "PRIV_INSTRUCTION";
		case EXCEPTION_IN_PAGE_ERROR:            return "IN_PAGE_ERROR";
		case EXCEPTION_ILLEGAL_INSTRUCTION:      return "ILLEGAL_INSTRUCTION";
		case EXCEPTION_NONCONTINUABLE_EXCEPTION: return "NONCONTINUABLE_EXCEPTION";
		case EXCEPTION_STACK_OVERFLOW:           return "STACK_OVERFLOW";
		case EXCEPTION_INVALID_DISPOSITION:      return "INVALID_DISPOSITION";
		case EXCEPTION_GUARD_PAGE:               return "GUARD_PAGE";
		case EXCEPTION_INVALID_HANDLE:           return "INVALID_HANDLE";
		default: break;
	}

	sprintf( buf, "0x%08X", (unsigned int)code );
	return buf;
}


static LONG WINAPI Win32_ExceptionFilter( struct _EXCEPTION_POINTERS* ExceptionInfo )
{
	if ( ExceptionInfo->ExceptionRecord->ExceptionCode != EXCEPTION_BREAKPOINT )
	{
		char  msg[ 128 ];
		byte *addr, *base;
		bool  vma;

		addr = (byte*)ExceptionInfo->ExceptionRecord->ExceptionAddress;
		base = (byte*)GetModuleHandle( NULL );

		if ( addr >= base )
		{
			addr = (byte*)( addr - base );
			vma  = true;
		}
		else
		{
			vma = false;
		}

		sprintf( msg, "Exception Code: %s\nException Address: %p%s",
		         Win32_GetExceptionName( ExceptionInfo->ExceptionRecord->ExceptionCode ),
		         addr, vma ? " (VMA)" : "" );

		Log_ErrorF( "Unhandled exception caught\n%s", msg );

		if ( gExceptionDebugger )
			sys_wait_for_debugger();
	}

	return EXCEPTION_EXECUTE_HANDLER;
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
	snprintf( message, 512, "Win32 API Error %ud: %s", errorID, strErrorMessage );

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
	if ( !g_win_10_or_later )
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

	if ( g_win_10_or_later )
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



	




	//SetConsoleMode( con, );^

	return true;
}


void sys_wait_for_debugger()
{
	Log_Dev( 1, "WAITING FOR DEBUGGER\n" );

	while( !::IsDebuggerPresent() )
		::Sleep( 100 ); // to avoid 100% CPU load

	DebugBreak();
}


void sys_debug_break()
{
	if ( IsDebuggerPresent() )
		DebugBreak();
}


static FResizeCallback* gResizeCallbackFunc = nullptr;

LRESULT __stdcall Win32_WindowProc( HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
	// ImGui_ImplWin32_WndProcHandler( hwnd, uMsg, wParam, lParam );

	#if 0
	switch ( uMsg )
	{
		case WM_INPUT:
		{
			RAWINPUT  input;
			UINT      szData = sizeof( input ), szHeader = sizeof( RAWINPUTHEADER );
			HRAWINPUT handle = reinterpret_cast< HRAWINPUT >( lParam );

			GetRawInputData( handle, RID_INPUT, &input, &szData, szHeader );
			if ( input.header.dwType == RIM_TYPEMOUSE )
			{
				// Here input.data.mouse.ulRawButtons is 0 at all times.
			}

			printf( "WM_INPUT\n" );
			break;
		}
		default:
			break;
	}
	#endif

	// resize events
	switch ( uMsg )
	{
		case WM_DPICHANGED:
		case WM_SIZE:
		{
			// hack
			static bool firstTime = true;
			if ( firstTime )
			{
				firstTime = false;
				break;
			}

			if ( gResizeCallbackFunc )
				gResizeCallbackFunc( hwnd );

			break;
		}
	}

	return DefWindowProc( hwnd, uMsg, wParam, lParam );
}


void sys_init()
{
	setlocale( LC_ALL, "en_US.UTF-8" );

	// detect windows version
	BOOL ret = win32_get_version( &gOSVer );

	g_win_10_or_later = gOSVer.dwMajorVersion >= 10;

	// Get Module Handle because we don't use WinMain at the moment (probably should tbh, idk)
	// ghInst = GetModuleHandle( 0 );
	ghInst = GetModuleHandle( "ch_core.dll" );

	SetUnhandledExceptionFilter( Win32_ExceptionFilter );

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

	// very, very early windows 10
	if ( gOSVer.dwMajorVersion == 6 && gOSVer.dwMinorVersion == 4 )
	{
		Log_DevF( 1, "runnig windows 9 :D\n" );
	}

	Log_DevF( 1, "Windows Version: %d.%d.%d\n", gOSVer.dwMajorVersion, gOSVer.dwMinorVersion, gOSVer.dwBuildNumber );

	// List CPU Version
	cpu_info_t cpu_info = Sys_GetCpuInfo();

	Log_DevF( 1, "CPU Vendor ID: %s\n", cpu_info.vendor_id );
	Log_DevF( 1, "CPU Model Name: %s\n", cpu_info.model_name );

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

	// Set Process Priority
	bool argPriorityHigh = Args_Register( "Set the engine to high priority", "-high" );
	bool argPriorityLow  = Args_Register( "Set the engine to low priority", "-low" );

	if ( argPriorityHigh && argPriorityLow )
	{
		Log_Warn( "High and Low Process Priority in Arguments, picking Default Process Priority\n" );
	}
	else if ( argPriorityHigh || argPriorityLow )
	{
		HANDLE hProcess   = GetCurrentProcess();
		DWORD  dwPriority = GetPriorityClass( hProcess );

		// TODO: check to see if we have THREAD_SET_INFORMATION or THREAD_SET_LIMITED_INFORMATION access right?
		// https://learn.microsoft.com/en-us/windows/win32/api/processthreadsapi/nf-processthreadsapi-setthreadpriority

		if ( argPriorityHigh )
		{
			Log_Dev( 1, "Setting Engine to High Process Priority\n" );

			if ( dwPriority != HIGH_PRIORITY_CLASS )
			{
				SetPriorityClass( hProcess, HIGH_PRIORITY_CLASS );
				SetThreadPriority( hProcess, THREAD_PRIORITY_ABOVE_NORMAL );
			}
		}
		else if ( argPriorityLow )
		{
			Log_Dev( 1, "Setting Engine to Low Process Priority\n" );

			if ( dwPriority != IDLE_PRIORITY_CLASS )
			{
				SetPriorityClass( hProcess, IDLE_PRIORITY_CLASS );
				SetThreadPriority( hProcess, THREAD_PRIORITY_BELOW_NORMAL );
			}
		}
	}

	gExceptionDebugger = Args_Register( "Wait for debugger on catching a fatal exception", "-exception-debugger" );

#if CH_MEM_DEBUG
	// Get the current state of the flag
	// and store it in a temporary variable
	int tmpFlag = _CrtSetDbgFlag( _CRTDBG_REPORT_FLAG );

	// Turn On (OR) - Keep freed memory blocks in the
	// heap's linked list and mark them as freed
	tmpFlag |= _CRTDBG_DELAY_FREE_MEM_DF;

	// Turn Off (AND) - prevent _CrtCheckMemory from
	// being called at every allocation request
	tmpFlag &= ~_CRTDBG_CHECK_ALWAYS_DF;

	// Set the new state for the flag
	_CrtSetDbgFlag( tmpFlag );
#endif

	// Create Window Class
	WNDCLASSEX wc = { 0 };
	ZeroMemory( &wc, sizeof( wc ) );

	wc.cbClsExtra    = 0;
	wc.cbSize        = sizeof( wc );
	wc.cbWndExtra    = 0;
	wc.hInstance     = GetModuleHandle( NULL );
	wc.hCursor       = LoadCursor( NULL, IDC_ARROW );
	wc.hIcon         = LoadIcon( NULL, IDI_APPLICATION );
	wc.hIconSm       = LoadIcon( 0, IDI_APPLICATION );
	wc.hbrBackground = (HBRUSH)GetStockObject( BLACK_BRUSH );  // does this affect perf? i hope not
	wc.style         = CS_HREDRAW | CS_VREDRAW;                // redraw if size changes
	wc.lpszClassName = "chocolate_engine";
	wc.lpszMenuName  = 0;
	wc.lpfnWndProc   = Win32_WindowProc;

	gWindowClass = RegisterClassEx( &wc );

	if ( gWindowClass == 0 )
	{
		// somehow not running on windows nt?
		Log_FatalF(" Failed to create window class\n" );
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


int Sys_GetCoreCount()
{
	SYSTEM_INFO sysinfo;
	GetSystemInfo( &sysinfo );
	int numCPU = sysinfo.dwNumberOfProcessors;
	return numCPU;
}


void Sys_SetResizeCallback( FResizeCallback callback )
{
	gResizeCallbackFunc = callback;
}


void* Sys_CreateWindow( const char* spWindowName, int sWidth, int sHeight, bool sMaximize )
{
	const LPTSTR _ClassName( MAKEINTATOM( gWindowClass ) );

	DWORD        dwStyle   = WS_VISIBLE | WS_OVERLAPPEDWINDOW | WS_EX_CONTROLPARENT;
	DWORD        dwExStyle = 0;

	RECT         rect;
	rect.left   = 0;
	rect.top    = 0;
	rect.right  = sWidth;
	rect.bottom = sHeight;

	// thanks win32
	// According to the wiki, this function
	// "Calculates the required size of the window rectangle, based on the desired size of the client rectangle.
	// The window rectangle can then be passed to the CreateWindowEx function to create a window whose client area is the desired size."
	// 
	// Without that, the window ends up smaller than it actually should be
	// ex. 1280x720 becomes 1266x713
	// https://stackoverflow.com/questions/34583160/winapi-createwindow-function-creates-smaller-windows-than-set
	// https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-adjustwindowrectex
	AdjustWindowRectEx( &rect, dwStyle, 0, 0 );

	HWND window = CreateWindowExA(
	  dwExStyle,
	  _ClassName,
	  spWindowName,
	  dwStyle,
	  CW_USEDEFAULT,
	  CW_USEDEFAULT,
	  rect.right - rect.left, rect.bottom - rect.top,
	  NULL, NULL,
	  GetModuleHandle( NULL ),
	  nullptr );

	if ( !window )
	{
		sys_print_last_error( "Failed to create window\n" );
		return nullptr;
	}

	if ( sMaximize )
	{
		ShowWindow( window, SW_MAXIMIZE );
	}

	return window;
}


// TODO: https://learn.microsoft.com/en-us/windows/win32/procthread/creating-a-child-process-with-redirected-input-and-output?redirectedfrom=MSDN
int Sys_Execute( const char* spFile, const char* spArgs )
{
	SHELLEXECUTEINFO ShExecInfo = { 0 };
	ShExecInfo.cbSize           = sizeof( SHELLEXECUTEINFO );
	ShExecInfo.fMask            = SEE_MASK_NOCLOSEPROCESS;
	ShExecInfo.hwnd             = NULL;
	ShExecInfo.lpVerb           = NULL;
	ShExecInfo.lpFile           = spFile;
	ShExecInfo.lpParameters     = spArgs;
	ShExecInfo.lpDirectory      = NULL;
	ShExecInfo.nShow            = SW_SHOW;
	ShExecInfo.hInstApp         = NULL;

	BOOL ret = ShellExecuteExA( &ShExecInfo );

	if ( ret == FALSE || ShExecInfo.hProcess == INVALID_HANDLE_VALUE )
		return -1;

	WaitForSingleObject( ShExecInfo.hProcess, INFINITE );

	DWORD exitCode = 0;
	ret            = GetExitCodeProcess( ShExecInfo.hProcess, &exitCode );

	return (int)exitCode;
}


int Sys_ExecuteV( const char* spFile, const char* spArgs, ... )
{
	va_list args;
	va_start( args, spArgs );
	ch_string string = ch_str_copy_v( spArgs, args );
	va_end( args );

	if ( string.data == nullptr )
		return -1;

	int ret = Sys_Execute( spFile, string.data );

	ch_str_free( string.data );
	return ret;
}


#if 0
uchar* Sys_ToWideChar( const char* spStr, int sSize )
{
	if ( spStr == nullptr )
		return nullptr;

	// check if the string is null-terminated
	if ( sSize == -1 )
		sSize = (int)strlen( spStr );

	bool     nullTerm = spStr[ sSize - 1 ] == '\0';
	size_t   len      = nullTerm ? sSize : sSize + 1;
	size_t   memSize  = len * sizeof( wchar_t );

	wchar_t* out      = ch_malloc< wchar_t >( memSize );

	// the following function converts the UTF-8 filename to UTF-16 (WCHAR) nameW
	int      outLen   = MultiByteToWideChar( CP_UTF8, 0, spStr, -1, out, len );

	if ( outLen > 0 )
		return out;

	ch_free( out );
	return nullptr;
}


char* Sys_ToMultiByte( const uchar* spStr, int sSize )
{
	if ( spStr == nullptr )
		return nullptr;

	// check if the string is null-terminated
	if ( sSize == -1 )
		sSize = (int)wcslen( spStr );

	bool     nullTerm = spStr[ sSize - 1 ] == L'\0';
	size_t   len      = nullTerm ? sSize : sSize + 1;
	size_t   memSize  = len * sizeof( char );

	char*    out      = ch_malloc< char >( memSize );

	// the following function converts the UTF-16 (WCHAR) filename to UTF-8 name
	int      outLen   = WideCharToMultiByte( CP_UTF8, 0, spStr, -1, out, len, NULL, NULL );

	if ( outLen > 0 )
	{
		ch_str_add( out );
		return out;
	}

	ch_free( out );
	return nullptr;
}


void Sys_FreeConvertedString( const uchar* spStr )
{
	ch_str_remove( spStr );
	ch_free( (void*)spStr );
}


void Sys_FreeConvertedString( const char* spStr )
{
	ch_str_remove( spStr );
	ch_free( (void*)spStr );
}
#endif


void Sys_CheckHeap()
{
	int result = _heapchk();

	switch ( result )
	{
		case _HEAPEMPTY:
			Log_Msg( "Heap Empty\n" );
			break;

		case _HEAPOK:
			Log_Msg( "Heap OK\n" );
			break;

		case _HEAPBADBEGIN:
			Log_Msg( "Heap Bad Begin\n" );
			break;

		case _HEAPBADNODE:
			Log_Msg( "Heap Bad Node\n" );
			break;

		case _HEAPEND:
			Log_Msg( "Heap End\n" );
			break;

		case _HEAPBADPTR:
			Log_Msg( "Heap Bad Pointer\n" );
			break;
	}

	CH_ASSERT( result == _HEAPOK );
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