#include "core/platform.h"
#include "core/log.h"

#if __unix__

#include <stdio.h>
#include <dlfcn.h>
#include <csignal>
#include <unistd.h>

void sys_init()
{
}

Module sys_load_library( const char* path )
{
	return (Module)dlopen( path, RTLD_NOW );
}

void sys_close_library( Module mod )
{
	if ( !mod )
		return;

	dlclose( mod );
}

void* sys_load_func( Module mod, const char* path )
{
	return dlsym( mod, path );
}

const char* sys_get_error()
{
	return dlerror();
}


const char* sys_get_cwd()
{
	return getcwd( 0, 0 );
}

// sleep for x milliseconds
void sys_sleep( float ms )
{
    usleep( ms * 1000 );
}


void sys_wait_for_debugger()
{
	// TODO: look into this
	// https://stackoverflow.com/a/24969863
    // raise( SIGSTOP );
}


void sys_debug_break()
{
	//printf( "\n *** NEED TO CHECK IF A DEBUGGER IS PRESENT TO NOT LOCK UP THE PROGRAM, IF NONE PRESENT, THEN DONT WAIT FOR ONE !!!\n\n" );
    //raise( SIGSTOP );
}


void sys_shutdown()
{
}


int sys_get_core_count()
{
    int numCPU = sysconf(_SC_NPROCESSORS_ONLN);
	return numCPU;
}


void* sys_create_window( const char* spWindowName, int sWidth, int sHeight, bool sMaximize )
{
#if 1
    // UNUSED AT THE MOMENT, WILL USE LATER
	return nullptr;
#else
    // lmao just pass in vulkan here
	int flags = SDL_WINDOW_VULKAN | SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI;

	if ( sMaximize )
		flags |= SDL_WINDOW_MAXIMIZED;

	SDL_Window window = SDL_CreateWindow( spWindowName, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
	                                  sWidth, sHeight, flags );

	return window;
#endif
}


// CONCMD_VA( sys_stack_info, "Print the current stack usage" )
// {
	// struct rlimit limit;
	// getrlimit( RLIMIT_STACK, &limit );

	// Log_DevF( 1, "Stack Limit: %ld bytes - Stack Max: %lu bytes\n", limit.rlim_cur, limit.rlim_max );
// }



#endif /* __unix__  */