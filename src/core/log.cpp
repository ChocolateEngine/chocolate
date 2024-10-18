#include "core/log.h"
#include "core/console.h"
#include "core/profiler.h"

#include <iostream>
#include <mutex>
#include <regex>
#include <stack>

#include <SDL.h>

CONVAR_INT( log_dev_global, 1, "Base Developer Logging Level for all Log Channels", 0 );

ELogColor                     gCurrentColor = ELogColor_Default;
LogChannel                   gLC_General   = INVALID_LOG_CHANNEL;
LogChannel                   gLC_Logging   = INVALID_LOG_CHANNEL;
static ch_string             gDefaultChannelName( "General", 7 );
static std::mutex            gLogMutex;

// apparently you could of added stuff to this before static initialization got to this, and then you lose some log channels as a result
ChVector< LogChannelDef_t >& GetLogChannels()
{
	static ChVector< LogChannelDef_t > gChannels;
	return gChannels;
}

// static std::vector< LogChannelDef_t >             gChannels;
static std::vector< Log >                      gLogHistory;

static std::vector< LogChannelShownCallbackF > gCallbacksChannelShown;

constexpr glm::vec4                            gVecTo255( 255, 255, 255, 255 );


/* Windows Specific Functions for console text colors.  */
#ifdef _WIN32
#include <consoleapi2.h>
#include <debugapi.h>

constexpr int Win32GetColor( ELogColor color )
{
	switch ( color )
	{
		case ELogColor_Black:
			return 0;
		case ELogColor_White:
			return FOREGROUND_INTENSITY | FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;

		case ELogColor_DarkBlue:
			return FOREGROUND_BLUE;
		case ELogColor_DarkGreen:
			return FOREGROUND_GREEN;
		case ELogColor_DarkCyan:
			return FOREGROUND_GREEN | FOREGROUND_BLUE;
		case ELogColor_DarkRed:
			return FOREGROUND_RED;
		case ELogColor_DarkPurple:
			return FOREGROUND_RED | FOREGROUND_BLUE;
		case ELogColor_DarkYellow:
			return FOREGROUND_RED | FOREGROUND_GREEN;
		case ELogColor_DarkGray:
			return FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;

		case ELogColor_Blue:
			return FOREGROUND_INTENSITY | FOREGROUND_BLUE;
		case ELogColor_Green:
			return FOREGROUND_INTENSITY | FOREGROUND_GREEN;
		case ELogColor_Cyan:
			return FOREGROUND_INTENSITY | FOREGROUND_GREEN | FOREGROUND_BLUE;
		case ELogColor_Red:
			return FOREGROUND_INTENSITY | FOREGROUND_RED;
		case ELogColor_Purple:
			return FOREGROUND_INTENSITY | FOREGROUND_RED | FOREGROUND_BLUE;
		case ELogColor_Yellow:
			return FOREGROUND_INTENSITY | FOREGROUND_RED | FOREGROUND_GREEN;
		case ELogColor_Gray:
			return FOREGROUND_INTENSITY;

		case ELogColor_Default:
		case ELogColor_Count:
		default:
			return FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;
	}
}


void Win32SetColor( ELogColor color )
{
	// um
	HANDLE handle = sys_get_console_window();

	if ( !handle )
	{
		ch_print( "*** Failed to Get Console Window\n" );
		return;
	}

	BOOL result = SetConsoleTextAttribute( handle, Win32GetColor( color ) );

	if ( !result )
	{
		ch_printf( "*** Failed to set console color: %s\n", sys_get_error() );
	}
}

#endif


const char* Log_ColorToStr( ELogColor color )
{
    switch ( color )
    {
        case ELogColor_Black:
            return "Black";
        case ELogColor_White:
            return "White";

        case ELogColor_DarkBlue:
            return "Dark Blue";
        case ELogColor_DarkGreen:
            return "Dark Green";
        case ELogColor_DarkCyan:
            return "Dark Cyan";
        case ELogColor_DarkRed:
            return "Dark Red";
		case ELogColor_DarkPurple:
            return "Dark Purple";
        case ELogColor_DarkYellow:
            return "Dark Yellow";
        case ELogColor_DarkGray:
            return "Dark Gray";

        case ELogColor_Blue:
            return "Blue";
        case ELogColor_Green:
            return "Green";
        case ELogColor_Cyan:
            return "Cyan";
        case ELogColor_Red:
            return "Red";
		case ELogColor_Purple:
            return "Purple";
        case ELogColor_Yellow:
            return "Yellow";
        case ELogColor_Gray:
            return "Gray";

        case ELogColor_Default:
		case ELogColor_Count:
        default:
            return "Default";
    }
}


const char* Log_ColorToUnix( ELogColor color )
{
	switch ( color )
	{
		case ELogColor_Default:
		case ELogColor_Count:
		default:
			return ANSI_CLR_DEFAULT;

		case ELogColor_Black:
			return ANSI_CLR_BLACK;
		case ELogColor_White:
			return ANSI_CLR_WHITE;

		case ELogColor_DarkBlue:
			return ANSI_CLR_DARK_BLUE;
		case ELogColor_DarkGreen:
			return ANSI_CLR_DARK_GREEN;
		case ELogColor_DarkCyan:
			return ANSI_CLR_DARK_CYAN;
		case ELogColor_DarkRed:
			return ANSI_CLR_DARK_RED;
		case ELogColor_DarkPurple:
			return ANSI_CLR_DARK_PURPLE;
		case ELogColor_DarkYellow:
			return ANSI_CLR_DARK_YELLOW;
		case ELogColor_DarkGray:
			return ANSI_CLR_DARK_GRAY;

		case ELogColor_Blue:
			return ANSI_CLR_BLUE;
		case ELogColor_Green:
			return ANSI_CLR_GREEN;
		case ELogColor_Cyan:
			return ANSI_CLR_CYAN;
		case ELogColor_Red:
			return ANSI_CLR_RED;
		case ELogColor_Purple:
			return ANSI_CLR_PURPLE;
		case ELogColor_Yellow:
			return ANSI_CLR_YELLOW;
		case ELogColor_Gray:
			return ANSI_CLR_GRAY;
	}
}


ch_string CORE_API Log_ColorToUnixStr( ELogColor color )
{
	switch ( color )
	{
		case ELogColor_Default:
		case ELogColor_Count:
		default:
			return ch_string( Log_ColorToUnix( color ), 4 );

		case ELogColor_Black:
		case ELogColor_White:
		case ELogColor_DarkBlue:
		case ELogColor_DarkGreen:
		case ELogColor_DarkCyan:
		case ELogColor_DarkRed:
		case ELogColor_DarkPurple:
		case ELogColor_DarkYellow:
		case ELogColor_DarkGray:
		case ELogColor_Blue:
		case ELogColor_Green:
		case ELogColor_Cyan:
		case ELogColor_Red:
		case ELogColor_Purple:
		case ELogColor_Yellow:
		case ELogColor_Gray:
			return ch_string( Log_ColorToUnix( color ), 7 );
	}
}


// check the specifc color int
ELogColor Log_UnixCodeToColor( bool sIsLight, int sColor )
{
	if ( sColor == 0 )
		return ELogColor_Default;

	switch ( sColor )
	{
		default:
			return ELogColor_Default;

		// case 30:
		// 	return sIsLight ? ELogColor_Default : ELogColor_Black;

		case 37:
			return sIsLight ? ELogColor_White : ELogColor_Default;

		case 34:
			return sIsLight ? ELogColor_Blue : ELogColor_DarkBlue;

		case 32:
			return sIsLight ? ELogColor_Green : ELogColor_DarkGreen;

		case 36:
			return sIsLight ? ELogColor_Cyan : ELogColor_DarkCyan;

		case 31:
			return sIsLight ? ELogColor_Red : ELogColor_DarkRed;

		case 35:
			return sIsLight ? ELogColor_Purple : ELogColor_DarkPurple;

		case 33:
			return sIsLight ? ELogColor_Yellow : ELogColor_DarkYellow;

		case 30:
			return sIsLight ? ELogColor_Gray : ELogColor_DarkGray;
	}
}


ELogColor Log_UnixToColor( const char* spColor, size_t sLen )
{
	if ( sLen == 4 )
	{
		// \033[0m
		return ELogColor_Default;
	}

	if ( sLen != 7 )
	{
		ch_printf( " *** Log_UnixToColor: Unknown Color Type: \"%s\"\n", spColor );
		return ELogColor_Default;
	}

	bool isLight       = spColor[ 2 ] == '1';
	char colorStr[ 3 ] = { spColor[ 4 ], spColor[ 5 ], '\0' };

	long color         = 0;
	if ( !ToLong3( colorStr, color ) )
	{
		ch_printf( " *** Failed get color code from string: %s (%s)\n", spColor, colorStr );
		return ELogColor_Default;
	}

	return Log_UnixCodeToColor( isLight, color );
}


void UnixSetColor( ELogColor color )
{
	fputs( Log_ColorToUnix( color ), stdout );
}


void Log_Init()
{
	gLC_General = Log_RegisterChannel( "General", ELogColor_Default );
	gLC_Logging = Log_RegisterChannel( "Logging", ELogColor_Default );
}


void Log_Shutdown()
{
	// TODO: FREE ALL STRING MEMORY
	// except logging channel names, those are static

	for ( auto& log : gLogHistory )
	{
		if ( log.aMessage.data )
			ch_str_free( log.aMessage.data );

		if ( log.aFormatted.data )
			ch_str_free( log.aFormatted.data );
	}
}


LogChannel Log_RegisterChannel( const char *sName, ELogColor sColor )
{
	ch_string name;
	name.data = (char*)sName;
	name.size = strlen( sName );

	if ( name.size == 0 )
	{
		ch_printf( " *** LogSystem: Invalid Channel Name: \"%s\"\n", sName );
		return INVALID_LOG_CHANNEL;
	}

	for ( size_t i = 0; i < GetLogChannels().size(); i++ )
    {
		LogChannelDef_t* channel = &GetLogChannels()[ i ];
        if ( !ch_str_equals( channel->aName, name ) )
            continue;
            
        return i;
    }

    LogChannelDef_t& channel = GetLogChannels().emplace_back();
	channel.aName            = name;
	channel.aShown           = true;
	channel.aColor           = sColor;

	return (LogChannel)GetLogChannels().size() - 1U;
}


LogChannel Log_GetChannel( const char* sChannel )
{
	for ( int i = 0; i < GetLogChannels().size(); i++ )
	{
		LogChannelDef_t* channel = &GetLogChannels()[ i ];
		if ( !ch_str_equals( channel->aName, sChannel ) )
			continue;

		return (LogChannel)i;
	}

	return INVALID_LOG_CHANNEL;
}


LogChannelDef_t* Log_GetChannelData( LogChannel sChannel )
{
	if ( sChannel >= GetLogChannels().size() )
        return nullptr;

    return &GetLogChannels()[ sChannel ];
}


LogChannelDef_t* Log_GetChannelByName( const char* sChannel )
{
	for ( int i = 0; i < GetLogChannels().size(); i++ )
    {
		LogChannelDef_t* channel = &GetLogChannels()[ i ];
		if ( !ch_str_equals( channel->aName, sChannel ) )
            continue;

        return channel;
    }

    return nullptr;
}


constexpr ELogColor GetColor( LogChannelDef_t* channel, ELogType sType )
{
	switch ( sType )
	{
		default:
		case ELogType_Normal:
		case ELogType_Dev:
		case ELogType_Dev2:
		case ELogType_Dev3:
		case ELogType_Dev4:
		case ELogType_Raw:
			return channel->aColor;

		case ELogType_Warning:
			return LOG_COLOR_WARNING;

		case ELogType_Error:
		case ELogType_Fatal:
			return LOG_COLOR_ERROR;
	}
}


static bool CheckConcatRealloc( ch_string& newOutput, ch_string& output )
{
	if ( !newOutput.data )
	{
		ch_print( " *** LogSystem: Failed to Concatenate Strings during realloc!\n" );
		return false;
	}

	output = newOutput;
	return true;
}


static bool FormatLog_Dev( LogChannelDef_t* channel, ch_string& output, const char* devLevel, const char* last, size_t dist )
{
	const char* strings[] = { "[", channel->aName.data, "] [DEV ", devLevel, "] ", last };
	const u64   lengths[] = { 1,   channel->aName.size, 7,         1,        2,    dist };
	ch_string   newOutput = ch_str_concat( CH_STR_UR( output ), 6, strings, lengths );

	return CheckConcatRealloc( newOutput, output );
}


static bool FormatLog_Dev( LogChannelDef_t* channel, ch_string& output, ch_string& color, const char* devLevel, const char* last, size_t dist )
{
	const char* strings[] = { color.data, "[", channel->aName.data, "] [DEV ", devLevel, "] ", last };
	const u64   lengths[] = { color.size, 1,   channel->aName.size, 7,         1,        2,    dist };
	ch_string   newOutput = ch_str_concat( CH_STR_UR( output ), 7, strings, lengths );

	return CheckConcatRealloc( newOutput, output );
}


// Format for system console output
static ch_string FormatLog( LogChannelDef_t* channel, ELogType sType, const char* spMessage, s64 len )
{
    PROF_SCOPE();

	if ( !spMessage || len == 0 )
		return ch_string();

	// Special Exception for "Raw" ELogType
	if ( sType == ELogType_Raw )
		return ch_str_copy( spMessage, len );

	// Split by New Line characters
	ch_string   output;

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

		ch_string color = Log_ColorToUnixStr( GetColor( channel, sType ) );

		switch ( sType )
		{
			default:
			case ELogType_Normal:
			{
				const char* strings[] = { color.data, "[", channel->aName.data, "] ", last };
				const u64   lengths[] = { color.size, 1, channel->aName.size, 2, dist };
				ch_string   newOutput = ch_str_concat( CH_STR_UR( output ), 5, strings, lengths );

				CheckConcatRealloc( newOutput, output );
				break;
			}

			case ELogType_Dev:
				FormatLog_Dev( channel, output, color, "1", last, dist );
				break;

			case ELogType_Dev2:
				FormatLog_Dev( channel, output, color, "2", last, dist );
				break;

			case ELogType_Dev3:
				FormatLog_Dev( channel, output, color, "3", last, dist );
				break;

			case ELogType_Dev4:
				FormatLog_Dev( channel, output, color, "4", last, dist );
				break;

			case ELogType_Warning:
			{
				const char* strings[] = { color.data, "[", channel->aName.data, "] [WARNING] ", last };
				const u64   lengths[] = { color.size, 1, channel->aName.size, 12, dist };
				ch_string   newOutput = ch_str_concat( CH_STR_UR( output ), 5, strings, lengths );

				CheckConcatRealloc( newOutput, output );
				break;
			}

			case ELogType_Error:
			{
				const char* strings[] = { color.data, "[", channel->aName.data, "] [ERROR] ", last };
				const u64   lengths[] = { color.size, 1, channel->aName.size, 10, dist };
				ch_string   newOutput = ch_str_concat( CH_STR_UR( output ), 5, strings, lengths );

				CheckConcatRealloc( newOutput, output );
				break;
			}

			case ELogType_Fatal:
			{
				const char* strings[] = { color.data, "[", channel->aName.data, "] [FATAL] ", last };
				const u64   lengths[] = { color.size, 1, channel->aName.size, 10, dist };
				ch_string   newOutput = ch_str_concat( CH_STR_UR( output ), 5, strings, lengths );

				CheckConcatRealloc( newOutput, output );
				break;
			}
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
static ch_string Log_StripColors( const ch_string& sBuffer )
{
	ch_string output;
	output.data = nullptr;
	output.size = 0;

	char* buf  = sBuffer.data;
	char* find = strstr( buf, "\033[" );

	// no color codes found
	if ( find == nullptr )
		return output;

	// no color code at start of string
	if ( find != buf )
	{
		ch_string newOutput = ch_str_concat( CH_STR_UR( output ), buf, find - buf );

		if ( !newOutput.data )
		{
			ch_print( " *** LogSystem: Failed to Concatenate Strings in Log_StripColors!\n" );
			return output;
		}

		output = newOutput;
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
			ch_string newOutput = ch_str_concat( CH_STR_UR( output ), endColor, sBuffer.size - ( last - buf ) );

			if ( !newOutput.data )
			{
				ch_print( " *** LogSystem: Failed to Concatenate Strings in Log_StripColors!\n" );
				return output;
			}

			output = newOutput;
		}
		else
		{
			// colorBuf.aLen = find - last;
			ch_string newOutput = ch_str_concat( CH_STR_UR( output ), endColor, nextFind - endColor );

			if ( !newOutput.data )
			{
				ch_print( " *** LogSystem: Failed to Concatenate Strings in Log_StripColors!\n" );
				return output;
			}

			output = newOutput;
			last = endColor;
		}

		find = nextFind;
	}

	return output;
}


// Formats the log as normal, but strips all color codes from it
static ch_string FormatLogNoColors( const Log& srLog )
{
	PROF_SCOPE();

	// Split by New Line characters
	ch_string     output;

	LogChannelDef_t* channel = Log_GetChannelData( srLog.aChannel );
	if ( !channel )
	{
		ch_printf( "\n *** LogSystem: Channel Not Found for message: \"%s\"\n", srLog.aMessage.data );
		return output;
	}

	ch_string_auto stripped = Log_StripColors( srLog.aMessage );

	size_t      len  = 0;
	const char* last = nullptr;
	const char* find = nullptr;
	const char* buf = nullptr;

	if ( !stripped.data )
	{
		// No color codes found
		len = srLog.aMessage.size;
		buf = srLog.aMessage.data;
	}
	else
	{
		// Color Codes found
		len = stripped.size;
		buf = stripped.data;
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
			case ELogType_Normal:
			{
				const char* strings[] = { "[", channel->aName.data, "] ", last };
				const u64   lengths[] = { 1, channel->aName.size, 2, dist };
				ch_string   newOutput = ch_str_concat( CH_STR_UR( output ), 4, strings, lengths );

				CheckConcatRealloc( newOutput, output );
				break;
			}

			case ELogType_Dev:
				FormatLog_Dev( channel, output, "1", last, dist );
				break;

			case ELogType_Dev2:
				FormatLog_Dev( channel, output, "2", last, dist );
				break;

			case ELogType_Dev3:
				FormatLog_Dev( channel, output, "3", last, dist );
				break;

			case ELogType_Dev4:
				FormatLog_Dev( channel, output, "4", last, dist );
				break;

			case ELogType_Raw:
			{
				ch_string newOutput = ch_str_concat( CH_STR_UR( output ), last, dist );
				CheckConcatRealloc( newOutput, output );
				break;
			}

			case ELogType_Warning:
			{
				const char* strings[] = { "[", channel->aName.data, "] [WARNING] ", last };
				const u64   lengths[] = { 1, channel->aName.size, 12, dist };
				ch_string   newOutput = ch_str_concat( CH_STR_UR( output ), 4, strings, lengths );

				CheckConcatRealloc( newOutput, output );
				break;
			}

			case ELogType_Error:
			{
				const char* strings[] = { "[", channel->aName.data, "] [ERROR] ", last };
				const u64   lengths[] = { 1, channel->aName.size, 10, dist };
				ch_string   newOutput = ch_str_concat( CH_STR_UR( output ), 4, strings, lengths );

				CheckConcatRealloc( newOutput, output );
				break;
			}

			case ELogType_Fatal:
			{
				const char* strings[] = { "[", channel->aName.data, "] [FATAL] ", last };
				const u64   lengths[] = { 1, channel->aName.size, 10, dist };
				ch_string   newOutput = ch_str_concat( CH_STR_UR( output ), 4, strings, lengths );

				CheckConcatRealloc( newOutput, output );
				break;
			}
		}

		if ( !find )
			break;

		last = ++find;
		find = strchr( last, '\n' );
	}

	return output;
}


// copied from graphics/gui/consoleui.cpp
constexpr glm::vec4 GetColorRGBA( ELogColor col )
{
    switch (col)
    {
        // hmm
        case ELogColor_Black:
            return {0.3, 0.3, 0.3, 1};
        case ELogColor_White:
            return {1, 1, 1, 1};

        case ELogColor_DarkBlue:
            return {0, 0.3, 0.8, 1};
        case ELogColor_DarkGreen:
            return {0.25, 0.57, 0.25, 1};
        case ELogColor_DarkCyan:
            return {0, 0.35, 0.75, 1};
        case ELogColor_DarkRed:
            return {0.7, 0, 0.25, 1};
		case ELogColor_DarkPurple:
            return {0.45, 0, 0.7, 1};
        case ELogColor_DarkYellow:
            return {0.6, 0.6, 0, 1};
        case ELogColor_DarkGray:
            return {0.45, 0.45, 0.45, 1};

        case ELogColor_Blue:
            return {0, 0.4, 1, 1};
        case ELogColor_Green:
            return {0.4, 0.9, 0.4, 1};
        case ELogColor_Cyan:
            return {0, 0.85, 1, 1};
        case ELogColor_Red:
            return {0.9, 0, 0.4, 1};
		case ELogColor_Purple:
            return {0.6, 0, 0.9, 1};
        case ELogColor_Yellow:
            return {1, 1, 0, 1};
        case ELogColor_Gray:
            return {0.7, 0.7, 0.7, 1};

        case ELogColor_Default:
        case ELogColor_Count:
        default:
            return {1, 1, 1, 1};
    }
}


constexpr glm::vec4 GetColorRGBA( LogChannelDef_t *channel, const Log& log )
{
    return GetColorRGBA( GetColor( channel, log.aType ) );
}


u32 GetColorU32( ELogColor sColor )
{
	// i don't like this
	glm::vec4 colorVec     = GetColorRGBA( sColor );

	u8        colorU8[ 4 ] = {
			   static_cast< u8 >( colorVec.x * 255 ),
			   static_cast< u8 >( colorVec.y * 255 ),
			   static_cast< u8 >( colorVec.z * 255 ),
			   static_cast< u8 >( colorVec.w * 255 ),
	};

	// what
	u32 color = *( (u32*)colorU8 );

	return color;
}

    
u32 GetColorU32( LogChannelDef_t *channel, const Log& log )
{
	return GetColorU32( GetColor( channel, log.aType ) );
}


// Is this ELogType one of the developer levels?
inline bool Log_IsDevType( ELogType sType )
{
	return ( sType == ELogType_Dev ||
	         sType == ELogType_Dev2 ||
	         sType == ELogType_Dev3 ||
	         sType == ELogType_Dev4 );
}


inline bool Log_DevLevelVisible( const Log& log )
{
	LogChannelDef_t* channel = Log_GetChannelData( log.aChannel );

	if ( channel )
	{
		if ( channel->aDevLevel >= (int)log.aType && Log_IsDevType( log.aType ) )
			return true;
	}

	if ( log_dev_global < (int)log.aType && Log_IsDevType( log.aType ) )
		return false;

	return true;
}


static void StripTrailingLines( const char* pText, size_t& len )
{
	while ( len > 1 )
	{
		switch ( pText[ len - 1 ] )
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


inline bool Log_TracyEnabled()
{
#ifdef TRACY_ENABLE
	return tracy::ProfilerAvailable();
#else
	return false;
#endif
}


inline void Log_Tracy( ELogColor sMainColor, const char* sMsg, s64 msgLen )
{
#ifdef TRACY_ENABLE
	if ( !tracy::ProfilerAvailable() )
		return;

	StripTrailingLines( sMsg, msgLen );
	TracyMessageC( sMsg, msgLen, GetColorU32( sMainColor ) );
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
ch_string Log_BuildHistoryString( int sMaxSize )
{
	PROF_SCOPE();

	gLogMutex.lock();

	ch_string output;
	output.data = nullptr;
	output.size = 0;

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

			ch_string append = FormatLogNoColors( log );

			if ( !append.data || append.size == 0 )
				continue;

			ch_string newOutput = ch_str_concat( CH_STR_UR( output ), append.data, append.size );

			if ( !CheckConcatRealloc( newOutput, output ) )
				break;
		}

		gLogMutex.unlock();

		return output;
	}

	// go from latest to oldest
	for ( size_t i = gLogHistory.size() - 1; i > 0; i-- )
	{
		auto& log = gLogHistory[ i ];

		if ( !Log_IsVisible( log ) )
			continue;

		int strLen   = glm::min( sMaxSize, (int)log.aFormatted.size );
		int strStart = log.aFormatted.size - strLen;

		// if the length wanted is less then the string length, then start at an offset
		ch_string section   = ch_str_copy( log.aFormatted.data + strStart, strLen );

		if ( !section.data )
			continue;

		ch_string newOutput = ch_str_concat( CH_STR_UR( section ), CH_STR_UR( output ) );

		if ( !CheckConcatRealloc( newOutput, output ) )
			continue;

		sMaxSize -= strLen;

		if ( sMaxSize == 0 )
			break;
	}

	gLogMutex.unlock();
	return output;
}


void Log_SplitStringColors( ELogColor sMainColor, const ch_string& sBuffer, ChVector< LogColorBuf_t >& srColorList, bool sNoColors )
{
	// on win32, we need to split up the string by colors
	char* buf  = const_cast< char* >( sBuffer.data );

	char* find = strstr( buf, "\033[" );

	// no color codes found
	if ( find == nullptr )
	{
		LogColorBuf_t& colorBuf = srColorList.emplace_back();
		colorBuf.aColor         = sMainColor;
		colorBuf.aLen           = sBuffer.size;
		colorBuf.apStr          = sBuffer.data;
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

		size_t colorLength = endColor - find;
		last               = endColor;
		char* nextFind     = strstr( endColor, "\033[" );

		// no characters in between these colors
		if ( nextFind && nextFind - endColor == 0 )
		{
			find = nextFind;
			continue;
		}

		LogColorBuf_t& colorBuf = srColorList.emplace_back();
		colorBuf.apStr          = endColor;

		if ( !sNoColors )
			colorBuf.aColor = Log_UnixToColor( find, colorLength );

		if ( nextFind == nullptr )
		{
			colorBuf.aLen = sBuffer.size - (last - buf);
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


// print to system console and tracy
void Log_SysPrint( ELogColor sMainColor, const Log& srLog, FILE* spStream )
{
#ifdef _WIN32
	
	// on win32, we need to split up the string by colors
	ChVector< LogColorBuf_t > colorList;
	Log_SplitStringColors( sMainColor, srLog.aFormatted, colorList );

	for ( LogColorBuf_t& colorBuffer : colorList )
	{
		Log_SetColor( colorBuffer.aColor );
		fprintf( spStream, "%*.*s", colorBuffer.aLen, colorBuffer.aLen, colorBuffer.apStr );
	}

	Log_SetColor( ELogColor_Default );
	fflush( spStream );

  #if !TRACY_ENABLE
	if ( !IsDebuggerPresent() )
		return;
  #endif

	ch_string_auto debugString = FormatLogNoColors( srLog );
	OutputDebugStringA( debugString.data );

	if ( Log_TracyEnabled() )
		Log_Tracy( sMainColor, CH_STR_UR( debugString ) );
#else
	Log_SetColor( sMainColor );
	fputs( srLog.aFormatted.c_str(), spStream );
	Log_SetColor( ELogColor_Default );
	fflush( spStream );

	if ( Log_TracyEnabled() )
	{
		std::string debugString = FormatLogNoColors( srLog );
		Log_Tracy( sMainColor, debugString );
	}
#endif
}


CONCMD( log_test_colors )
{
	Log_Msg( ANSI_CLR_DARK_GREEN "TEST " ANSI_CLR_CYAN "TEST CYAN " ANSI_CLR_DARK_PURPLE ANSI_CLR_DARK_BLUE "TEST DARK BLUE \n" );
}


void Log_AddLogInternal( Log& log )
{
    PROF_SCOPE();

    LogChannelDef_t* channel = Log_GetChannelData( log.aChannel );
    if ( !channel )
    {
        ch_printf( "\n *** LogSystem: Channel Not Found for message: \"%s\"\n", log.aMessage.data );
        return;
    }

    log.aFormatted = FormatLog( channel, log.aType, CH_STR_UR( log.aMessage ) );

    if ( channel->aShown )
	{
        switch ( log.aType )
        {
            default:
            case ELogType_Normal:
            case ELogType_Raw:
				Log_SysPrint( channel->aColor, log, stdout );
                break;

            case ELogType_Dev:
            case ELogType_Dev2:
            case ELogType_Dev3:
            case ELogType_Dev4:
				if ( !Log_DevLevelVisible( log ) )
                    break;

				Log_SysPrint( channel->aColor, log, stdout );
                break;

            case ELogType_Warning:
				Log_SysPrint( LOG_COLOR_WARNING, log, stdout );
                break;

            case ELogType_Error:
				Log_SysPrint( LOG_COLOR_ERROR, log, stderr );
                break;

            case ELogType_Fatal:
				Log_SysPrint( LOG_COLOR_ERROR, log, stderr );

				const char*    strings[]       = { "[", channel->aName.data, "] Fatal Error" };
				const u64      lengths[]       = { 1, channel->aName.size, 13 };
				ch_string_auto messageBoxTitle = ch_str_join( 3, strings, lengths );

                if ( ch_str_ends_with( log.aMessage, "\n", 1 ) )
				{
					ch_string_auto substr = ch_str_copy( log.aMessage.data, log.aMessage.size - 1 );
					SDL_ShowSimpleMessageBox( SDL_MESSAGEBOX_ERROR, messageBoxTitle.data, substr.data, NULL );
				}
				else
				{
					SDL_ShowSimpleMessageBox( SDL_MESSAGEBOX_ERROR, messageBoxTitle.data, log.aMessage.data, NULL );
				}

                sys_debug_break();
				exit( -1 );
                break;
        }
    }
}


void Log_SetColor( ELogColor color )
{
    gCurrentColor = color;

#ifdef _WIN32
    Win32SetColor( color );
#elif __unix__
    // technically works in windows 10, might try later and see if it's cheaper, idk
    UnixSetColor( color );
#endif
}


ELogColor Log_GetColor()
{
    return gCurrentColor;
}


LogChannel LogGetChannel( const char* name )
{
	if ( !name )
		return INVALID_LOG_CHANNEL;

	u64 nameLen = strlen( name );

	for ( size_t i = 0; const auto& channel : GetLogChannels() )
    {
        // if ( GetLogSystem().aChannels[i].aName == name )
        if ( ch_str_equals( channel.aName, name, nameLen ) )
            return (LogChannel)i;

        i++;
    }

    return INVALID_LOG_CHANNEL;
}


ELogColor Log_GetChannelColor( LogChannel handle )
{
	LogChannelDef_t* channel = Log_GetChannelData( handle );
    if ( !channel )
        return ELogColor_Default;

    return channel->aColor;
}


ch_string Log_GetChannelName( LogChannel handle )
{
	LogChannelDef_t* channel = Log_GetChannelData( handle );
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
	LogChannelDef_t* channel = Log_GetChannelData( handle );
	return channel && channel->aShown;
}


int Log_GetChannelDevLevel( LogChannel handle )
{
	LogChannelDef_t* channel = Log_GetChannelData( handle );
	if ( !channel )
		return log_dev_global;

	return std::max( log_dev_global, channel->aDevLevel );
}


int Log_GetDevLevel()
{
	return log_dev_global;
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


void ch_printf( const char* format, ... )
{
	va_list args;
	va_start( args, format );
	ch_string msg = ch_str_copy_v( format, args );
	va_end( args );

	fputs( msg.data, stdout );

	ch_str_free( msg.data );
}


void ch_print( const char* buffer )
{
	fputs( buffer, stdout );
}


// ----------------------------------------------------------------
// Log Group Functions


Log& Log_GroupBeginEx( LogChannel channel, ELogType type )
{
	Log log{
		.aChannel = channel,
		.aType    = type,
	};

	return log;
}


Log& Log_GroupBegin( LogChannel channel )
{
	return Log_GroupBeginEx( channel, ELogType_Normal );
}


Log& Log_GroupBegin()
{
	return Log_GroupBeginEx( gLC_General, ELogType_Normal );
}


void Log_GroupEnd( Log& sGroup )
{
	gLogMutex.lock();

	// Submit the built log
	gLogHistory.push_back( sGroup );
	Log_AddLogInternal( gLogHistory.back() );

	gLogMutex.unlock();
}


void Log_Group( Log& sGroup, const char* spBuf )
{
	PROF_SCOPE();

	size_t bufLen = 0;
	if ( !ch_str_check_empty( spBuf, bufLen ) )
		return;

	ch_string append = ch_str_concat( CH_STR_UR( sGroup.aMessage ), spBuf, bufLen );
	CheckConcatRealloc( append, sGroup.aMessage );
}


void Log_GroupF( Log& sGroup, const char* spFmt, ... )
{
	va_list args;
	va_start( args, spFmt );
	Log_GroupV( sGroup, spFmt, args );
	va_end( args );
}


void Log_GroupV( Log& sGroup, const char* spFmt, va_list args )
{
	PROF_SCOPE();

	size_t bufLen = 0;
	if ( !ch_str_check_empty( spFmt, bufLen ) )
		return;

	ch_string formatted = ch_str_copy_v( spFmt, args );

	if ( !formatted.data )
		return;

	// concat it
	ch_string append = ch_str_concat( CH_STR_UR( sGroup.aMessage ), CH_STR_UR( formatted ) );
	CheckConcatRealloc( append, sGroup.aMessage );
}


// ----------------------------------------------------------------
// Extended Logging Functions


#define LOG_SYS_MSG_VA( str, channel, level ) \
    va_list args; \
    va_start( args, str ); \
    Log_ExV( channel, level, str, args ); \
    va_end( args )


bool Log_ShouldAddLog( LogChannel sChannel, ELogType sLevel )
{
	// Is this a developer level?
	if ( !Log_IsDevType( sLevel ) )
		return true;

	// Is the global dev level less than the log's developer level?
	if ( log_dev_global < static_cast< int >( sLevel ) )
	{
		// Check if the channel dev level is less than the log's developer level
		LogChannelDef_t* channel = Log_GetChannelData( sChannel );
		if ( !channel )
		{
			Log_Error( "Unable to find channel when checking developer level\n" );
			return false;
		}

		// log_dev_global is the base value for every channel
		if ( channel->aDevLevel <= log_dev_global )
			return false;

		// Don't even save this log, it's probably flooding the log history
		// and slowing down perf with adding it, and the 2 vnsprintf calls
		// TODO: maybe we can have an convar option to save this anyway?
		if ( channel->aDevLevel < static_cast< int >( sLevel ) )
			return false;
	}

	return true;
}


void Log_Ex( LogChannel sChannel, ELogType sLevel, const char* spBuf )
{
	PROF_SCOPE();

	gLogMutex.lock();

	// Is this a developer level?
	if ( !Log_ShouldAddLog( sChannel, sLevel ) )
	{
		gLogMutex.unlock();
		return;
	}

	ch_string message = ch_str_copy( spBuf );

	gLogHistory.emplace_back( sChannel, sLevel, message );
	Log& log = gLogHistory[ gLogHistory.size() - 1 ];

	Log_AddLogInternal( log );

	gLogMutex.unlock();
}


void Log_ExF( LogChannel sChannel, ELogType sLevel, const char* spFmt, ... )
{
	LOG_SYS_MSG_VA( spFmt, sChannel, sLevel );
}


void Log_ExV( LogChannel sChannel, ELogType sLevel, const char* spFmt, va_list args )
{
	PROF_SCOPE();

	gLogMutex.lock();

	// Is this a developer level?
	if ( !Log_ShouldAddLog( sChannel, sLevel ) )
	{
		gLogMutex.unlock();
		return;
	}

	gLogHistory.emplace_back( sChannel, sLevel );
	Log&    log = gLogHistory[ gLogHistory.size() - 1 ];

	va_list copy;
	va_copy( copy, args );
	int len = std::vsnprintf( nullptr, 0, spFmt, copy );
	va_end( copy );

	if ( len < 0 )
	{
		ch_print( "\n *** LogSystem: vsnprintf failed?\n\n" );
		gLogMutex.unlock();
		return;
	}

	log.aMessage = ch_str_copy_v( spFmt, args );

	if ( !log.aMessage.data )
	{
		ch_print( "\n *** LogSystem: Failed to Allocate Memory for Log Message!\n\n" );
		gLogMutex.unlock();
		return;
	}

	Log_AddLogInternal( log );

	gLogMutex.unlock();
}


// ----------------------------------------------------------------
// Standard Logging Functions

// Lowest severity.
void CORE_API Log_Msg( LogChannel channel, const char* spBuf )
{
	Log_Ex( channel, ELogType_Normal, spBuf );
}

void CORE_API Log_MsgF( LogChannel channel, const char* spFmt, ... )
{
	LOG_SYS_MSG_VA( spFmt, channel, ELogType_Normal );
}

// Medium severity.
void CORE_API Log_Warn( LogChannel channel, const char* spBuf )
{
	Log_Ex( channel, ELogType_Warning, spBuf );
}

void CORE_API Log_WarnF( LogChannel channel, const char* spFmt, ... )
{
	LOG_SYS_MSG_VA( spFmt, channel, ELogType_Warning );
}

// High severity.
void CORE_API Log_Error( LogChannel channel, const char* spBuf )
{
	Log_Ex( channel, ELogType_Error, spBuf );
}

void CORE_API Log_ErrorF( LogChannel channel, const char* spFmt, ... )
{
	LOG_SYS_MSG_VA( spFmt, channel, ELogType_Error );
}

// Extreme severity.
void CORE_API Log_Fatal( LogChannel channel, const char* spBuf )
{
	Log_Ex( channel, ELogType_Fatal, spBuf );
}

void CORE_API Log_FatalF( LogChannel channel, const char* spFmt, ... )
{
	LOG_SYS_MSG_VA( spFmt, channel, ELogType_Fatal );
}

// Dev only.
void CORE_API Log_Dev( LogChannel channel, u8 sLvl, const char* spBuf )
{
	if ( sLvl < 1 || sLvl > 4 )
		sLvl = 4;

	Log_Ex( channel, (ELogType)sLvl, spBuf );
}

void CORE_API Log_DevF( LogChannel channel, u8 sLvl, const char* spFmt, ... )
{
	if ( sLvl < 1 || sLvl > 4 )
		sLvl = 4;

	LOG_SYS_MSG_VA( spFmt, channel, (ELogType)sLvl );
}


// ----------------------------------------------------------------
// Default to general channel.

// Lowest severity.
void CORE_API Log_Msg( const char* spBuf )
{
	Log_Ex( gLC_General, ELogType_Normal, spBuf );
}

void CORE_API Log_MsgF( const char* spFmt, ... )
{
	LOG_SYS_MSG_VA( spFmt, gLC_General, ELogType_Normal );
}

// Medium severity.
void CORE_API Log_Warn( const char* spBuf )
{
	Log_Ex( gLC_General, ELogType_Warning, spBuf );
}

void CORE_API Log_WarnF( const char* spFmt, ... )
{
	LOG_SYS_MSG_VA( spFmt, gLC_General, ELogType_Warning );
}

// High severity.
void CORE_API Log_Error( const char* spBuf )
{
	Log_Ex( gLC_General, ELogType_Error, spBuf );
}

void CORE_API Log_ErrorF( const char* spFmt, ... )
{
	LOG_SYS_MSG_VA( spFmt, gLC_General, ELogType_Error );
}

// Extreme severity.
void CORE_API Log_Fatal( const char* spBuf )
{
	Log_Ex( gLC_General, ELogType_Fatal, spBuf );
}

void CORE_API Log_FatalF( const char* spFmt, ... )
{
	LOG_SYS_MSG_VA( spFmt, gLC_General, ELogType_Fatal );
}

// Dev only.
void CORE_API Log_Dev( u8 sLvl, const char* spBuf )
{
	if ( sLvl < 1 || sLvl > 4 )
		sLvl = 4;

	Log_Ex( gLC_General, (ELogType)sLvl, spBuf );
}

void CORE_API Log_DevF( u8 sLvl, const char* spFmt, ... )
{
	if ( sLvl < 1 || sLvl > 4 )
		sLvl = 4;

	LOG_SYS_MSG_VA( spFmt, gLC_General, (ELogType)sLvl );
}


// =====================================================================================
// Log ConCommands


void log_channel_dropdown(
  const std::vector< std::string >& args,         // arguments currently typed in by the user
  const std::string&                fullCommand,  // the full command line the user has typed in
  std::vector< std::string >&       results )     // results to populate the dropdown list with
{
	for ( const auto& channel : GetLogChannels() )
	{
		if ( args.size() && !( ch_str_starts_with( channel.aName, args[ 0 ].data(), args[ 0 ].size() ) ) )
			continue;

		results.push_back( channel.aName.data );
	}
}


CONCMD_DROP( log_channel_hide, log_channel_dropdown )
{
    if ( args.size() == 0 )
        return;

    LogChannelDef_t* channel = Log_GetChannelByName( args[0].c_str() );
    if ( !channel )
        return;

    channel->aShown = false;

	Log_RunCallbacksChannelShown();
}


CONCMD_DROP( log_channel_show, log_channel_dropdown )
{
    if ( args.size() == 0 )
        return;

    LogChannelDef_t* channel = Log_GetChannelByName( args[ 0 ].c_str() );
    if ( !channel )
        return;

    channel->aShown = true;

	Log_RunCallbacksChannelShown();
}


constexpr const char* LOG_CHANNEL_DUMP_HEADER     = "Channel Name%*s  | Shown  | Developer Level | Color\n";
constexpr s64         LOG_CHANNEL_DUMP_HEADER_LEN = 53;


// TODO: this is not very good, clean this up, adding developer levels broke it and i haven't fixed it yet
CONCMD( log_channel_dump )
{
    // Calculate max name length
	size_t           maxNameLength = 0;
	size_t           maxLength     = 0;
	constexpr size_t logNameLen = 13;

    for ( const auto& channel : GetLogChannels() )
	{
		maxNameLength = std::max( logNameLen, std::max( maxNameLength, channel.aName.size ) );
		// maxLength = std::max( maxLength, maxNameLength + 23 );
		maxLength = std::max( maxLength, maxNameLength + ( LOG_CHANNEL_DUMP_HEADER_LEN - logNameLen ) );
	}

	Log& group = Log_GroupBegin( gLC_Logging );

    // Log_GroupF( group, "Channel Name%*s  | Shown  | Color \n", maxNameLength > logNameLen ? maxNameLength - logNameLen : logNameLen, "" );
	Log_GroupF( group, LOG_CHANNEL_DUMP_HEADER, maxNameLength > logNameLen ? maxNameLength - logNameLen : logNameLen, "" );

    // Build separator bar
    // Example Output: ---------------|--------|------------
    char* separator = new char[ maxLength + 1 ] {'\0'};

    for ( size_t i = 0; i < maxLength; i++ )
	{
		if ( i == maxNameLength + 1 || maxLength - logNameLen == i )
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
		  "%s%s%*s | %s | %d | %s\n",
		  Log_ColorToUnix( channel.aColor ),
		  channel.aName.data,
		  maxNameLength - channel.aName.size,
		  "",
		  channel.aShown ? "Shown " : "Hidden",
		  channel.aDevLevel,
		  Log_ColorToStr( channel.aColor ) );
    }

	Log_GroupEnd( group );
}


CONCMD( log_color_dump )
{
	Log_Msg( "Color Dump\n" );
	Log_Msg( "------------\n" );

	for ( char i = 0; i < (char)ELogColor_Count; i++ )
	{
		Log_MsgF( "%s%s\n", Log_ColorToUnix( (ELogColor)i ), Log_ColorToStr( (ELogColor)i ) );
    }
}


CONCMD_VA( clear, "Clear Logging System History" )
{
	gLogMutex.lock();
	
	gLogHistory.clear();
	gLogHistory.shrink_to_fit();

	Log_RunCallbacksChannelShown();

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

	if ( FileSys_IsDir( outputPath.data(), outputPath.size(), true ) )
	{
		Log_ErrorF( gLC_Logging, "Output file for log_dump already exists as a directory: \"%s\"\n", outputPath.c_str() );
		return;
	}

	if ( FileSys_IsFile( outputPath.data(), outputPath.size(), true ) )
	{
		Log_ErrorF( gLC_Logging, "Output file for log_dump already exists: \"%s\"\n", outputPath.c_str() );
		return;
	}

	ch_string_auto output = Log_BuildHistoryString( -1 );

	// Write the data
	FILE* fp = fopen( outputPath.c_str(), "wb" );

	if ( fp == nullptr )
	{
		Log_ErrorF( gLC_Logging, "Failed to open file handle: \"%s\"\n", outputPath.c_str() );
		return;
	}

	fwrite( output.data, sizeof( char ), output.size, fp );
	fclose( fp );

	Log_DevF( gLC_Logging, 1, "Wrote Log History to File: \"%s\"\n", outputPath.c_str() );
}


static void log_dev_dropdown(
  const std::vector< std::string >& args,         // arguments currently typed in by the user
  const std::string&                fullCommand,  // the full command line the user has typed in
  std::vector< std::string >&       results )     // results to populate the dropdown list with
{
	for ( const LogChannelDef_t& channel : GetLogChannels() )
	{
		// if ( args.size() && !channel.aName.starts_with( args[ 0 ] ) )
		if ( args.size() )
		{
			if ( args[ 0 ].size() > channel.aName.size )
				continue;

			if ( ch_strncasecmp( channel.aName.data, args[ 0 ].data(), args[ 0 ].size() ) != 0 )
				continue;
		}

		std::string value = vstring( "%s %d", channel.aName.data, channel.aDevLevel );
		results.push_back( value );
	}
}


CONCMD_DROP_VA( log_dev, log_dev_dropdown, 0, "Change Log Developer Level of a Channel" )
{
	if ( !args.size() )
	{
		Log_Msg( log_dev_cmd.GetPrintMessage().c_str() );
		return;
	}

	LogChannelDef_t* channel = Log_GetChannelByName( args[ 0 ].c_str() );

	if ( !channel )
	{
		Log_ErrorF( "Log Channel Not Found: %s\n", args[ 0 ].c_str() );
		return;
	}

	if ( args.size() > 1 )
	{
		long out = 0;
		if ( !ToLong2( args[ 1 ], out ) )
		{
			Log_ErrorF( "Failed to convert requested developer to int: %s\n", args[ 1 ].c_str() );
		}
		else
		{
			channel->aDevLevel = std::clamp( out, 0L, 4L );
			Log_MsgF( "Set Developer Level of Log Channel \"%s\" to %d\n", channel->aName.data, channel->aDevLevel );
		}
	}
	else
	{
		Log_MsgF( "Log Channel \"%s\" - Developer Level: %d\n", channel->aName.data, channel->aDevLevel );
	}
}

