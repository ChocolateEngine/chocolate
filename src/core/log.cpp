#include "core/log.h"
#include "core/console.h"

#include <iostream>

#include <SDL.h>

/* Lowest severity.  */
void LogMsg( const char *spFmt, ... ) 
{
    char pBuf[ 1024 ];
    va_list args;
    va_start( args, spFmt );
    vsprintf( pBuf, spFmt, args );
    PrintFast( pBuf );
    va_end( args );
}

/* Medium severity.  */
void LogWarn( const char *spFmt, ... )
{
    char pBuf[ 1024 ];
    va_list args;
    va_start( args, spFmt );
    vsprintf( pBuf, spFmt, args );
    Print( "[Warning]: %s", pBuf );
    va_end( args );
}

/* High severity.  */
void LogError( const char *spFmt, ... )
{
    char pBuf[ 1024 ];
    va_list args;
    va_start( args, spFmt );
    vsprintf( pBuf, spFmt, args );
    Print( "[Error]: %s", pBuf );
    va_end( args );
}

/* Extreme severity.  */
void LogFatal( const char *spFmt, ... )
{
    char pBuf[ 1024 ];
    va_list args;
    va_start( args, spFmt );
    vsprintf( pBuf, spFmt, args );

    Print( "[Fatal]: %s", pBuf );
    SDL_ShowSimpleMessageBox( SDL_MESSAGEBOX_ERROR, "Fatal Error", pBuf, NULL );
    throw std::runtime_error( pBuf );

    va_end( args );
}

/* Dev only.  */
void LogDev( u8 sLvl, const char *spFmt, ... )
{
    char pBuf[ 1024 ];
    va_list args;
    va_start( args, spFmt );
    vsprintf( pBuf, spFmt, args );
    PrintFast( pBuf );
    va_end( args );
}
