#pragma once


#ifndef _WIN32
	typedef void* Module;
#endif

// oh yeah
#ifdef _WIN32
	#include <libloaderapi.h>
	#include <errhandlingapi.h>

	#define LOAD_LIBRARY(path) LoadLibrary(path)
	#define CLOSE_LIBRARY(handle) FreeLibrary(handle)
	// #define GET_ERROR() GetLastError()
	#define GET_ERROR() "*dies of cringe*"
	#define LOAD_FUNC GetProcAddress
	#define EXT_DLL ".dll"

	#define DLL_EXPORT __declspec(dllexport)

	typedef HMODULE Module;
#elif __linux__
	#include <dlfcn.h>

	#define LOAD_LIBRARY(path) dlopen(path, RTLD_LAZY)
	#define CLOSE_LIBRARY(handle) dlclose(handle)
	#define GET_ERROR() dlerror()
	#define LOAD_FUNC dlsym
	#define EXT_DLL ".so"

	#define DLL_EXPORT

#else
	#error "Library loading not setup for this platform"
#endif


void PrintLastError( const char* userErrorMessage );

