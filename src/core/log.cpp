#include "core/log.h"
#include "core/console.h"

#include <iostream>
#include <mutex>

#include <SDL.h>

CONVAR( developer, 1 );

LogColor          gCurrentColor = LogColor::Default;
LogChannel        gGeneralChannel = INVALID_LOG_CHANNEL;
static std::mutex gLogMutex;

class LogSystem
{
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

    // Format for system console output
    void FormatLog( LogChannel_t *channel, Log &log )
    {
        switch ( log.aType )
        {
            default:
            case LogType::Normal:
                vstring( log.aFormatted, "[%s] %s", channel->aName.c_str(), log.aMessage.c_str() );
                break;

            case LogType::Dev:
            case LogType::Dev2:
            case LogType::Dev3:
            case LogType::Dev4:
                vstring( log.aFormatted, "[%s] [DEV %u] %s", channel->aName.c_str(), log.aType, log.aMessage.c_str() );
                break;

            case LogType::Input:
                vstring( log.aFormatted, "] %s\n", log.aMessage.c_str() );
                break;

            case LogType::Raw:
                vstring( log.aFormatted, "%s", log.aMessage.c_str() );
                break;

            case LogType::Warning:
                vstring( log.aFormatted, "[%s] [WARNING] %s", channel->aName.c_str(), log.aMessage.c_str() );
                break;

            case LogType::Error:
                vstring( log.aFormatted, "[%s] [ERROR] %s", channel->aName.c_str(), log.aMessage.c_str() );
                break;

            case LogType::Fatal:
                vstring( log.aFormatted, "[%s] [FATAL] %s", channel->aName.c_str(), log.aMessage.c_str() );
                break;
        }
    }

    void LogMsg( LogChannel sChannel, LogType sLevel, const char* spFmt, va_list args )
    {
        gLogMutex.lock();

        aLogHistory.push_back( { sChannel, sLevel, "" } );
        Log& log = aLogHistory[aLogHistory.size() - 1];

        va_list copy;
        va_copy( copy, args );
        int len = std::vsnprintf( nullptr, 0, spFmt, copy );
        va_end( copy );

        if ( len < 0 )
        {
            Print( "\n *** LogSystem: vsnprintf failed?\n\n" );
            gLogMutex.unlock();
            return;
        }

        log.aMessage.resize( std::size_t(len) + 1, '\0' );
        std::vsnprintf( log.aMessage.data(), log.aMessage.size(), spFmt, args );
        log.aMessage.resize( len );

        AddLogInternal( log );

        gLogMutex.unlock();
    }

    void LogMsg( LogChannel sChannel, LogType sLevel, const char* spBuf )
    {
        gLogMutex.lock();

        aLogHistory.push_back( { sChannel, sLevel, spBuf } );
        Log& log = aLogHistory[aLogHistory.size() - 1];

        AddLogInternal( log );

        gLogMutex.unlock();
    }

    void AddLogInternal( Log& log )
    {
        LogChannel_t* channel = GetChannel( log.aChannel );
        if ( !channel )
        {
            Print( "\n *** LogSystem: Channel Not Found for message: \"%s\"\n", log.aMessage );
            return;
        }

        FormatLog( channel, log );

        if ( channel->aShown )
        {
            switch ( log.aType )
            {
                default:
                case LogType::Normal:
                case LogType::Dev:
                case LogType::Input:
                case LogType::Raw:
                    LogSetColor( channel->aColor );
                    fputs( log.aFormatted.c_str(), stdout );
                    LogSetColor( LogColor::Default );
                    break;

                case LogType::Warning:
                    LogSetColor( LOG_COLOR_WARNING );
                    fputs( log.aFormatted.c_str(), stdout );
                    LogSetColor( LogColor::Default );
                    break;

                case LogType::Error:
                    LogSetColor( LOG_COLOR_ERROR );
                    fputs( log.aFormatted.c_str(), stdout );
                    LogSetColor( LogColor::Default );
                    break;

                case LogType::Fatal:
                    LogSetColor( LOG_COLOR_ERROR );
                    fputs( log.aFormatted.c_str(), stderr );
                    LogSetColor( LogColor::Default );

                    std::string messageBoxTitle;
                    vstring( messageBoxTitle, "[%s] Fatal Error", channel->aName.c_str() );
                    SDL_ShowSimpleMessageBox( SDL_MESSAGEBOX_ERROR, messageBoxTitle.c_str(), log.aMessage.c_str(), NULL);
                    throw std::runtime_error( log.aFormatted.c_str() );
                    break;
            }
        }

        AddToHistoryString( log );
    }

    inline bool DevLevelVisible( const Log& log )
    {
        if ( developer.GetInt() < ( int )log.aType && ( log.aType == LogType::Dev  || 
                                                        log.aType == LogType::Dev2 || 
                                                        log.aType == LogType::Dev3 || 
                                                        log.aType == LogType::Dev4    ) ) 
            return false;

        return true;
    }

    // -----------------------------------------------------------------------------
    // Maybe move this stuff to some separate class, for different variations of "log listeners"? idk

    // TODO: make this async and probably store elsewhere
    void BuildHistoryString( int maxSize )
    {
        aLogHistoryStr = "";

        // No limit
        if ( maxSize == -1 )
        {
            for ( auto& log : aLogHistory )
                AddToHistoryString( log );

            goto BuildHistoryStringEnd;
        }

        // go from latest to oldest
        for ( size_t i = aLogHistory.size() - 1; i > 0; i-- )
        {
            auto& log = aLogHistory[i];

            if ( !DevLevelVisible( log ) )
                continue;

            LogChannel_t* channel = GetChannel( log.aChannel );
            if ( !channel || !channel->aShown )
                continue;

            int strLen = glm::min( maxSize, (int)log.aFormatted.length() );
            int strStart = log.aFormatted.length() - strLen;

            // if the length wanted is less then the string length, then start at an offset
            aLogHistoryStr = log.aFormatted.substr( strStart, strLen ) + aLogHistoryStr;

            maxSize -= strLen;

            if ( maxSize == 0 )
                break;
        }

BuildHistoryStringEnd:
        RunCallbacksChannelShown();
    }

    inline void AddToHistoryString( const Log& log )
    {
        if ( !DevLevelVisible( log ) )
            return;

        LogChannel_t* channel = GetChannel( log.aChannel );
        if ( !channel || !channel->aShown )
            return;

        aLogHistoryStr += log.aFormatted;
    }

    const std::string& GetHistoryStr( int maxSize )
    {
        // lazy
        if ( maxSize != -1 )
        {
            BuildHistoryString( maxSize );
        }

        return aLogHistoryStr;
    }
    
    void RunCallbacksChannelShown()
    {
        for ( LogChannelShownCallbackF callback : aCallbacksChannelShown )
        {
            callback();
        }
    }

    std::vector< LogChannel_t > aChannels;
    std::vector< Log >          aLogHistory;

    // filtered text output for game console
    std::string                 aLogHistoryStr;
    std::string                 aFilterEx;  // super basic exclude filter
    std::string                 aFilterIn;  // super basic include filter

    std::vector< LogChannelShownCallbackF > aCallbacksChannelShown;
};

LogSystem& GetLogSystem()
{
    static LogSystem logsystem;
    return logsystem;
}

void log_channel_dropdown(
	const std::vector< std::string >& args,  // arguments currently typed in by the user
	std::vector< std::string >& results )      // results to populate the dropdown list with
{
	for ( const auto &channel : GetLogSystem().aChannels )
	{
		if ( args.size() && !channel.aName.starts_with( args[0] ) )
			continue;

		results.push_back( channel.aName );
	}
}

CONCMD_DROP( log_channel_hide, log_channel_dropdown )
{
    if ( args.size() == 0 )
        return;

    LogChannel_t* channel = GetLogSystem().GetChannel( args[0].c_str() );
    if ( !channel )
        return;

    channel->aShown = false;

    GetLogSystem().BuildHistoryString( -1 );
}

CONCMD_DROP( log_channel_show, log_channel_dropdown )
{
    if ( args.size() == 0 )
        return;

    LogChannel_t* channel = GetLogSystem().GetChannel( args[0].c_str() );
    if ( !channel )
        return;

    channel->aShown = true;

    GetLogSystem().BuildHistoryString( -1 );
}

CONCMD( log_channel_dump )
{
    LogPuts( "Log Channels: \n" );
    for ( const auto& channel : GetLogSystem().aChannels )
    {
        LogMsg( "\t%s - Visibility: %s, Color: %d\n", channel.aName.c_str(), channel.aShown ? "Shown" : "Hidden", channel.aColor );
    }
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
        case LogColor::White:
            return FOREGROUND_INTENSITY | FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;

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
        case LogColor::Gray:
            return FOREGROUND_INTENSITY;

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

#endif

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
    fputs( UnixGetColor( color ), stdout );
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


constexpr const char* LogColorToStr( LogColor color )
{
    switch ( color )
    {
        case LogColor::Black:
            return "Black";
        case LogColor::White:
            return "White";

        case LogColor::DarkBlue:
            return "DarkBlue";
        case LogColor::DarkGreen:
            return "DarkGreen";
        case LogColor::DarkCyan:
            return "DarkCyan";
        case LogColor::DarkRed:
            return "DarkRed";
        case LogColor::DarkMagenta:
            return "DarkMagenta";
        case LogColor::DarkYellow:
            return "DarkYellow";
        case LogColor::DarkGray:
            return "DarkGray";

        case LogColor::Blue:
            return "Blue";
        case LogColor::Green:
            return "Green";
        case LogColor::Cyan:
            return "Cyan";
        case LogColor::Red:
            return "Red";
        case LogColor::Magenta:
            return "Magenta";
        case LogColor::Yellow:
            return "Yellow";
        case LogColor::Gray:
            return "Gray";

        case LogColor::Default:
        default:
            return "Default";
    }
}


LogChannel LogRegisterChannel( const char* sName, LogColor sColor )
{
    return GetLogSystem().RegisterChannel( sName, sColor );
}


LogChannel LogGetChannel( const char* name )
{
    for ( size_t i = 0; auto channel : GetLogSystem().aChannels )
    {
        // if ( GetLogSystem().aChannels[i].aName == name )
        if ( channel.aName == name )
            return (LogChannel)i;

        i++;
    }

    return INVALID_LOG_CHANNEL;
}


LogColor LogGetChannelColor( LogChannel handle )
{
    LogChannel_t* channel = GetLogSystem().GetChannel( handle );
    if ( !channel )
        return LogColor::Default;

    return channel->aColor;
}


const std::string& LogGetChannelName( LogChannel handle )
{
    LogChannel_t* channel = GetLogSystem().GetChannel( handle );
    if ( !channel )
        return "[General]";

    return channel->aName;
}


unsigned char LogGetChannelCount()
{
    return (unsigned char)GetLogSystem().aChannels.size();
}


const std::string& LogGetHistoryStr( int maxSize )
{
    return GetLogSystem().GetHistoryStr( maxSize );
}


const std::vector< Log >& LogGetLogHistory()
{
    return GetLogSystem().aLogHistory;
}


bool LogChannelIsShown( LogChannel handle )
{
    LogChannel_t* channel = GetLogSystem().GetChannel( handle );
    if ( !channel )
        return false;

    return channel->aShown;
}


const Log* LogGetLastLog()
{
    if ( GetLogSystem().aLogHistory.size() == 0 )
        return nullptr;

    return &GetLogSystem().aLogHistory[ GetLogSystem().aLogHistory.size() - 1 ];
}


bool LogIsVisible( const Log& log )
{
    if ( !GetLogSystem().DevLevelVisible( log ) )
        return false;

    return LogChannelIsShown( log.aChannel );
}


void LogAddChannelShownCallback( LogChannelShownCallbackF callback )
{

}


// ----------------------------------------------------------------
// System printing, skip logging


/* Deprecated (... or is it?!) */
void Print( const char* format, ... )
{
    std::string buffer;
	VSTRING( buffer, format );
	fputs( buffer.c_str(), stdout );
}


void Puts( const char* buffer )
{
	fputs( buffer, stdout );
}


// ----------------------------------------------------------------
// Specify a custom channel.


#define LOG_SYS_MSG_VA( str, channel, level ) \
    va_list args; \
    va_start( args, str ); \
    GetLogSystem().LogMsg( channel, level, str, args ); \
    va_end( args )


/* General Logging Function.  */
void LogEx( LogChannel channel, LogType sLevel, const char *spBuf )
{
    GetLogSystem().LogMsg( channel, sLevel, spBuf );
}

void LogExF( LogChannel channel, LogType sLevel, const char *spFmt, ... )
{
    LOG_SYS_MSG_VA( spFmt, channel, sLevel );
}


// maybe later, don't feel like unrestricting it from the channels
#if 0
void LogEx( LogChannel channel, LogType sLevel, LogColor color, const char *spBuf )
{
    GetLogSystem().LogMsg( channel, sLevel, color, spBuf );
}

void LogExF( LogChannel channel, LogType sLevel, LogColor color, const char *spFmt, ... )
{
    va_list args;
    va_start( args, str );
    GetLogSystem().LogMsg( channel, sLevel, color, str, args );
    va_end( args );
}
#endif


/* Lowest severity.  */
void LogMsg( LogChannel channel, const char *spFmt, ... ) 
{
    LOG_SYS_MSG_VA( spFmt, channel, LogType::Normal );
}

/* Lowest severity, no format.  */
void LogPuts( LogChannel channel, const char *spBuf )
{
    GetLogSystem().LogMsg( channel, LogType::Normal, spBuf );
}

/* Medium severity.  */
void LogWarn( LogChannel channel, const char *spFmt, ... )
{
    LOG_SYS_MSG_VA( spFmt, channel, LogType::Warning );
}

/* Medium severity.  */
void LogPutsWarn( LogChannel channel, const char *spBuf )
{
    GetLogSystem().LogMsg( channel, LogType::Warning, spBuf );
}

/* High severity.  */
/* TODO: maybe have this print to stderr? */
void LogError( LogChannel channel, const char *spFmt, ... )
{
    LOG_SYS_MSG_VA( spFmt, channel, LogType::Error );
}

/* Extreme severity.  */
void LogFatal( LogChannel channel, const char *spFmt, ... )
{
    LOG_SYS_MSG_VA( spFmt, channel, LogType::Fatal );
}

/* Dev only.  */
void LogDev( LogChannel channel, u8 sLvl, const char *spFmt, ... )
{
    if ( sLvl < 1 || sLvl > 4)
        sLvl = 4;

    LOG_SYS_MSG_VA( spFmt, channel, (LogType)sLvl );
}


/* Dev only.  */
void LogPutsDev( LogChannel channel, u8 sLvl, const char *spBuf )
{
    GetLogSystem().LogMsg( channel, LogType::Dev, spBuf );
}


// ----------------------------------------------------------------
// Default to general channel.

/* Lowest severity.  */
void LogMsg( const char *spFmt, ... ) 
{
    LOG_SYS_MSG_VA( spFmt, gGeneralChannel, LogType::Normal );
}

/* Lowest severity, no format.  */
void LogPuts( const char *spBuf )
{
    GetLogSystem().LogMsg( gGeneralChannel, LogType::Normal, spBuf );
}

/* Medium severity.  */
void LogWarn( const char *spFmt, ... )
{
    LOG_SYS_MSG_VA( spFmt, gGeneralChannel, LogType::Warning );
}

/* Medium severity.  */
void LogPutsWarn( const char *spBuf )
{
    GetLogSystem().LogMsg( gGeneralChannel, LogType::Warning, spBuf );
}

/* High severity.  */
/* TODO: maybe have this print to stderr? */
void LogError( const char *spFmt, ... )
{
    LOG_SYS_MSG_VA( spFmt, gGeneralChannel, LogType::Error );
}

/* Extreme severity.  */
void LogFatal( const char *spFmt, ... )
{
    LOG_SYS_MSG_VA( spFmt, gGeneralChannel, LogType::Fatal );
}

/* Dev only.  */
void LogDev( u8 sLvl, const char *spFmt, ... )
{
    LOG_SYS_MSG_VA( spFmt, gGeneralChannel, LogType::Dev );
}


/* Dev only.  */
void LogPutsDev( u8 sLvl, const char *spBuf )
{
    GetLogSystem().LogMsg( gGeneralChannel, LogType::Dev, spBuf );
}

