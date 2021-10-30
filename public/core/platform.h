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

// actually NO, just use SDL2's library loading stuff smh my head

// very quake-esque style code here with the sys prefix lol

Module          sys_load_library( const char* path );
void            sys_close_library( Module mod );
void*           sys_load_func( Module mod, const char* path );
const char*     sys_get_error(  );
void            sys_print_last_error( const char* userErrorMessage );


// maybe use sys_print_last_error if SDL_GetError isn't verbose enough on win32?


