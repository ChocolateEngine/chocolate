#include "launcher_base.h"
#include "core/util.h"
#include <stdio.h>
#include <string>

#if CH_USE_MIMALLOC
  #include "mimalloc-new-delete.h"
#endif

Module sdl2  = 0;
Module core  = 0;
Module imgui = 0;
Module app   = 0;


#ifdef _WIN32
#include <Windows.h>
#include <direct.h>

Module sys_load_library( const char* path )
{
	return (Module)LoadLibraryA( path );
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

	if ( errorID == 0 )
		return "";  // No error message

	LPSTR strErrorMessage = NULL;

	FormatMessageA(
	  FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS | FORMAT_MESSAGE_ARGUMENT_ARRAY | FORMAT_MESSAGE_ALLOCATE_BUFFER,
	  NULL,
	  errorID,
	  0,
	  (LPSTR)&strErrorMessage,
	  0,
	  NULL );

	//std::string message;
	//message.resize(512);
	//snprintf( message.data(), 512, "Win32 API Error %d: %s", errorID, strErrorMessage );

	static char message[ 512 ];
	memset( message, 512, 0 );
	snprintf( message, 512, "Win32 API Error %ud: %s", errorID, strErrorMessage );

	// Free the Win32 string buffer.
	LocalFree( strErrorMessage );

	return message;
}

#elif __unix__
#include <string.h>
#include <unistd.h>
#include <dlfcn.h>	

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

#else
	#error "No Library Loading Code"
#endif


void unload_objects()
{
	if ( app ) sys_close_library( app );
	if ( core ) sys_close_library( core );
	if ( imgui ) sys_close_library( imgui );
	if ( sdl2 ) sys_close_library( sdl2 );
}


bool load_object( Module* mod, const char* path )
{
	if ( *mod = sys_load_library( path ) )
		return true;

	fprintf( stderr, "Failed to load %s: %s\n", path, sys_get_error() );
	unload_objects();

	return false;
}


void* load_func( Module module, const char* name )
{
	void* func = sys_load_func( module, name );
	if ( !func )
	{
		fprintf( stderr, "Error Loading Function \"%s\": %s\n", name, sys_get_error() );
		unload_objects();
		return nullptr;
	}

	return func;
}


// hmmm, this does work, but should i use it...
// bool load_func2( auto& func, Module module, const char* name )
// {
// 	func = sys_load_func( module, name );
// 	if ( !func )
// 	{
// 		fprintf( stderr, "Error Loading Function \"%s\": %s\n", name, sys_get_error() );
// 		unload_objects();
// 		return false;
// 	}
// 
// 	return func;
// }


#if CH_USE_MIMALLOC
// ensure mimalloc is loaded
struct ForceMiMalloc_t
{
	ForceMiMalloc_t()
	{
		mi_version();

  #if _DEBUG
		mi_option_enable( mi_option_show_errors );
		mi_option_enable( mi_option_show_stats );
		mi_option_enable( mi_option_verbose );
  #endif
	}
};

static ForceMiMalloc_t forceMiMalloc;
#endif


int start( int argc, char *argv[], const char* app_path, const char* module_name )
{
	int  ( *app_init )()                                                = 0;
	int  ( *core_init )( int argc, char* argv[], const char* app_path ) = 0;
	void ( *core_exit )( bool write_archive )                           = 0;

	if ( load_object( &sdl2, "bin/" CH_PLAT_FOLDER "/SDL2" EXT_DLL ) == -1 )
		return -1;

	if ( load_object( &core, "bin/" CH_PLAT_FOLDER "/ch_core" EXT_DLL ) == -1 )
		return -1;

	if ( load_object( &imgui, "bin/" CH_PLAT_FOLDER "/imgui" EXT_DLL ) == -1 )
		return -2;

	*(void**)( &core_init ) = load_func( core, "core_init" );
	if ( !core_init )
		return -3;

	*(void**)( &core_exit ) = load_func( core, "core_exit" );
	if ( !core_exit )
		return -4;

	// MUST LOAD THIS FIRST BEFORE THE APP'S DLL TO REGISTER LAUNCH ARGUMENTS, AND PREPARE EVERYTHING ELSE
	int core_ret = core_init( argc, argv, app_path );

	// Failed to initialize core systems
	if ( core_ret != 0 )
	{
		core_exit( false );
		unload_objects();
		return core_ret;
	}

	// Now we can load the app module
	{
		char name[ 512 ] = {};
		
		strcat( name, "../" );  // account for the path change in core_init()
		strcat( name, app_path );
		strcat( name, "/bin/" CH_PLAT_FOLDER "/" );
		strcat( name, module_name );
		strcat( name, EXT_DLL );

		if ( load_object( &app, name ) == -1 )
			return -5;
	}

	// grab the init function from the app
	*(void**)( &app_init ) = load_func( app, "app_init" );
	if ( !app_init )
		return -6;

	// finally run the app
	int app_ret = app_init();
	core_exit( app_ret == 0 );

	unload_objects();

	return 0;
}
