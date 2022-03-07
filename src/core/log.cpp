#include "core/log.h"
#include "core/console.h"

#include <iostream>
#include <mutex>

#include <SDL.h>

CONVAR( developer, 1 );

LogColor gCurrentColor = LogColor::Default;

enum class LogLevel
{
	Normal,
	Dev,
	Warning,
	Error,
    Fatal
};

LogChannel gGeneralChannel = INVALID_LOG_CHANNEL;
static std::mutex gLogMutex;

class LogSystem
{
    struct Log
    {
        LogChannel aChannel;
        LogLevel aLevel;
        std::string aMessage;
        std::string aFormatted;
    };

public:
    LogSystem()
    {
        gGeneralChannel = RegisterChannel( "General", LogColor::Default );
    }

    LogChannel RegisterChannel( const char *sName, LogColor sColor )
    {
        for ( int i = 0; i < aChannels.size(); i++ )
        {
            LogChannel_t* channel = &aChannels[i];
            if ( channel->aName != sName )
                continue;
            
            return i;
        }

        aChannels.push_back( { true, sName, sColor } );
        return (LogChannel)aChannels.size() - 1;
    }

    LogChannel_t *GetChannel( LogChannel sChannel )
    {
        if ( sChannel >= aChannels.size() )
            return nullptr;

        return &aChannels[sChannel];
    }

    LogChannel_t* GetChannel( const char* sChannel )
    {
        for ( int i = 0; i < aChannels.size(); i++ )
        {
            LogChannel_t *channel = &aChannels[i];
            if ( channel->aName != sChannel )
                continue;

            return channel;
        }

        return nullptr;
    }

    void FormatLog( LogChannel_t *channel, Log &log )
    {
        char pBuf[ 1024 ];
        switch ( log.aLevel )
        {
            default:
            case LogLevel::Normal:
                snprintf( pBuf, 1024, "[%s] %s", channel->aName.c_str(), log.aMessage.c_str() );
                break;
            case LogLevel::Warning:
                snprintf( pBuf, 1024, "[%s] [WARNING] %s", channel->aName.c_str(), log.aMessage.c_str() );
                break;
            case LogLevel::Error:
                snprintf( pBuf, 1024, "[%s] [ERROR] %s", channel->aName.c_str(), log.aMessage.c_str() );
                break;
        }

        log.aFormatted = pBuf;
    }

    void LogMsg( LogChannel sChannel, LogLevel sLevel, const char *sMessage )
    {
        gLogMutex.lock();

        LogChannel_t* channel = GetChannel( sChannel );
        if ( !channel )
        {
            gLogMutex.unlock();
            return; // wtf
        }

        if ( channel->aEnabled )
        {
            switch ( sLevel )
            {
                default:
                case LogLevel::Normal:
                    LogSetColor( channel->aColor );
                    Print( "[%s] %s", channel->aName.c_str(), sMessage );
                    LogSetColor( LogColor::Default );
                    break;
                case LogLevel::Warning:
                    LogSetColor( LogColor::Yellow );
                    Print( "[%s] [WARNING] %s", channel->aName.c_str(), sMessage );
                    LogSetColor( LogColor::Default );
                    break;
                case LogLevel::Error:
                    LogSetColor( LogColor::Red );
                    Print( "[%s] [ERROR] %s", channel->aName.c_str(), sMessage );
                    LogSetColor( LogColor::Default );
                    break;
                case LogLevel::Fatal:
                    fprintf( stderr, "[%s] [FATAL]: %s", channel->aName.c_str(), sMessage );
                    SDL_ShowSimpleMessageBox( SDL_MESSAGEBOX_ERROR, "Fatal Error", sMessage, NULL );
                    throw std::runtime_error( sMessage );
                    break;
            }
        }

        aLogHistory.push_back( { sChannel, sLevel, sMessage } );

        FormatLog( channel, aLogHistory[aLogHistory.size() - 1] );

        gLogMutex.unlock();
    }

    void GetHistoryStr( std::string& output, int maxSize )
    {
        // No limit
        if ( maxSize == -1 )
        {
            for ( auto& log : aLogHistory )
            {
                LogChannel_t* channel = GetChannel( log.aChannel );
                if ( !channel || !channel->aEnabled )
                    continue;

                output += log.aFormatted;
            }

            return;
        }

        // go from latest to oldest
        for ( size_t i = aLogHistory.size() - 1; i > 0; i-- )
        {
            LogChannel_t* channel = GetChannel( aLogHistory[i].aChannel );
            if ( !channel || !channel->aEnabled )
                continue;

            int strLen = glm::min( maxSize, (int)aLogHistory[i].aFormatted.length() );
            int strStart = aLogHistory[i].aFormatted.length() - strLen;

            // if the length wanted is less then the string length, then start at an offset
            output = aLogHistory[i].aFormatted.substr( strStart, strLen ) + output;

            maxSize -= strLen;

            if ( maxSize == 0 )
                break;
        }
    }

private:
    std::vector< LogChannel_t > aChannels;
    std::vector< Log > aLogHistory;
};

LogSystem& GetLogSystem()
{
    static LogSystem logsystem;
    return logsystem;
}

CONCMD( log_channel_disable )
{
    if ( args.size() == 0 )
        return;

    LogChannel_t* channel = GetLogSystem().GetChannel( args[0].c_str() );
    if ( !channel )
        return;

    channel->aEnabled = false;
}

CONCMD( log_channel_enable )
{
    if ( args.size() == 0 )
        return;

    LogChannel_t* channel = GetLogSystem().GetChannel( args[0].c_str() );
    if ( !channel )
        return;

    channel->aEnabled = true;
}

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
        Print( "*** Failed to Get Console Window\n" );
        return;
    }

    BOOL result = SetConsoleTextAttribute( handle, Win32GetColor( color ) );

    if ( !result )
    {
        Print( "*** Failed to set console color: %s\n", sys_get_error() );
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


LogChannel LogRegisterChannel( const char* sName, LogColor sColor )
{
    return GetLogSystem().RegisterChannel( sName, sColor );
}


void LogGetHistoryStr( std::string& output, int maxSize )
{
    return GetLogSystem().GetHistoryStr( output, maxSize );
}


/* Deprecated (... or is it?!) */
void Print( const char* format, ... )
{
	VSTRING( std::string buffer, format );
	fputs( buffer.c_str(), stdout );
}


void Puts( const char* buffer )
{
	fputs( buffer, stdout );
}


// ----------------------------------------------------------------
// Specify a custom channel.

/* Lowest severity.  */
void LogMsg( LogChannel channel, const char *spFmt, ... ) 
{
    char pBuf[ 1024 ];
    va_list args;
    va_start( args, spFmt );
    vsprintf( pBuf, spFmt, args );
    GetLogSystem().LogMsg( channel, LogLevel::Normal, pBuf );
    va_end( args );
}

/* Lowest severity, no format.  */
void LogPuts( LogChannel channel, const char *spBuf )
{
    GetLogSystem().LogMsg( channel, LogLevel::Normal, spBuf );
}

/* Medium severity.  */
void LogWarn( LogChannel channel, const char *spFmt, ... )
{
    char pBuf[ 1024 ];
    va_list args;
    va_start( args, spFmt );
    vsprintf( pBuf, spFmt, args );
    GetLogSystem().LogMsg( channel, LogLevel::Warning, pBuf );
    va_end( args );
}

/* Medium severity.  */
void LogPutsWarn( LogChannel channel, const char *spBuf )
{
    GetLogSystem().LogMsg( channel, LogLevel::Warning, spBuf );
}

/* High severity.  */
/* TODO: maybe have this print to stderr? */
void LogError( LogChannel channel, const char *spFmt, ... )
{
    char pBuf[ 1024 ];
    va_list args;
    va_start( args, spFmt );
    vsprintf( pBuf, spFmt, args );
    GetLogSystem().LogMsg( channel, LogLevel::Error, pBuf );
    va_end( args );
}

/* Extreme severity.  */
void LogFatal( LogChannel channel, const char *spFmt, ... )
{
    char pBuf[ 1024 ];
    va_list args;
    va_start( args, spFmt );
    vsprintf( pBuf, spFmt, args );
    GetLogSystem().LogMsg( channel, LogLevel::Fatal, pBuf );
    va_end( args );
}

/* Dev only.  */
void LogDev( LogChannel channel, u8 sLvl, const char *spFmt, ... )
{
    char pBuf[ 1024 ];
    va_list args;
    va_start( args, spFmt );
    vsprintf( pBuf, spFmt, args );
    GetLogSystem().LogMsg( channel, LogLevel::Dev, pBuf );
    va_end( args );
}


/* Dev only.  */
void LogPutsDev( LogChannel channel, u8 sLvl, const char *spBuf )
{
    GetLogSystem().LogMsg( channel, LogLevel::Dev, spBuf );
}


// ----------------------------------------------------------------
// Default to general channel.

/* Lowest severity.  */
void LogMsg( const char *spFmt, ... ) 
{
    char pBuf[ 1024 ];
    va_list args;
    va_start( args, spFmt );
    vsprintf( pBuf, spFmt, args );
    GetLogSystem().LogMsg( gGeneralChannel, LogLevel::Normal, pBuf );
    va_end( args );
}

/* Lowest severity, no format.  */
void LogPuts( const char *spBuf )
{
    GetLogSystem().LogMsg( gGeneralChannel, LogLevel::Normal, spBuf );
}

/* Medium severity.  */
void LogWarn( const char *spFmt, ... )
{
    char pBuf[ 1024 ];
    va_list args;
    va_start( args, spFmt );
    vsprintf( pBuf, spFmt, args );
    GetLogSystem().LogMsg( gGeneralChannel, LogLevel::Warning, pBuf );
    va_end( args );
}

/* Medium severity.  */
void LogPutsWarn( const char *spBuf )
{
    GetLogSystem().LogMsg( gGeneralChannel, LogLevel::Warning, spBuf );
}

/* High severity.  */
/* TODO: maybe have this print to stderr? */
void LogError( const char *spFmt, ... )
{
    char pBuf[ 1024 ];
    va_list args;
    va_start( args, spFmt );
    vsprintf( pBuf, spFmt, args );
    GetLogSystem().LogMsg( gGeneralChannel, LogLevel::Error, pBuf );
    va_end( args );
}

/* Extreme severity.  */
void LogFatal( const char *spFmt, ... )
{
    char pBuf[ 1024 ];
    va_list args;
    va_start( args, spFmt );
    vsprintf( pBuf, spFmt, args );
    GetLogSystem().LogMsg( gGeneralChannel, LogLevel::Fatal, pBuf );
    va_end( args );
}

/* Dev only.  */
void LogDev( u8 sLvl, const char *spFmt, ... )
{
    char pBuf[ 1024 ];
    va_list args;
    va_start( args, spFmt );
    vsprintf( pBuf, spFmt, args );
    GetLogSystem().LogMsg( gGeneralChannel, LogLevel::Dev, pBuf );
    va_end( args );
}


/* Dev only.  */
void LogPutsDev( u8 sLvl, const char *spBuf )
{
    GetLogSystem().LogMsg( gGeneralChannel, LogLevel::Dev, spBuf );
}

