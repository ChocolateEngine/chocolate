#pragma once

typedef void* Module;


#ifdef _WIN32
	#define EXT_DLL ".dll"
	#define DLL_EXPORT __declspec(dllexport)
	#define DLL_IMPORT __declspec(dllimport)
#elif __linux__
	#define EXT_DLL ".so"
	#define DLL_EXPORT __attribute__((__visibility__("default")))
	#define DLL_IMPORT
#else
	#error "Library loading not setup for this platform"
#endif

#ifndef CORE_DLL
	#define CORE_API DLL_IMPORT
#else
	#define CORE_API DLL_EXPORT
#endif


using FResizeCallback = void( void );


// why is msvc like this with this dllexport/dllimport stuff aaaa
void            CORE_API    sys_init();
void            CORE_API    sys_shutdown();

Module          CORE_API    sys_load_library( const char* path );
void            CORE_API    sys_close_library( Module mod );
void            CORE_API*   sys_load_func( Module mod, const char* path );
const char      CORE_API*   sys_get_error();
void            CORE_API    sys_print_last_error( const char* userErrorMessage );

// sleep for x milliseconds
void            CORE_API    sys_sleep( float ms );

// console functions
void            CORE_API*   sys_get_console_window();
int             CORE_API    sys_allow_console_input();

// wait for a debugger to attach
void            CORE_API    sys_wait_for_debugger();
void            CORE_API    sys_debug_break();

int CORE_API                Sys_GetCoreCount();

// window management
#ifdef _WIN32
void            CORE_API*   Sys_CreateWindow( const char* spWindowName, int sWidth, int sHeight );
void            CORE_API    Sys_SetResizeCallback( FResizeCallback callback );
#endif
