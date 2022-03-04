#include "core/platform.h"
#include "core/log.h"

#if __unix__

#include <stdio.h>
#include <dlfcn.h>

Module          sys_load_library( const char* path )
{
    auto pHandle = dlopen( path, RTLD_LAZY );
    if ( !pHandle )
    {
        LogDev(  1, "Failed to load library: %s!\n", dlerror() );
        return nullptr;
    }
    return ( Module )pHandle;
}

void            sys_close_library( Module mod )
{
    if ( !mod )
        return;

    dlclose( mod );
}

void*           sys_load_func( Module mod, const char* path )
{
    auto pFunc = dlsym( mod, path );
    if ( !pFunc )
    {
        LogDev( 1, "Failed to load function: %s!\n", dlerror() );
        return nullptr;
    }
    return pFunc;
}

const char*     sys_get_error()
{
    return dlerror();
}

void            sys_print_last_error( const char* userErrorMessage )
{
    fprintf( stderr, "Error: %s\n%s\n", userErrorMessage, sys_get_error(  ) );
}

#endif /* __unix__  */