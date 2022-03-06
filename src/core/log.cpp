#include "core/log.h"
#include "core/console.h"

#include <iostream>

#include <SDL.h>

CONVAR( developer, 1 );

LogColor gCurrentColor = LogColor::Default;

/* Windows Specific Functions for console text colors.  */
#ifdef _WIN32

#include <Windows.h>


constexpr int Win32GetColor( LogColor color ) 
{
    switch ( color )
    {
        case LogColor::Black:
            return 0;

        case LogColor::DarkBlue:
            return FOREGROUND_BLUE;
        case LogColor::DarkGreen:
            return FOREGROUND_GREEN;
        case LogColor::DarkCyan:
            return FOREGROUND_GREEN | FOREGROUND_BLUE;
        case LogColor::DarkRed:
            return FOREGROUND_RED;
        case LogColor::DarkMagenta:
            return FOREGROUND_RED | FOREGROUND_BLUE;
        case LogColor::DarkYellow:
            return FOREGROUND_RED | FOREGROUND_GREEN;
        case LogColor::DarkGray:
            return FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;

        case LogColor::Gray:
            return FOREGROUND_INTENSITY;
        case LogColor::Blue:
            return FOREGROUND_INTENSITY | FOREGROUND_BLUE;
        case LogColor::Green:
            return FOREGROUND_INTENSITY | FOREGROUND_GREEN;
        case LogColor::Cyan:
            return FOREGROUND_INTENSITY | FOREGROUND_GREEN | FOREGROUND_BLUE;
        case LogColor::Red:
            return FOREGROUND_INTENSITY | FOREGROUND_RED;
        case LogColor::Magenta:
            return FOREGROUND_INTENSITY | FOREGROUND_RED | FOREGROUND_BLUE;
        case LogColor::Yellow:
            return FOREGROUND_INTENSITY | FOREGROUND_RED | FOREGROUND_GREEN;
        case LogColor::White:
            return FOREGROUND_INTENSITY | FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;

        case LogColor::Default:
        default:
            return 7;
    }
}

void Win32SetColor( LogColor color )
{
    // um
    static HANDLE handle = sys_get_console_window();

    if ( !handle )
    {
        LogMsg( "[Warning]: Failed to Get Console Window\n" );
        return;
    }

    BOOL result = SetConsoleTextAttribute( handle, Win32GetColor( color ) );

    if ( !result )
    {
        LogMsg( "[Warning]: Failed to set console color: %s\n", sys_get_error() );
    }
}

#elif __unix__

// TODO: setup the rest of the colors here
constexpr const char* UnixGetColor( LogColor color ) 
{
    switch ( color )
    {
    case LogColor::Red:
        return "\033[0;31m";
    case LogColor::DarkGreen:
        return "\033[0;32m";
    case LogColor::Green:
        return "\033[1;32m";
    case LogColor::Yellow:
        return "\033[0;33m";
    case LogColor::Blue:
        return "\033[0;34m";
    case LogColor::Cyan:
        return "\033[0;36m";
    case LogColor::Magenta:
        return "\033[1;35m";

    case LogColor::Default:
    default:
        return "\033[0m";
    }
}


void UnixSetColor( LogColor color )
{
    printf( UnixGetColor( color ) );
}

#endif


/* Lowest severity.  */
void LogMsg( const char *spFmt, ... ) 
{
    char pBuf[ 1024 ];
    va_list args;
    va_start( args, spFmt );
    vsprintf( pBuf, spFmt, args );
    Puts( pBuf );
    va_end( args );
}

/* Lowest severity - Color Option.  */
void LogMsg( LogColor color, const char *spFmt, ... )
{
    LogSetColor( color );

    char pBuf[ 1024 ];
    va_list args;
    va_start( args, spFmt );
    vsprintf( pBuf, spFmt, args );
    Puts( pBuf );
    va_end( args );

    LogSetColor( LogColor::Default );
}

/* Lowest severity, no format.  */
void LogPuts( const char *spBuf, LogColor color )
{
    if ( color != LogColor::Default )
        LogSetColor( color );

    Puts( spBuf );

    if ( color != LogColor::Default )
        LogSetColor( LogColor::Default );
}

/* Medium severity.  */
void LogWarn( const char *spFmt, ... )
{
    LogSetColor( LogColor::Yellow );

    char pBuf[ 1024 ];
    va_list args;
    va_start( args, spFmt );
    vsprintf( pBuf, spFmt, args );
    Print( "[Warning]: %s", pBuf );
    va_end( args );

    LogSetColor( LogColor::Default );
}

/* Medium severity.  */
void LogPutsWarn( const char *spBuf )
{
    LogSetColor( LogColor::Yellow );

    Print( "[Warning]: %s", spBuf );

    LogSetColor( LogColor::Default );
}

/* High severity.  */
/* TODO: maybe have this print to stderr? */
void LogError( const char *spFmt, ... )
{
    LogSetColor( LogColor::Red );

    char pBuf[ 1024 ];
    va_list args;
    va_start( args, spFmt );
    vsprintf( pBuf, spFmt, args );
    Print( "[Error]: %s", pBuf );
    va_end( args );

    LogSetColor( LogColor::Default );
}

/* Extreme severity.  */
void LogFatal( const char *spFmt, ... )
{
    LogSetColor( LogColor::Red );

    char pBuf[ 1024 ];
    va_list args;
    va_start( args, spFmt );
    vsprintf( pBuf, spFmt, args );

    // Print( "[Fatal]: %s", pBuf );
    fprintf( stderr, "[Fatal]: %s", pBuf );
    SDL_ShowSimpleMessageBox( SDL_MESSAGEBOX_ERROR, "Fatal Error", pBuf, NULL );
    throw std::runtime_error( pBuf );

    va_end( args );

    LogSetColor( LogColor::Default );
}

/* Dev only.  */
void LogDev( u8 sLvl, const char *spFmt, ... )
{
    if ( developer < sLvl )
        return;

    LogSetColor( LogColor::Cyan );

    char pBuf[ 1024 ];
    va_list args;
    va_start( args, spFmt );
    vsprintf( pBuf, spFmt, args );
    Puts( pBuf );
    va_end( args );

    LogSetColor( LogColor::Default );
}


/* Dev only.  */
void LogPutsDev( u8 sLvl, const char *spBuf )
{
    if ( developer > sLvl )
        return;

    LogSetColor( LogColor::Cyan );

    Puts( spBuf );

    LogSetColor( LogColor::Default );
}


void LogSetColor( LogColor color )
{
    gCurrentColor = color;

#ifdef _WIN32
    Win32SetColor( color );
#elif __unix__
    // technically works in windows 10, might try later and see if it's cheaper, idk
    UnixSetColor( color );
#endif
}


LogColor LogGetColor()
{
    return gCurrentColor;
}

