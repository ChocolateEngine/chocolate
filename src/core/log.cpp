#include "core/log.h"
#include "core/console.h"
#include "core/profiler.h"

#include <iostream>
#include <mutex>
#include <regex>
#include <stack>

#include <SDL.h>

CONVAR( developer, 1 );

LogColor                                       gCurrentColor       = LogColor::Default;
LogChannel                                     gLC_General         = INVALID_LOG_CHANNEL;
LogChannel                                     gLC_Logging         = INVALID_LOG_CHANNEL;
static std::string                             gDefaultChannelName = "[General]";
static std::mutex                              gLogMutex;

// apparently you could of added stuff to this before static initialization got to this, and then you lose some log channels as a result
std::vector< LogChannel_t >& GetLogChannels()
{
	static std::vector< LogChannel_t > gChannels;
	return gChannels;
}

// static std::vector< LogChannel_t >             gChannels;
static std::vector< Log >                      gLogHistory;

static std::unordered_map< LogGroup, Log >     gLogGroups;

// filtered text output for game console
static std::string                             gFilterEx;  // super basic exclude filter
static std::string                             gFilterIn;  // super basic include filter

static std::vector< LogChannelShownCallbackF > gCallbacksChannelShown;

constexpr glm::vec4                            gVecTo255( 255, 255, 255, 255 );


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
		case LogColor::DarkPurple:
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
		case LogColor::Purple:
			return FOREGROUND_INTENSITY | FOREGROUND_RED | FOREGROUND_BLUE;
		case LogColor::Yellow:
			return FOREGROUND_INTENSITY | FOREGROUND_RED | FOREGROUND_GREEN;
		case LogColor::Gray:
			return FOREGROUND_INTENSITY;

		case LogColor::Default:
		case LogColor::Count:
		default:
			return FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;
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


const char* Log_ColorToStr( LogColor color )
{
    switch ( color )
    {
        case LogColor::Black:
            return "Black";
        case LogColor::White:
            return "White";

        case LogColor::DarkBlue:
            return "Dark Blue";
        case LogColor::DarkGreen:
            return "Dark Green";
        case LogColor::DarkCyan:
            return "Dark Cyan";
        case LogColor::DarkRed:
            return "Dark Red";
		case LogColor::DarkPurple:
            return "Dark Purple";
        case LogColor::DarkYellow:
            return "Dark Yellow";
        case LogColor::DarkGray:
            return "Dark Gray";

        case LogColor::Blue:
            return "Blue";
        case LogColor::Green:
            return "Green";
        case LogColor::Cyan:
            return "Cyan";
        case LogColor::Red:
            return "Red";
		case LogColor::Purple:
            return "Purple";
        case LogColor::Yellow:
            return "Yellow";
        case LogColor::Gray:
            return "Gray";

        case LogColor::Default:
		case LogColor::Count:
        default:
            return "Default";
    }
}


const char* Log_ColorToUnix( LogColor color )
{
	switch ( color )
	{
		case LogColor::Default:
		case LogColor::Count:
		default:
			return UNIX_CLR_DEFAULT;

		case LogColor::Black:
			return UNIX_CLR_BLACK;
		case LogColor::White:
			return UNIX_CLR_WHITE;

		case LogColor::DarkBlue:
			return UNIX_CLR_DARK_BLUE;
		case LogColor::DarkGreen:
			return UNIX_CLR_DARK_GREEN;
		case LogColor::DarkCyan:
			return UNIX_CLR_DARK_CYAN;
		case LogColor::DarkRed:
			return UNIX_CLR_DARK_RED;
		case LogColor::DarkPurple:
			return UNIX_CLR_DARK_PURPLE;
		case LogColor::DarkYellow:
			return UNIX_CLR_DARK_YELLOW;
		case LogColor::DarkGray:
			return UNIX_CLR_DARK_GRAY;

		case LogColor::Blue:
			return UNIX_CLR_BLUE;
		case LogColor::Green:
			return UNIX_CLR_GREEN;
		case LogColor::Cyan:
			return UNIX_CLR_CYAN;
		case LogColor::Red:
			return UNIX_CLR_RED;
		case LogColor::Purple:
			return UNIX_CLR_PURPLE;
		case LogColor::Yellow:
			return UNIX_CLR_YELLOW;
		case LogColor::Gray:
			return UNIX_CLR_GRAY;
	}
}


// check the specifc color int
LogColor Log_UnixCodeToColor( bool sIsLight, int sColor )
{
	if ( sColor == 0 )
		return LogColor::Default;

	switch ( sColor )
	{
		default:
			return LogColor::Default;

		// case 30:
		// 	return sIsLight ? LogColor::Default : LogColor::Black;

		case 37:
			return sIsLight ? LogColor::White : LogColor::Default;

		case 34:
			return sIsLight ? LogColor::Blue : LogColor::DarkBlue;

		case 32:
			return sIsLight ? LogColor::Green : LogColor::DarkGreen;

		case 36:
			return sIsLight ? LogColor::Cyan : LogColor::DarkCyan;

		case 31:
			return sIsLight ? LogColor::Red : LogColor::DarkRed;

		case 35:
			return sIsLight ? LogColor::Purple : LogColor::DarkPurple;

		case 33:
			return sIsLight ? LogColor::Yellow : LogColor::DarkYellow;

		case 30:
			return sIsLight ? LogColor::Gray : LogColor::DarkGray;
	}
}


LogColor Log_UnixToColor( const char* spColor, size_t sLen )
{
	if ( sLen == 4 )
	{
		// \033[0m
		return LogColor::Default;
	}

	if ( sLen != 7 )
	{
		PrintF( " *** Log_UnixToColor: Unknown Color Type: \"%s\"\n", spColor );
		return LogColor::Default;
	}

	bool isLight       = spColor[ 2 ] == '1';
	char colorStr[ 3 ] = { spColor[ 4 ], spColor[ 5 ], '\0' };

	long color         = 0;
	if ( !ToLong3( colorStr, color ) )
	{
		PrintF( " *** Failed get color code from string: %s (%s)\n", spColor, colorStr );
		return LogColor::Default;
	}

	return Log_UnixCodeToColor( isLight, color );
}


void UnixSetColor( LogColor color )
{
	fputs( Log_ColorToUnix( color ), stdout );
}


void Log_Init()
{
	gLC_General = Log_RegisterChannel( "General", LogColor::Default );
	gLC_Logging = Log_RegisterChannel( "Logging", LogColor::Default );
}


LogChannel Log_RegisterChannel( const char *sName, LogColor sColor )
{
	for ( size_t i = 0; i < GetLogChannels().size(); i++ )
    {
		LogChannel_t* channel = &GetLogChannels()[ i ];
        if ( channel->aName != sName )
            continue;
            
        return i;
    }

    GetLogChannels().emplace_back( sName, true, sColor );
	return (LogChannel)GetLogChannels().size() - 1U;
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


constexpr LogColor GetColor( LogChannel_t* channel, LogType sType )
{
	switch ( sType )
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


// Format for system console output
static std::string FormatLog( LogChannel_t* channel, LogType sType, const char* spMessage )
{
    PROF_SCOPE();

	// Special Exception for "Raw" LogType
	if ( sType == LogType::Raw )
		return spMessage;

	// Split by New Line characters
	std::string output;

	size_t      len  = strlen( spMessage );

	const char* last = spMessage;
	const char* find = strchr( spMessage, '\n' );

	while ( last )
	{
		size_t dist = 0;
		if ( find )
			dist = ( find - last ) + 1;
		else
			dist = len - ( last - spMessage );

		if ( dist == 0 )
			break;

		const char* color = Log_ColorToUnix( GetColor( channel, sType ) );

		switch ( sType )
		{
			default:
			case LogType::Normal:
				output += vstring( "%s[%s] %*.*s", color, channel->aName.data(), dist, dist, last );
				break;

			case LogType::Dev:
			case LogType::Dev2:
			case LogType::Dev3:
			case LogType::Dev4:
				output += vstring( "%s[%s] [DEV %u] %*.*s", color, channel->aName.data(), sType, dist, dist, last );
				break;

			case LogType::Input:
				output += vstring( "%s] %*.*s\n", color, dist, dist, last );
				break;

			case LogType::Warning:
				output += vstring( "%s[%s] [WARNING] %*.*s", color, channel->aName.data(), dist, dist, last );
				break;

			case LogType::Error:
				output += vstring( "%s[%s] [ERROR] %*.*s", color, channel->aName.data(), dist, dist, last );
				break;

			case LogType::Fatal:
				output += vstring( "%s[%s] [FATAL] %*.*s", color, channel->aName.data(), dist, dist, last );
				break;
		}

		if ( !find )
			break;

		last = ++find;
		find = strchr( last, '\n' );
	}

	return output;
}


// If no colors are found, srOutput is empty
// NOTE: this could probably be done better
static void Log_StripColors( std::string_view sBuffer, std::string& srOutput )
{
	char* buf  = const_cast< char* >( sBuffer.data() );

	char* find = strstr( buf, "\033[" );

	// no color codes found
	if ( find == nullptr )
		return;

	// no color code at start of string
	if ( find != buf )
	{
		srOutput.append( buf, find - buf );
	}

	char* last = buf;

	while ( find )
	{
		char* endColor = strstr( find, "m" );

		if ( !endColor )
		{
			Log_Error( "Invalid Unix Color Code!\n" );
			break;
		}

		endColor++;

		size_t colorLength = endColor - find;

		last               = endColor;
		char* nextFind     = strstr( endColor, "\033[" );

		// no characters in between these colors
		if ( nextFind && nextFind - endColor == 0 )
		{
			find = nextFind;
			continue;
		}

		if ( nextFind == nullptr )
		{
			srOutput.append( endColor, sBuffer.size() - ( last - buf ) );
		}
		else
		{
			// colorBuf.aLen = find - last;
			srOutput.append( endColor, nextFind - endColor );
			last = endColor;
		}

		find = nextFind;
	}
}


// Formats the log as normal, but strips all color codes from it
static std::string FormatLogNoColors( const Log& srLog )
{
	PROF_SCOPE();

	// Split by New Line characters
	std::string   output;

	LogChannel_t* channel = Log_GetChannelData( srLog.aChannel );
	if ( !channel )
	{
		PrintF( "\n *** LogSystem: Channel Not Found for message: \"%s\"\n", srLog.aMessage.c_str() );
		return output;
	}

	std::string stripped;
	Log_StripColors( srLog.aMessage, stripped );

	size_t      len  = 0;
	const char* last = nullptr;
	const char* find = nullptr;
	const char* buf = nullptr;

	if ( stripped.empty() )
	{
		// No color codes found
		len = srLog.aMessage.size();
		buf = srLog.aMessage.data();
	}
	else
	{
		// Color Codes found
		len = stripped.size();
		buf = stripped.data();
	}

	last = buf;
	find = strchr( buf, '\n' );

	while ( last )
	{
		size_t dist = 0;
		if ( find )
			dist = ( find - last ) + 1;
		else
			dist = len - ( last - buf );

		if ( dist == 0 )
			break;

		switch ( srLog.aType )
		{
			default:
			case LogType::Normal:
				output += vstring( "[%s] %*.*s", channel->aName.data(), dist, dist, last );
				break;

			case LogType::Dev:
			case LogType::Dev2:
			case LogType::Dev3:
			case LogType::Dev4:
				output += vstring( "[%s] [DEV %u] %*.*s", channel->aName.data(), srLog.aType, dist, dist, last );
				break;

			case LogType::Input:
				output += vstring( "] %*.*s\n", dist, dist, last );
				break;

			case LogType::Raw:
				output += vstring( "%*.*s\n", dist, dist, last );
				break;

			case LogType::Warning:
				output += vstring( "[%s] [WARNING] %*.*s", channel->aName.data(), dist, dist, last );
				break;

			case LogType::Error:
				output += vstring( "[%s] [ERROR] %*.*s", channel->aName.data(), dist, dist, last );
				break;

			case LogType::Fatal:
				output += vstring( "[%s] [FATAL] %*.*s", channel->aName.data(), dist, dist, last );
				break;
		}

		if ( !find )
			break;

		last = ++find;
		find = strchr( last, '\n' );
	}

	return output;
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
		case LogColor::DarkPurple:
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
		case LogColor::Purple:
            return {0.6, 0, 0.9, 1};
        case LogColor::Yellow:
            return {1, 1, 0, 1};
        case LogColor::Gray:
            return {0.7, 0.7, 0.7, 1};

        case LogColor::Default:
        case LogColor::Count:
        default:
            return {1, 1, 1, 1};
    }
}


constexpr glm::vec4 GetColorRGBA( LogChannel_t *channel, const Log& log )
{
    return GetColorRGBA( GetColor( channel, log.aType ) );
}

    
u32 GetColorU32( LogChannel_t *channel, const Log& log )
{
    // i don't like this
	glm::vec4 colorVec   = GetColorRGBA( GetColor( channel, log.aType ) );

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


// TODO: make this async and probably store elsewhere
void Log_BuildHistoryString( std::string& srOutput, int sMaxSize )
{
	PROF_SCOPE();

	gLogMutex.lock();

	// No limit
	if ( sMaxSize == -1 )
	{
		// // Preallocate String for all logs
		// size_t stringLen = 0;
		// for ( auto& log : gLogHistory )
		// {
		// 	if ( Log_IsVisible( log ) )
		// 		stringLen += log.aFormatted.size();
		// }
		// 
		// srOutput.reserve( stringLen );

		// Add formatted logs to the strings
		for ( auto& log : gLogHistory )
		{
			if ( !Log_IsVisible( log ) )
				continue;

			srOutput += FormatLogNoColors( log );
		}

		gLogMutex.unlock();

		return;
	}

	// go from latest to oldest
	for ( size_t i = gLogHistory.size() - 1; i > 0; i-- )
	{
		auto& log = gLogHistory[ i ];

		if ( !Log_IsVisible( log ) )
			continue;

		int strLen   = glm::min( sMaxSize, (int)log.aFormatted.length() );
		int strStart = log.aFormatted.length() - strLen;

		// if the length wanted is less then the string length, then start at an offset
		srOutput     = log.aFormatted.substr( strStart, strLen ) + srOutput;

		sMaxSize -= strLen;

		if ( sMaxSize == 0 )
			break;
	}

	gLogMutex.unlock();
}


void Log_SplitStringColors( LogColor sMainColor, std::string_view sBuffer, ChVector< LogColorBuf_t >& srColorList, bool sNoColors )
{
	// on win32, we need to split up the string by colors
	char* buf  = const_cast< char* >( sBuffer.data() );

	char* find = strstr( buf, "\033[" );

	// no color codes found
	if ( find == nullptr )
	{
		LogColorBuf_t& colorBuf = srColorList.emplace_back();
		colorBuf.aColor         = sMainColor;
		colorBuf.aLen           = sBuffer.size();
		colorBuf.apStr          = sBuffer.data();
		return;
	}

	// no color code at start of string
	if ( find != buf )
	{
		LogColorBuf_t& colorBuf = srColorList.emplace_back();
		colorBuf.aColor         = sMainColor;
		colorBuf.aLen           = find - buf;
		colorBuf.apStr          = buf;
	}

	char* last = buf;
	
	while ( find )
	{
		char* endColor = strstr( find, "m" );

		if ( !endColor )
		{
			Log_Error( "Invalid Unix Color Code!\n" );
			break;
		}

		endColor++;

		size_t colorLength         = endColor - find;

		last                       = endColor;
		char*          nextFind    = strstr( endColor, "\033[" );

		// no characters in between these colors
		if ( nextFind && nextFind - endColor == 0 )
		{
			find = nextFind;
			continue;
		}

		LogColorBuf_t& colorBuf    = srColorList.emplace_back();
		colorBuf.apStr             = endColor;

		if ( !sNoColors )
			colorBuf.aColor = Log_UnixToColor( find, colorLength );

		if ( nextFind == nullptr )
		{
			colorBuf.aLen = sBuffer.size() - (last - buf);
		}
		else
		{
			// colorBuf.aLen = find - last;
			colorBuf.aLen = nextFind - endColor;
			last          = endColor;
		}

		find = nextFind;
	}
}


// print to system console
void Log_SysPrint( LogColor sMainColor, const std::string& srBuffer, FILE* spStream )
{
#ifdef _WIN32
	// on win32, we need to split up the string by colors
	ChVector< LogColorBuf_t > colorList;
	Log_SplitStringColors( sMainColor, srBuffer, colorList );

	for ( LogColorBuf_t& colorBuffer : colorList )
	{
		Log_SetColor( colorBuffer.aColor );
		fprintf( spStream, "%*.*s", colorBuffer.aLen, colorBuffer.aLen, colorBuffer.apStr );
	}

#if 0
	if ( IsDebuggerPresent() )
	{
		// doesn't work?
		// http://unixwiz.net/techtips/outputdebugstring.html
		OutputDebugStringA( srBuffer.c_str() );
	}
#endif

	Log_SetColor( LogColor::Default );
	fflush( spStream );
#else
	Log_SetColor( sMainColor );
	fputs( srBuffer.c_str(), spStream );
	Log_SetColor( LogColor::Default );
	fflush( spStream );
#endif
}


CONCMD( log_test_colors )
{
	Log_Msg( UNIX_CLR_DARK_GREEN "TEST " UNIX_CLR_CYAN "TEST CYAN " UNIX_CLR_DARK_PURPLE UNIX_CLR_DARK_BLUE "TEST DARK BLUE \n" );
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

    log.aFormatted = FormatLog( channel, log.aType, log.aMessage.c_str() );

    if ( channel->aShown )
	{
        switch ( log.aType )
        {
            default:
            case LogType::Normal:
            case LogType::Input:
            case LogType::Raw:
				Log_SysPrint( channel->aColor, log.aFormatted, stdout );
                break;

            case LogType::Dev:
            case LogType::Dev2:
            case LogType::Dev3:
            case LogType::Dev4:
				if ( !Log_DevLevelVisible( log ) )
                    break;

				Log_SysPrint( channel->aColor, log.aFormatted, stdout );
                break;

            case LogType::Warning:
				Log_SysPrint( LOG_COLOR_WARNING, log.aFormatted, stdout );
                break;

            case LogType::Error:
				Log_SysPrint( LOG_COLOR_ERROR, log.aFormatted, stderr );
                break;

            case LogType::Fatal:
				Log_SysPrint( LOG_COLOR_ERROR, log.aFormatted, stderr );

                std::string messageBoxTitle;
				vstring( messageBoxTitle, "[%s] Fatal Error", channel->aName.data() );

                if ( log.aMessage.ends_with("\n") )
                    SDL_ShowSimpleMessageBox( SDL_MESSAGEBOX_ERROR, messageBoxTitle.c_str(), log.aMessage.substr(0, log.aMessage.size()-1 ).c_str(), NULL );
                else
                    SDL_ShowSimpleMessageBox( SDL_MESSAGEBOX_ERROR, messageBoxTitle.c_str(), log.aMessage.c_str(), NULL );

                sys_debug_break();
				exit( -1 );
                break;
        }
    }

    Log_Tracy( channel, log );
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


std::string_view Log_GetChannelName( LogChannel handle )
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
	return channel && channel->aShown;
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
// Log Group Functions


LogGroup Log_GroupBeginEx( LogChannel channel, LogType sType )
{
	// Generate a magic number to serve as the handle
	unsigned int magic = ( rand() % 0xFFFFFFFE ) + 1;

	Log&         log   = gLogGroups[ magic ];
	log.aChannel       = channel;
	log.aType          = sType;

	return magic;
}


LogGroup Log_GroupBegin( LogChannel channel )
{
	return Log_GroupBeginEx( channel, LogType::Normal );
}


LogGroup Log_GroupBegin()
{
	return Log_GroupBeginEx( gLC_General, LogType::Normal );
}


void Log_GroupEnd( LogGroup sGroup )
{
	auto it = gLogGroups.find( sGroup );
	if ( it == gLogGroups.end() )
	{
		Log_Error( "Log_GroupEnd: Failed to Find Log Group!\n" );
		return;
	}

	gLogMutex.lock();

	// Submit the built log
	gLogHistory.push_back( it->second );
	Log_AddLogInternal( gLogHistory.back() );
	gLogGroups.erase( it );

	gLogMutex.unlock();
}


void Log_Group( LogGroup sGroup, const char* spBuf )
{
	PROF_SCOPE();

	auto it = gLogGroups.find( sGroup );
	if ( it == gLogGroups.end() )
	{
		Log_Error( "Log_GroupEx: Failed to Find Log Group!\n" );
		return;
	}

	Log& log = it->second;
	
	log.aMessage += spBuf;
}


void Log_GroupF( LogGroup sGroup, const char* spFmt, ... )
{
	va_list args;
	va_start( args, spFmt );
	Log_GroupV( sGroup, spFmt, args );
	va_end( args );
}


void Log_GroupV( LogGroup sGroup, const char* spFmt, va_list args )
{
	PROF_SCOPE();

	auto it = gLogGroups.find( sGroup );
	if ( it == gLogGroups.end() )
	{
		Log_Error( "Log_GroupExV: Failed to Find Log Group!\n" );
		return;
	}

	it->second.aMessage += vstringV( spFmt, args );
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
	Log_Ex( gLC_General, LogType::Normal, spBuf );
}

void CORE_API Log_MsgF( const char* spFmt, ... )
{
	LOG_SYS_MSG_VA( spFmt, gLC_General, LogType::Normal );
}

// Medium severity.
void CORE_API Log_Warn( const char* spBuf )
{
	Log_Ex( gLC_General, LogType::Warning, spBuf );
}

void CORE_API Log_WarnF( const char* spFmt, ... )
{
	LOG_SYS_MSG_VA( spFmt, gLC_General, LogType::Warning );
}

// High severity.
void CORE_API Log_Error( const char* spBuf )
{
	Log_Ex( gLC_General, LogType::Error, spBuf );
}

void CORE_API Log_ErrorF( const char* spFmt, ... )
{
	LOG_SYS_MSG_VA( spFmt, gLC_General, LogType::Error );
}

// Extreme severity.
void CORE_API Log_Fatal( const char* spBuf )
{
	Log_Ex( gLC_General, LogType::Error, spBuf );
}

void CORE_API Log_FatalF( const char* spFmt, ... )
{
	LOG_SYS_MSG_VA( spFmt, gLC_General, LogType::Fatal );
}

// Dev only.
void CORE_API Log_Dev( u8 sLvl, const char* spBuf )
{
	if ( sLvl < 1 || sLvl > 4 )
		sLvl = 4;

	Log_Ex( gLC_General, (LogType)sLvl, spBuf );
}

void CORE_API Log_DevF( u8 sLvl, const char* spFmt, ... )
{
	if ( sLvl < 1 || sLvl > 4 )
		sLvl = 4;

	LOG_SYS_MSG_VA( spFmt, gLC_General, (LogType)sLvl );
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

		results.push_back( channel.aName.data() );
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

	Log_RunCallbacksChannelShown();
}


CONCMD_DROP( log_channel_show, log_channel_dropdown )
{
    if ( args.size() == 0 )
        return;

    LogChannel_t* channel = Log_GetChannelByName( args[ 0 ].c_str() );
    if ( !channel )
        return;

    channel->aShown = true;

	Log_RunCallbacksChannelShown();
}


CONCMD( log_channel_dump )
{
    // Calculate max name length
	size_t maxNameLength = 0;
	size_t maxLength = 0;
	constexpr size_t logNameLen = 13;

    for ( const auto& channel : GetLogChannels() )
	{
		maxNameLength = std::max( logNameLen, std::max( maxNameLength, channel.aName.size() ) );
		maxLength = std::max( maxLength, maxNameLength + 23 );
	}

	LogGroup group = Log_GroupBegin( gLC_Logging );

    Log_GroupF( group, "Channel Name%*s  | Shown  | Color\n", maxNameLength > logNameLen ? maxNameLength - logNameLen : logNameLen, "" );

    // Build separator bar
    // Example Output: ---------------|--------|------------
    char* separator = new char[ maxLength + 1 ] {'\0'};

    for ( size_t i = 0; i < maxLength; i++ )
	{
		if ( i == maxNameLength + 1 || maxLength - 13 == i )
		{
			separator[ i ] = '|';
			continue;
		}
		
	    separator[ i ] = '-';
	}

	Log_GroupF( group, "%s\n", separator );

    delete[] separator;

    // Display Log Channels
    for ( const auto& channel : GetLogChannels() )
	{
		Log_GroupF(
		  group,
		  "%s%s%*s | %s | %s\n",
		  Log_ColorToUnix( channel.aColor ),
		  channel.aName.data(),
		  maxNameLength - channel.aName.size(),
		  "",
		  channel.aShown ? "Shown " : "Hidden",
		  Log_ColorToStr( channel.aColor ) );
    }

	Log_GroupEnd( group );
}


CONCMD( log_color_dump )
{
	Log_Msg( "Color Dump\n" );
	Log_Msg( "------------\n" );

	for ( char i = 0; i < (char)LogColor::Count; i++ )
	{
		Log_MsgF( "%s%s\n", Log_ColorToUnix( (LogColor)i ), Log_ColorToStr( (LogColor)i ) );
    }
}


CONCMD_VA( clear, "Clear Logging System History" )
{
	gLogMutex.lock();
	
	gLogHistory.clear();
	gLogHistory.shrink_to_fit();

	gLogMutex.unlock();
}

#define LOG_DUMP_FILENAME "log_dump"

// Get current date/time, format is YYYY-MM-DD_HH-mm-ss
size_t Util_CurrentDateTime( char* spBuf, int sSize )
{
	time_t    now = time( 0 );
	struct tm tstruct;
	tstruct = *localtime( &now );

	return strftime( spBuf, sSize, "%Y-%m-%d_%H-%M-%S", &tstruct );
}


CONCMD_VA( log_dump, "Dump Logging History to file" )
{
	std::string outputPath;

	char        time[ 80 ];
	size_t      len = Util_CurrentDateTime( time, 80 );

	// If no manual output path specified/allowed, use the default filename
	if ( args.size() )
	{
		vstring( outputPath, "%s_%s.txt", args[ 0 ].c_str(), time );
	}
	else
	{
		vstring( outputPath, LOG_DUMP_FILENAME "_%s.txt", time );
	}

	if ( FileSys_IsDir( outputPath, true ) )
	{
		Log_ErrorF( gLC_Logging, "Output file for log_dump already exists as a directory: \"%s\"\n", outputPath.c_str() );
		return;
	}

	std::string output;
	Log_BuildHistoryString( output, -1 );

	// Write the data
	FILE* fp = fopen( outputPath.c_str(), "wb" );

	if ( fp == nullptr )
	{
		Log_ErrorF( gLC_Logging, "Failed to open file handle: \"%s\"\n", outputPath.c_str() );
		return;
	}

	fwrite( output.c_str(), sizeof( char ), output.size(), fp );
	fclose( fp );

	Log_DevF( gLC_Logging, 1, "Wrote Log History to File: \"%s\"\n", outputPath.c_str() );
}

