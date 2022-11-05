#include "core/log.h"
#include "core/console.h"
#include "core/profiler.h"

#include <iostream>
#include <mutex>

#include <SDL.h>

CONVAR( developer, 1 );

LogColor                                       gCurrentColor       = LogColor::Default;
LogChannel                                     gGeneralChannel     = INVALID_LOG_CHANNEL;
static std::string                             gDefaultChannelName = "[General]";
static std::mutex                              gLogMutex;

// apparently you could of added stuff to this before dynamic initialization got to this, and then you lose some log channels as a result
std::vector< LogChannel_t >& GetLogChannels()
{
	static std::vector< LogChannel_t > gChannels;
	return gChannels;
}

// static std::vector< LogChannel_t >             gChannels;
static std::vector< Log >                      gLogHistory;

// filtered text output for game console
static std::string                             gLogHistoryStr;
static std::string                             gFilterEx;  // super basic exclude filter
static std::string                             gFilterIn;  // super basic include filter

static std::vector< LogChannelShownCallbackF > gCallbacksChannelShown;

constexpr glm::vec4                            gVecTo255( 255, 255, 255, 255 );


static void StripTrailingLines( const char* pText, size_t& len )
{
    while ( len > 1 )
    {
        switch ( pText[len - 1] )
        {
        case ' ':
        case 9:
        case '\r':
        case '\n':
        case 11:
        case '\f':
            --len;
            break;
        default:
            return;
        }
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
		PrintF( "*** Failed to set console color: %s\n", sys_get_error() );
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


void Log_Init()
{
	gGeneralChannel = Log_RegisterChannel( "General", LogColor::Default );
}


LogChannel Log_RegisterChannel( const char *sName, LogColor sColor )
{
	for ( int i = 0; i < GetLogChannels().size(); i++ )
    {
		LogChannel_t* channel = &GetLogChannels()[ i ];
        if ( channel->aName != sName )
            continue;
            
        return i;
    }

    GetLogChannels().emplace_back( true, sName, sColor );
	return (LogChannel)GetLogChannels().size() - 1;
}


LogChannel Log_GetChannel( const char* sChannel )
{
	for ( int i = 0; i < GetLogChannels().size(); i++ )
	{
		LogChannel_t* channel = &GetLogChannels()[ i ];
		if ( channel->aName != sChannel )
			continue;

		return (LogChannel)i;
	}

	return INVALID_LOG_CHANNEL;
}


LogChannel_t* Log_GetChannelData( LogChannel sChannel )
{
	if ( sChannel >= GetLogChannels().size() )
        return nullptr;

    return &GetLogChannels()[ sChannel ];
}


LogChannel_t* Log_GetChannelByName( const char* sChannel )
{
	for ( int i = 0; i < GetLogChannels().size(); i++ )
    {
		LogChannel_t* channel = &GetLogChannels()[ i ];
        if ( channel->aName != sChannel )
            continue;

        return channel;
    }

    return nullptr;
}


// Format for system console output
void FormatLog( LogChannel_t *channel, Log &log )
{
    PROF_SCOPE();

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


constexpr LogColor GetColor( LogChannel_t *channel, const Log& log )
{
    switch ( log.aType )
    {
        default:
        case LogType::Normal:
        case LogType::Dev:
        case LogType::Dev2:
        case LogType::Dev3:
        case LogType::Dev4:
        case LogType::Input:
        case LogType::Raw:
            return channel->aColor;

        case LogType::Warning:
            return LOG_COLOR_WARNING;

        case LogType::Error:
        case LogType::Fatal:
            return LOG_COLOR_ERROR;
    }
}


// copied from graphics/gui/consoleui.cpp
constexpr glm::vec4 GetColorRGBA( LogColor col )
{
    switch (col)
    {
        // hmm
        case LogColor::Black:
            return {0.3, 0.3, 0.3, 1};
        case LogColor::White:
            return {1, 1, 1, 1};

        case LogColor::DarkBlue:
            return {0, 0.3, 0.8, 1};
        case LogColor::DarkGreen:
            return {0.25, 0.57, 0.25, 1};
        case LogColor::DarkCyan:
            return {0, 0.35, 0.75, 1};
        case LogColor::DarkRed:
            return {0.7, 0, 0.25, 1};
        case LogColor::DarkMagenta:
            return {0.45, 0, 0.7, 1};
        case LogColor::DarkYellow:
            return {0.6, 0.6, 0, 1};
        case LogColor::DarkGray:
            return {0.45, 0.45, 0.45, 1};

        case LogColor::Blue:
            return {0, 0.4, 1, 1};
        case LogColor::Green:
            return {0.4, 0.9, 0.4, 1};
        case LogColor::Cyan:
            return {0, 0.85, 1, 1};
        case LogColor::Red:
            return {0.9, 0, 0.4, 1};
        case LogColor::Magenta:
            return {0.6, 0, 0.9, 1};
        case LogColor::Yellow:
            return {1, 1, 0, 1};
        case LogColor::Gray:
            return {0.7, 0.7, 0.7, 1};

        case LogColor::Default:
        default:
            return {1, 1, 1, 1};
    }
}


constexpr glm::vec4 GetColorRGBA( LogChannel_t *channel, const Log& log )
{
    return GetColorRGBA( GetColor( channel, log ) );
}

    
u32 GetColorU32( LogChannel_t *channel, const Log& log )
{
    // i don't like this
    glm::vec4 colorVec = GetColorRGBA( GetColor( channel, log ) );

    u8 colorU8[4] = {
        static_cast< u8 >( colorVec.x * 255 ),
        static_cast< u8 >( colorVec.y * 255 ),
        static_cast< u8 >( colorVec.z * 255 ),
        static_cast< u8 >( colorVec.w * 255 ),
    };

    // what
    u32 color = *( (u32 *)colorU8 );

    return color;
}


inline bool Log_DevLevelVisible( const Log& log )
{
	if ( developer.GetInt() < (int)log.aType && ( log.aType == LogType::Dev ||
	                                              log.aType == LogType::Dev2 ||
	                                              log.aType == LogType::Dev3 ||
	                                              log.aType == LogType::Dev4 ) )
		return false;

	return true;
}


inline void Log_Tracy( LogChannel_t* channel, const Log& log )
{
#ifdef TRACY_ENABLE
	if ( !tracy::ProfilerAvailable() )
		return;

	size_t len = log.aFormatted.size();
	StripTrailingLines( log.aFormatted.c_str(), len );
	TracyMessageC( log.aFormatted.c_str(), len, GetColorU32( channel, log ) );
#endif
}


void Log_RunCallbacksChannelShown()
{
	for ( LogChannelShownCallbackF callback : gCallbacksChannelShown )
	{
		callback();
	}
}


void Log_AddChannelShownCallback( LogChannelShownCallbackF callback )
{
	gCallbacksChannelShown.push_back( callback );
}


inline void Log_AddToHistoryString( const Log& log )
{
	if ( !Log_DevLevelVisible( log ) )
		return;

	LogChannel_t* channel = Log_GetChannelData( log.aChannel );
	if ( !channel || !channel->aShown )
		return;

	gLogHistoryStr += log.aFormatted;
}


// TODO: make this async and probably store elsewhere
void Log_BuildHistoryString( int maxSize )
{
	PROF_SCOPE();

	gLogHistoryStr = "";

	// No limit
	if ( maxSize == -1 )
	{
		for ( auto& log : gLogHistory )
			Log_AddToHistoryString( log );

		goto BuildHistoryStringEnd;
	}

	// go from latest to oldest
	for ( size_t i = gLogHistory.size() - 1; i > 0; i-- )
	{
		auto& log = gLogHistory[ i ];

		if ( !Log_DevLevelVisible( log ) )
			continue;

		LogChannel_t* channel = Log_GetChannelData( log.aChannel );
		if ( !channel || !channel->aShown )
			continue;

		int strLen     = glm::min( maxSize, (int)log.aFormatted.length() );
		int strStart   = log.aFormatted.length() - strLen;

		// if the length wanted is less then the string length, then start at an offset
		gLogHistoryStr = log.aFormatted.substr( strStart, strLen ) + gLogHistoryStr;

		maxSize -= strLen;

		if ( maxSize == 0 )
			break;
	}

BuildHistoryStringEnd:
	Log_RunCallbacksChannelShown();
}


const std::string& Log_GetHistoryStr( int maxSize )
{
	// lazy
	if ( maxSize != -1 )
	{
		Log_BuildHistoryString( maxSize );
	}

	return gLogHistoryStr;
}


void Log_AddLogInternal( Log& log )
{
    PROF_SCOPE();

    LogChannel_t* channel = Log_GetChannelData( log.aChannel );
    if ( !channel )
    {
        PrintF( "\n *** LogSystem: Channel Not Found for message: \"%s\"\n", log.aMessage.c_str() );
        return;
    }

    FormatLog( channel, log );

    if ( channel->aShown )
	{
#ifdef _WIN32
        // doesn't work?
        // http://unixwiz.net/techtips/outputdebugstring.html
		// OutputDebugStringA( log.aFormatted.c_str() );
#endif
        switch ( log.aType )
        {
            default:
            case LogType::Normal:
            case LogType::Input:
            case LogType::Raw:
                Log_SetColor( channel->aColor );
                fputs( log.aFormatted.c_str(), stdout );
                fflush( stdout );
				Log_SetColor( LogColor::Default );
                break;

            case LogType::Dev:
            case LogType::Dev2:
            case LogType::Dev3:
            case LogType::Dev4:
				if ( !Log_DevLevelVisible( log ) )
                    break;

                Log_SetColor( channel->aColor );
                fputs( log.aFormatted.c_str(), stdout );
                fflush( stdout );
				Log_SetColor( LogColor::Default );
                break;

            case LogType::Warning:
				Log_SetColor( LOG_COLOR_WARNING );
                fputs( log.aFormatted.c_str(), stdout );
                fflush( stdout );
				Log_SetColor( LogColor::Default );
                break;

            case LogType::Error:
				Log_SetColor( LOG_COLOR_ERROR );
				fputs( log.aFormatted.c_str(), stderr );
				fflush( stderr );
				Log_SetColor( LogColor::Default );
                break;

            case LogType::Fatal:
				Log_SetColor( LOG_COLOR_ERROR );
                fputs( log.aFormatted.c_str(), stderr );
                fflush( stderr );
				Log_SetColor( LogColor::Default );

                std::string messageBoxTitle;
                vstring( messageBoxTitle, "[%s] Fatal Error", channel->aName.c_str() );

                if ( log.aMessage.ends_with("\n") )
                    SDL_ShowSimpleMessageBox( SDL_MESSAGEBOX_ERROR, messageBoxTitle.c_str(), log.aMessage.substr(0, log.aMessage.size()-1 ).c_str(), NULL );
                else
                    SDL_ShowSimpleMessageBox( SDL_MESSAGEBOX_ERROR, messageBoxTitle.c_str(), log.aMessage.c_str(), NULL );

                throw std::runtime_error( log.aFormatted.c_str() );
                break;
        }
    }

    Log_Tracy( channel, log );
    Log_AddToHistoryString( log );
}


// =====================================================================================
// Log ConCommands

void log_channel_dropdown(
	const std::vector< std::string >& args,  // arguments currently typed in by the user
	std::vector< std::string >& results )      // results to populate the dropdown list with
{
	for ( const auto& channel : GetLogChannels() )
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

    LogChannel_t* channel = Log_GetChannelByName( args[0].c_str() );
    if ( !channel )
        return;

    channel->aShown = false;

    Log_BuildHistoryString( -1 );
}


CONCMD_DROP( log_channel_show, log_channel_dropdown )
{
    if ( args.size() == 0 )
        return;

    LogChannel_t* channel = Log_GetChannelByName( args[ 0 ].c_str() );
    if ( !channel )
        return;

    channel->aShown = true;

    Log_BuildHistoryString( -1 );
}

CONCMD( log_channel_dump )
{
    Log_Msg( "Log Channels: \n" );
	for ( const auto& channel : GetLogChannels() )
    {
		Log_MsgF( "\t%s - Visibility: %s, Color: %d\n", channel.aName.c_str(), channel.aShown ? "Shown" : "Hidden", channel.aColor );
    }
}


void Log_SetColor( LogColor color )
{
    gCurrentColor = color;

#ifdef _WIN32
    Win32SetColor( color );
#elif __unix__
    // technically works in windows 10, might try later and see if it's cheaper, idk
    UnixSetColor( color );
#endif
}

LogColor Log_GetColor()
{
    return gCurrentColor;
}


const char* Log_ColorToStr( LogColor color )
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


LogChannel LogGetChannel( const char* name )
{
	for ( size_t i = 0; const auto& channel : GetLogChannels() )
    {
        // if ( GetLogSystem().aChannels[i].aName == name )
        if ( channel.aName == name )
            return (LogChannel)i;

        i++;
    }

    return INVALID_LOG_CHANNEL;
}


LogColor Log_GetChannelColor( LogChannel handle )
{
	LogChannel_t* channel = Log_GetChannelData( handle );
    if ( !channel )
        return LogColor::Default;

    return channel->aColor;
}


const std::string& Log_GetChannelName( LogChannel handle )
{
	LogChannel_t* channel = Log_GetChannelData( handle );
    if ( !channel )
        return gDefaultChannelName;

    return channel->aName;
}


unsigned char Log_GetChannelCount()
{
	return (unsigned char)GetLogChannels().size();
}


const std::vector< Log >& Log_GetLogHistory()
{
    return gLogHistory;
}


bool Log_ChannelIsShown( LogChannel handle )
{
	LogChannel_t* channel = Log_GetChannelData( handle );
    if ( !channel )
        return false;

    return channel->aShown;
}


const Log* Log_GetLastLog()
{
    if ( gLogHistory.size() == 0 )
        return nullptr;

    return &gLogHistory[ gLogHistory.size() - 1 ];
}


bool Log_IsVisible( const Log& log )
{
	if ( !Log_DevLevelVisible( log ) )
        return false;

    return Log_ChannelIsShown( log.aChannel );
}


// ----------------------------------------------------------------
// System printing, skip logging


void PrintF( const char* format, ... )
{
    std::string buffer;
	VSTRING( buffer, format );
	fputs( buffer.c_str(), stdout );
}


void Print( const char* buffer )
{
	fputs( buffer, stdout );
}


// ----------------------------------------------------------------
// Extended Logging Functions


#define LOG_SYS_MSG_VA( str, channel, level ) \
    va_list args; \
    va_start( args, str ); \
    Log_ExV( channel, level, str, args ); \
    va_end( args )


void Log_Ex( LogChannel sChannel, LogType sLevel, const char* spBuf )
{
	PROF_SCOPE();

	gLogMutex.lock();

	gLogHistory.emplace_back( sChannel, sLevel, spBuf );
	Log& log = gLogHistory[ gLogHistory.size() - 1 ];

	Log_AddLogInternal( log );

	gLogMutex.unlock();
}


void Log_ExF( LogChannel sChannel, LogType sLevel, const char* spFmt, ... )
{
	LOG_SYS_MSG_VA( spFmt, sChannel, sLevel );
}


void Log_ExV( LogChannel sChannel, LogType sLevel, const char* spFmt, va_list args )
{
	PROF_SCOPE();

	gLogMutex.lock();

	gLogHistory.emplace_back( sChannel, sLevel, "" );
	Log&    log = gLogHistory[ gLogHistory.size() - 1 ];

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

	log.aMessage.resize( std::size_t( len ) + 1, '\0' );
	std::vsnprintf( log.aMessage.data(), log.aMessage.size(), spFmt, args );
	log.aMessage.resize( len );

	Log_AddLogInternal( log );

	gLogMutex.unlock();
}


// ----------------------------------------------------------------
// Standard Logging Functions

// Lowest severity.
void CORE_API Log_Msg( LogChannel channel, const char* spBuf )
{
	Log_Ex( channel, LogType::Normal, spBuf );
}

void CORE_API Log_MsgF( LogChannel channel, const char* spFmt, ... )
{
	LOG_SYS_MSG_VA( spFmt, channel, LogType::Normal );
}

// Medium severity.
void CORE_API Log_Warn( LogChannel channel, const char* spBuf )
{
	Log_Ex( channel, LogType::Warning, spBuf );
}

void CORE_API Log_WarnF( LogChannel channel, const char* spFmt, ... )
{
	LOG_SYS_MSG_VA( spFmt, channel, LogType::Warning );
}

// High severity.
void CORE_API Log_Error( LogChannel channel, const char* spBuf )
{
	Log_Ex( channel, LogType::Error, spBuf );
}

void CORE_API Log_ErrorF( LogChannel channel, const char* spFmt, ... )
{
	LOG_SYS_MSG_VA( spFmt, channel, LogType::Error );
}

// Extreme severity.
void CORE_API Log_Fatal( LogChannel channel, const char* spBuf )
{
	Log_Ex( channel, LogType::Fatal, spBuf );
}

void CORE_API Log_FatalF( LogChannel channel, const char* spFmt, ... )
{
	LOG_SYS_MSG_VA( spFmt, channel, LogType::Fatal );
}

// Dev only.
void CORE_API Log_Dev( LogChannel channel, u8 sLvl, const char* spBuf )
{
	if ( sLvl < 1 || sLvl > 4 )
		sLvl = 4;

	Log_Ex( channel, (LogType)sLvl, spBuf );
}

void CORE_API Log_DevF( LogChannel channel, u8 sLvl, const char* spFmt, ... )
{
	if ( sLvl < 1 || sLvl > 4 )
		sLvl = 4;

	LOG_SYS_MSG_VA( spFmt, channel, (LogType)sLvl );
}


// ----------------------------------------------------------------
// Default to general channel.

// Lowest severity.
void CORE_API Log_Msg( const char* spBuf )
{
	Log_Ex( gGeneralChannel, LogType::Normal, spBuf );
}

void CORE_API Log_MsgF( const char* spFmt, ... )
{
	LOG_SYS_MSG_VA( spFmt, gGeneralChannel, LogType::Normal );
}

// Medium severity.
void CORE_API Log_Warn( const char* spBuf )
{
	Log_Ex( gGeneralChannel, LogType::Warning, spBuf );
}

void CORE_API Log_WarnF( const char* spFmt, ... )
{
	LOG_SYS_MSG_VA( spFmt, gGeneralChannel, LogType::Warning );
}

// High severity.
void CORE_API Log_Error( const char* spBuf )
{
	Log_Ex( gGeneralChannel, LogType::Error, spBuf );
}

void CORE_API Log_ErrorF( const char* spFmt, ... )
{
	LOG_SYS_MSG_VA( spFmt, gGeneralChannel, LogType::Error );
}

// Extreme severity.
void CORE_API Log_Fatal( const char* spBuf )
{
	Log_Ex( gGeneralChannel, LogType::Error, spBuf );
}

void CORE_API Log_FatalF( const char* spFmt, ... )
{
	LOG_SYS_MSG_VA( spFmt, gGeneralChannel, LogType::Fatal );
}

// Dev only.
void CORE_API Log_Dev( u8 sLvl, const char* spBuf )
{
	if ( sLvl < 1 || sLvl > 4 )
		sLvl = 4;

	Log_Ex( gGeneralChannel, (LogType)sLvl, spBuf );
}

void CORE_API Log_DevF( u8 sLvl, const char* spFmt, ... )
{
	if ( sLvl < 1 || sLvl > 4 )
		sLvl = 4;

	LOG_SYS_MSG_VA( spFmt, gGeneralChannel, (LogType)sLvl );
}
