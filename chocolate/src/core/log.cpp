#include "core/log.h"
#include "core/console.h"
#include "core/profiler.h"

#include <iostream>
#include <mutex>
#include <regex>
#include <stack>

#include <SDL.h>

CONVAR_INT_NAME( log_verbose_global, "ch.log.verbosity.base", 1, "Base Developer Logging Level for all Log Channels", 0 );

ELogColor                  gCurrentColor = ELogColor_Default;
log_channel_h_t            gLC_General   = INVALID_LOG_CHANNEL;
log_channel_h_t            gLC_Logging   = INVALID_LOG_CHANNEL;
static ch_string           gDefaultChannelName( "General", 7 );
static std::mutex          gLogMutex;

// apparently you could of added stuff to this before static initialization got to this, and then you lose some log channels as a result
// TODO: this can just be a simple c array + u8 size, only allocates at start up and that's it
ChVector< log_channel_t >& GetLogChannels()
{
	static ChVector< log_channel_t > gChannels;
	return gChannels;
}

// static std::vector< log_channel_t >             gChannels;
static std::vector< log_t >                    gLogHistory;

static std::vector< LogChannelShownCallbackF > gCallbacksChannelShown;

constexpr glm::vec4                            gVecTo255( 255, 255, 255, 255 );


/* Windows Specific Functions for console text colors.  */
#ifdef _WIN32
#include <consoleapi2.h>
#include <debugapi.h>


int g_log_color_win32[] = {
	FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE,                         // Default
	0,                                                                           // Black
	FOREGROUND_INTENSITY | FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE,  // White

	FOREGROUND_BLUE,                                      // Dark Blue
	FOREGROUND_GREEN,                                     // Dark Green
	FOREGROUND_GREEN | FOREGROUND_BLUE,                   // Dark Cyan
	FOREGROUND_RED,                                       // Dark Red
	FOREGROUND_RED | FOREGROUND_BLUE,                     // Dark Purple
	FOREGROUND_RED | FOREGROUND_GREEN,                    // Dark Yellow
	FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE,  // Dark Gray

	FOREGROUND_INTENSITY | FOREGROUND_BLUE,                     // Blue
	FOREGROUND_INTENSITY | FOREGROUND_GREEN,                    // Green
	FOREGROUND_INTENSITY | FOREGROUND_GREEN | FOREGROUND_BLUE,  // Cyan
	FOREGROUND_INTENSITY | FOREGROUND_RED,                      // Red
	FOREGROUND_INTENSITY | FOREGROUND_RED | FOREGROUND_BLUE,    // Purple
	FOREGROUND_INTENSITY | FOREGROUND_RED | FOREGROUND_GREEN,   // Yellow
	FOREGROUND_INTENSITY,                                       // Gray
};


constexpr int Win32GetColor( ELogColor color )
{
	return g_log_color_win32[ static_cast< int >( color ) ];
}


void Win32SetColor( ELogColor color )
{
	BOOL result = SetConsoleTextAttribute( sys_get_console_window(), Win32GetColor( color ) );

	if ( !result )
	{
		printf( "*** Failed to set console color: %s\n", sys_get_error() );
	}
}

#endif


constexpr const char* g_log_color_str[] = {
	"Default",
	"Black",
	"White",

	"Dark Blue",
	"Dark Green",
	"Dark Cyan",
	"Dark Red",
	"Dark Purple",
	"Dark Yellow",
	"Dark Gray",

	"Blue",
	"Green",
	"Cyan",
	"Red",
	"Purple",
	"Yellow",
	"Gray",
};


constexpr const char* g_log_color_ansi[] = {
	ANSI_CLR_DEFAULT,
	ANSI_CLR_BLACK,
	ANSI_CLR_WHITE,

	ANSI_CLR_DARK_BLUE,
	ANSI_CLR_DARK_GREEN,
	ANSI_CLR_DARK_CYAN,
	ANSI_CLR_DARK_RED,
	ANSI_CLR_DARK_PURPLE,
	ANSI_CLR_DARK_YELLOW,
	ANSI_CLR_DARK_GRAY,

	ANSI_CLR_BLUE,
	ANSI_CLR_GREEN,
	ANSI_CLR_CYAN,
	ANSI_CLR_RED,
	ANSI_CLR_PURPLE,
	ANSI_CLR_YELLOW,
	ANSI_CLR_GRAY,
};


glm::vec4 g_log_color_rgba[] = {
	{ 1, 1, 1, 1 },        // Default
	{ 0.3, 0.3, 0.3, 1 },  // Black
	{ 1, 1, 1, 1 },        // White

	{ 0, 0.3, 0.8, 1 },       // Dark Blue
	{ 0.25, 0.57, 0.25, 1 },  // Dark Green
	{ 0, 0.35, 0.75, 1 },     // Dark Cyan
	{ 0.7, 0, 0.25, 1 },      // Dark Red
	{ 0.45, 0, 0.7, 1 },      // Dark Purple
	{ 0.6, 0.6, 0, 1 },       // Dark Yellow
	{ 0.45, 0.45, 0.45, 1 },  // Dark Gray

	{ 0, 0.4, 1, 1 },      // Blue
	{ 0.4, 0.9, 0.4, 1 },  // Green
	{ 0, 0.85, 1, 1 },     // Cyan
	{ 0.9, 0, 0.4, 1 },    // Red
	{ 0.6, 0, 0.9, 1 },    // Purple
	{ 1, 1, 0, 1 },        // Yellow
	{ 0.7, 0.7, 0.7, 1 },  // Gray
};


static_assert( ( CH_ARR_SIZE( g_log_color_str ) ) == (size_t)ELogColor_Count );
static_assert( ( CH_ARR_SIZE( g_log_color_ansi ) ) == (size_t)ELogColor_Count );
static_assert( ( CH_ARR_SIZE( g_log_color_rgba ) ) == (size_t)ELogColor_Count );


const char* Log_ColorToStr( ELogColor color )
{
	return g_log_color_str[ static_cast< int >( color ) ];
}


const char* Log_ColorToUnix( ELogColor color )
{
	return g_log_color_ansi[ static_cast< int >( color ) ];
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
		printf( " *** Log_UnixToColor: Unknown Color Type: \"%s\"\n", spColor );
		return ELogColor_Default;
	}

	bool isLight       = spColor[ 2 ] == '1';
	char colorStr[ 3 ] = { spColor[ 4 ], spColor[ 5 ], '\0' };

	long color         = 0;
	if ( !ch_to_long( colorStr, color ) )
	{
		printf( " *** Failed get color code from string: %s (%s)\n", spColor, colorStr );
		return ELogColor_Default;
	}

	return Log_UnixCodeToColor( isLight, color );
}


void UnixSetColor( ELogColor color )
{
	fputs( Log_ColorToUnix( color ), stdout );
}


// copied from graphics/gui/consoleui.cpp
constexpr glm::vec4 GetColorRGBA( ELogColor col )
{
	return g_log_color_rgba[ static_cast< int >( std::max( (u8)col, (u8)ELogColor_Count ) ) ];
}


constexpr ELogColor GetColor( log_channel_t* channel, ELogType sType )
{
	switch ( sType )
	{
		default:
		case ELogType_Normal:
		case ELogType_Verbose:
		case ELogType_Verbose2:
		case ELogType_Verbose3:
		case ELogType_Verbose4:
		case ELogType_Raw:
			return channel->color;

		case ELogType_Warning:
			return LOG_COLOR_WARNING;

		case ELogType_Error:
		case ELogType_Fatal:
			return LOG_COLOR_ERROR;
	}
}


constexpr glm::vec4 GetColorRGBA( log_channel_t* channel, const log_t& log )
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


u32 GetColorU32( log_channel_t* channel, const log_t& log )
{
	return GetColorU32( GetColor( channel, log.aType ) );
}


void Log_Init()
{
	gLC_General = Log_RegisterChannel( "General", ELogColor_Default );
	gLC_Logging = Log_RegisterChannel( "Logging", ELogColor_Default );

	// TODO: maybe add a launch option to change the verbosity of a log channel?
	// However that would be tricky since only the default channels exist right now, nothing else

	int verbose_level = std::clamp( args_register( 1, "Set logging system verbosity, Ranges from 0 to 4", "--verbosity" ), 0, 4 );

	Con_SetConVarValue( "ch.log.verbosity.base", verbose_level );
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


log_channel_h_t Log_RegisterChannel( const char *sName, ELogColor sColor )
{
	ch_string name;
	name.data = (char*)sName;
	name.size = strlen( sName );

	if ( name.size == 0 )
	{
		printf( " *** LogSystem: Invalid Channel Name: \"%s\"\n", sName );
		return INVALID_LOG_CHANNEL;
	}

	for ( size_t i = 0; i < GetLogChannels().size(); i++ )
    {
		log_channel_t* channel = &GetLogChannels()[ i ];
        if ( !ch_str_equals( channel->name, name ) )
            continue;
            
        return i;
    }

    log_channel_t& channel = GetLogChannels().emplace_back();
	channel.name            = name;
	channel.shown           = true;
	channel.color           = sColor;

	return (log_channel_h_t)GetLogChannels().size() - 1U;
}


log_channel_h_t Log_GetChannel( const char* sChannel )
{
	for ( int i = 0; i < GetLogChannels().size(); i++ )
	{
		log_channel_t* channel = &GetLogChannels()[ i ];
		if ( !ch_str_equals( channel->name, sChannel ) )
			continue;

		return (log_channel_h_t)i;
	}

	return INVALID_LOG_CHANNEL;
}


log_channel_t* Log_GetChannelData( log_channel_h_t sChannel )
{
	if ( sChannel >= GetLogChannels().size() )
        return nullptr;

    return &GetLogChannels()[ sChannel ];
}


log_channel_t* Log_GetChannelByName( const char* sChannel )
{
	for ( int i = 0; i < GetLogChannels().size(); i++ )
    {
		log_channel_t* channel = &GetLogChannels()[ i ];
		if ( !ch_str_equals( channel->name, sChannel ) )
            continue;

        return channel;
    }

    return nullptr;
}


static bool CheckConcatRealloc( ch_string& newOutput, ch_string& output )
{
	if ( !newOutput.data )
	{
		print( " *** LogSystem: Failed to Concatenate Strings during realloc!\n" );
		return false;
	}

	output = newOutput;
	return true;
}


static bool Log_FormatVerbose( log_channel_t* channel, ch_string& output, const char* devLevel, const char* last, size_t dist )
{
	const char* strings[] = { "[", channel->name.data, "] [V", devLevel, "] ", last };
	const u64   lengths[] = { 1,   channel->name.size, 4,         1,        2,    dist };
	ch_string   newOutput = ch_str_concat( CH_STR_UR( output ), 6, strings, lengths );

	return CheckConcatRealloc( newOutput, output );
}


static bool Log_FormatVerboseColor( log_channel_t* channel, ch_string& output, ch_string& color, const char* devLevel, const char* last, size_t dist )
{
	const char* strings[] = { color.data, "[", channel->name.data, "] [V", devLevel, "] ", last };
	const u64   lengths[] = { color.size, 1,   channel->name.size, 4,         1,        2,    dist };
	ch_string   newOutput = ch_str_concat( CH_STR_UR( output ), 7, strings, lengths );

	return CheckConcatRealloc( newOutput, output );
}


// Format for system console output
static ch_string FormatLog( log_channel_t* channel, ELogType sType, const char* spMessage, s64 len )
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
				const char* strings[] = { color.data, "[", channel->name.data, "] ", last };
				const u64   lengths[] = { color.size, 1, channel->name.size, 2, dist };
				ch_string   newOutput = ch_str_concat( CH_STR_UR( output ), 5, strings, lengths );

				CheckConcatRealloc( newOutput, output );
				break;
			}

			case ELogType_Verbose:
				Log_FormatVerboseColor( channel, output, color, "1", last, dist );
				break;

			case ELogType_Verbose2:
				Log_FormatVerboseColor( channel, output, color, "2", last, dist );
				break;

			case ELogType_Verbose3:
				Log_FormatVerboseColor( channel, output, color, "3", last, dist );
				break;

			case ELogType_Verbose4:
				Log_FormatVerboseColor( channel, output, color, "4", last, dist );
				break;

			case ELogType_Warning:
			{
				const char* strings[] = { color.data, "[", channel->name.data, "] [WARNING] ", last };
				const u64   lengths[] = { color.size, 1, channel->name.size, 12, dist };
				ch_string   newOutput = ch_str_concat( CH_STR_UR( output ), 5, strings, lengths );

				CheckConcatRealloc( newOutput, output );
				break;
			}

			case ELogType_Error:
			{
				const char* strings[] = { color.data, "[", channel->name.data, "] [ERROR] ", last };
				const u64   lengths[] = { color.size, 1, channel->name.size, 10, dist };
				ch_string   newOutput = ch_str_concat( CH_STR_UR( output ), 5, strings, lengths );

				CheckConcatRealloc( newOutput, output );
				break;
			}

			case ELogType_Fatal:
			{
				const char* strings[] = { color.data, "[", channel->name.data, "] [FATAL] ", last };
				const u64   lengths[] = { color.size, 1, channel->name.size, 10, dist };
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
			print( " *** LogSystem: Failed to Concatenate Strings in Log_StripColors!\n" );
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
				print( " *** LogSystem: Failed to Concatenate Strings in Log_StripColors!\n" );
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
				print( " *** LogSystem: Failed to Concatenate Strings in Log_StripColors!\n" );
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
static ch_string FormatLogNoColors( const log_t& srLog )
{
	PROF_SCOPE();

	// Split by New Line characters
	ch_string     output;

	log_channel_t* channel = Log_GetChannelData( srLog.channel );
	if ( !channel )
	{
		printf( "\n *** LogSystem: Channel Not Found for message: \"%s\"\n", srLog.aMessage.data );
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
				const char* strings[] = { "[", channel->name.data, "] ", last };
				const u64   lengths[] = { 1, channel->name.size, 2, dist };
				ch_string   newOutput = ch_str_concat( CH_STR_UR( output ), 4, strings, lengths );

				CheckConcatRealloc( newOutput, output );
				break;
			}

			case ELogType_Verbose:
				Log_FormatVerbose( channel, output, "1", last, dist );
				break;

			case ELogType_Verbose2:
				Log_FormatVerbose( channel, output, "2", last, dist );
				break;

			case ELogType_Verbose3:
				Log_FormatVerbose( channel, output, "3", last, dist );
				break;

			case ELogType_Verbose4:
				Log_FormatVerbose( channel, output, "4", last, dist );
				break;

			case ELogType_Raw:
			{
				ch_string newOutput = ch_str_concat( CH_STR_UR( output ), last, dist );
				CheckConcatRealloc( newOutput, output );
				break;
			}

			case ELogType_Warning:
			{
				const char* strings[] = { "[", channel->name.data, "] [WARNING] ", last };
				const u64   lengths[] = { 1, channel->name.size, 12, dist };
				ch_string   newOutput = ch_str_concat( CH_STR_UR( output ), 4, strings, lengths );

				CheckConcatRealloc( newOutput, output );
				break;
			}

			case ELogType_Error:
			{
				const char* strings[] = { "[", channel->name.data, "] [ERROR] ", last };
				const u64   lengths[] = { 1, channel->name.size, 10, dist };
				ch_string   newOutput = ch_str_concat( CH_STR_UR( output ), 4, strings, lengths );

				CheckConcatRealloc( newOutput, output );
				break;
			}

			case ELogType_Fatal:
			{
				const char* strings[] = { "[", channel->name.data, "] [FATAL] ", last };
				const u64   lengths[] = { 1, channel->name.size, 10, dist };
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


// Is this ELogType one of the developer levels?
inline bool Log_IsDevType( ELogType sType )
{
	return ( sType == ELogType_Verbose ||
	         sType == ELogType_Verbose2 ||
	         sType == ELogType_Verbose3 ||
	         sType == ELogType_Verbose4 );
}


inline bool Log_DevLevelVisible( const log_t& log )
{
	log_channel_t* channel = Log_GetChannelData( log.channel );

	if ( channel )
	{
		if ( channel->verbosity_level >= (int)log.aType && Log_IsDevType( log.aType ) )
			return true;
	}

	if ( log_verbose_global < (int)log.aType && Log_IsDevType( log.aType ) )
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
		colorBuf.color         = sMainColor;
		colorBuf.aLen           = sBuffer.size;
		colorBuf.apStr          = sBuffer.data;
		return;
	}

	// no color code at start of string
	if ( find != buf )
	{
		LogColorBuf_t& colorBuf = srColorList.emplace_back();
		colorBuf.color         = sMainColor;
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
			colorBuf.color = Log_UnixToColor( find, colorLength );

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
void Log_SysPrint( ELogColor sMainColor, const log_t& srLog, FILE* spStream )
{
#ifdef _WIN32
	
	// on win32, we need to split up the string by colors
	ChVector< LogColorBuf_t > colorList;
	Log_SplitStringColors( sMainColor, srLog.aFormatted, colorList );

	for ( LogColorBuf_t& colorBuffer : colorList )
	{
		Log_SetColor( colorBuffer.color );
		fprintf( spStream, "%*.*s", colorBuffer.aLen, colorBuffer.aLen, colorBuffer.apStr );
	}

	Log_SetColor( ELogColor_Default );
	fflush( spStream );

  #if !TRACY_ENABLE
	if ( !IsDebuggerPresent() )
		return;
  #endif

	ch_string_auto debug_string = FormatLogNoColors( srLog );
	OutputDebugStringA( debug_string.data );

	if ( Log_TracyEnabled() )
		Log_Tracy( sMainColor, CH_STR_UR( debug_string ) );
#else
	Log_SetColor( sMainColor );
	fputs( srLog.aFormatted.data, spStream );
	Log_SetColor( ELogColor_Default );
	fflush( spStream );

	if ( Log_TracyEnabled() )
	{
		ch_string_auto debug_string = FormatLogNoColors( srLog );
		Log_Tracy( sMainColor, CH_STR_UR( debug_string ) );
	}
#endif
}


CONCMD_NAME( log_test_colors, "ch.log.test_colors" )
{
	Log_Msg( ANSI_CLR_DARK_GREEN "TEST " ANSI_CLR_CYAN "TEST CYAN " ANSI_CLR_DARK_PURPLE ANSI_CLR_DARK_BLUE "TEST DARK BLUE \n" );
}


void Log_AddLogInternal( log_t& log )
{
    PROF_SCOPE();

    log_channel_t* channel = Log_GetChannelData( log.channel );
    if ( !channel )
    {
        printf( "\n *** LogSystem: Channel Not Found for message: \"%s\"\n", log.aMessage.data );
        return;
    }

    log.aFormatted = FormatLog( channel, log.aType, CH_STR_UR( log.aMessage ) );

    if ( channel->shown )
	{
        switch ( log.aType )
        {
            default:
            case ELogType_Normal:
            case ELogType_Raw:
				Log_SysPrint( channel->color, log, stdout );
                break;

            case ELogType_Verbose:
            case ELogType_Verbose2:
            case ELogType_Verbose3:
            case ELogType_Verbose4:
				if ( !Log_DevLevelVisible( log ) )
                    break;

				Log_SysPrint( channel->color, log, stdout );
                break;

            case ELogType_Warning:
				Log_SysPrint( LOG_COLOR_WARNING, log, stdout );
                break;

            case ELogType_Error:
				Log_SysPrint( LOG_COLOR_ERROR, log, stderr );
                break;

            case ELogType_Fatal:
				Log_SysPrint( LOG_COLOR_ERROR, log, stderr );

				const char*    strings[]       = { "[", channel->name.data, "] Fatal Error" };
				const u64      lengths[]       = { 1, channel->name.size, 13 };
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


log_channel_h_t LogGetChannel( const char* name )
{
	if ( !name )
		return INVALID_LOG_CHANNEL;

	u64 nameLen = strlen( name );

	for ( size_t i = 0; const auto& channel : GetLogChannels() )
    {
        // if ( GetLogSystem().aChannels[i].name == name )
        if ( ch_str_equals( channel.name, name, nameLen ) )
            return (log_channel_h_t)i;

        i++;
    }

    return INVALID_LOG_CHANNEL;
}


ELogColor Log_GetChannelColor( log_channel_h_t handle )
{
	log_channel_t* channel = Log_GetChannelData( handle );
    if ( !channel )
        return ELogColor_Default;

    return channel->color;
}


ch_string Log_GetChannelName( log_channel_h_t handle )
{
	log_channel_t* channel = Log_GetChannelData( handle );
    if ( !channel )
        return gDefaultChannelName;

    return channel->name;
}


unsigned char Log_GetChannelCount()
{
	return (unsigned char)GetLogChannels().size();
}


const std::vector< log_t >& Log_GetLogHistory()
{
    return gLogHistory;
}


bool Log_ChannelIsShown( log_channel_h_t handle )
{
	log_channel_t* channel = Log_GetChannelData( handle );
	return channel && channel->shown;
}


int Log_GetChannelDevLevel( log_channel_h_t handle )
{
	log_channel_t* channel = Log_GetChannelData( handle );
	if ( !channel )
		return log_verbose_global;

	return std::max( (u8)log_verbose_global, channel->verbosity_level );
}


int Log_GetDevLevel()
{
	return log_verbose_global;
}


const log_t* Log_GetLastLog()
{
    if ( gLogHistory.size() == 0 )
        return nullptr;

    return &gLogHistory[ gLogHistory.size() - 1 ];
}


bool Log_IsVisible( const log_t& log )
{
	if ( !Log_DevLevelVisible( log ) )
        return false;

    return Log_ChannelIsShown( log.channel );
}


// ----------------------------------------------------------------
// log_t Group Functions


log_t Log_GroupBeginEx( log_channel_h_t channel, ELogType type )
{
	return {
		.channel = channel,
		.aType    = type,
	};
}


log_t Log_GroupBegin( log_channel_h_t channel )
{
	return {
		.channel = channel,
		.aType   = ELogType_Normal,
	};
}


log_t Log_GroupBegin()
{
	return {
		.channel = gLC_General,
		.aType   = ELogType_Normal,
	};
}


void Log_GroupEnd( log_t& sGroup )
{
	gLogMutex.lock();

	// Submit the built log
	gLogHistory.push_back( sGroup );
	Log_AddLogInternal( gLogHistory.back() );

	gLogMutex.unlock();
}


void Log_Group( log_t& sGroup, const char* spBuf )
{
	PROF_SCOPE();

	size_t bufLen = 0;
	if ( !ch_str_check_empty( spBuf, bufLen ) )
		return;

	ch_string append = ch_str_concat( CH_STR_UR( sGroup.aMessage ), spBuf, bufLen );
	CheckConcatRealloc( append, sGroup.aMessage );
}


void Log_GroupF( log_t& sGroup, const char* spFmt, ... )
{
	va_list args;
	va_start( args, spFmt );
	Log_GroupV( sGroup, spFmt, args );
	va_end( args );
}


void Log_GroupV( log_t& sGroup, const char* spFmt, va_list args )
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


bool Log_ShouldAddLog( log_channel_h_t sChannel, ELogType sLevel )
{
	// Is this a developer level?
	if ( !Log_IsDevType( sLevel ) )
		return true;

	// Is the global dev level less than the log's developer level?
	if ( log_verbose_global < static_cast< int >( sLevel ) )
	{
		// Check if the channel dev level is less than the log's developer level
		log_channel_t* channel = Log_GetChannelData( sChannel );
		if ( !channel )
		{
			Log_Error( "Unable to find channel when checking developer level\n" );
			return false;
		}

		// log_verbose_global is the base value for every channel
		if ( channel->verbosity_level <= log_verbose_global )
			return false;

		// Don't even save this log, it's probably flooding the log history
		// and slowing down perf with adding it, and the 2 vnsprintf calls
		// TODO: maybe we can have an convar option to save this anyway?
		if ( channel->verbosity_level < static_cast< int >( sLevel ) )
			return false;
	}

	return true;
}


void Log_Ex( log_channel_h_t sChannel, ELogType sLevel, const char* spBuf )
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
	log_t& log = gLogHistory[ gLogHistory.size() - 1 ];

	Log_AddLogInternal( log );

	gLogMutex.unlock();
}


void Log_ExF( log_channel_h_t sChannel, ELogType sLevel, const char* spFmt, ... )
{
	LOG_SYS_MSG_VA( spFmt, sChannel, sLevel );
}


void Log_ExV( log_channel_h_t sChannel, ELogType sLevel, const char* spFmt, va_list args )
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
	log_t&    log = gLogHistory[ gLogHistory.size() - 1 ];

	va_list copy;
	va_copy( copy, args );
	int len = std::vsnprintf( nullptr, 0, spFmt, copy );
	va_end( copy );

	if ( len < 0 )
	{
		print( "\n *** LogSystem: vsnprintf failed?\n\n" );
		gLogMutex.unlock();
		return;
	}

	log.aMessage = ch_str_copy_v( spFmt, args );

	if ( !log.aMessage.data )
	{
		print( "\n *** LogSystem: Failed to Allocate Memory for Log Message!\n\n" );
		gLogMutex.unlock();
		return;
	}

	Log_AddLogInternal( log );

	gLogMutex.unlock();
}


// ----------------------------------------------------------------
// Standard Logging Functions

// Lowest severity.
void CORE_API Log_Msg( log_channel_h_t channel, const char* spBuf )
{
	Log_Ex( channel, ELogType_Normal, spBuf );
}

void CORE_API Log_MsgF( log_channel_h_t channel, const char* spFmt, ... )
{
	LOG_SYS_MSG_VA( spFmt, channel, ELogType_Normal );
}

// Medium severity.
void CORE_API Log_Warn( log_channel_h_t channel, const char* spBuf )
{
	Log_Ex( channel, ELogType_Warning, spBuf );
}

void CORE_API Log_WarnF( log_channel_h_t channel, const char* spFmt, ... )
{
	LOG_SYS_MSG_VA( spFmt, channel, ELogType_Warning );
}

// High severity.
void CORE_API Log_Error( log_channel_h_t channel, const char* spBuf )
{
	Log_Ex( channel, ELogType_Error, spBuf );
}

void CORE_API Log_ErrorF( log_channel_h_t channel, const char* spFmt, ... )
{
	LOG_SYS_MSG_VA( spFmt, channel, ELogType_Error );
}

// Extreme severity.
void CORE_API Log_Fatal( log_channel_h_t channel, const char* spBuf )
{
	Log_Ex( channel, ELogType_Fatal, spBuf );
}

void CORE_API Log_FatalF( log_channel_h_t channel, const char* spFmt, ... )
{
	LOG_SYS_MSG_VA( spFmt, channel, ELogType_Fatal );
}

// Dev only.
void CORE_API Log_Dev( log_channel_h_t channel, u8 sLvl, const char* spBuf )
{
	if ( sLvl < 1 || sLvl > 4 )
		sLvl = 4;

	Log_Ex( channel, (ELogType)sLvl, spBuf );
}

void CORE_API Log_DevF( log_channel_h_t channel, u8 sLvl, const char* spFmt, ... )
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
// log_t ConCommands


void log_channel_dropdown(
  const std::vector< std::string >& args,         // arguments currently typed in by the user
  const std::string&                fullCommand,  // the full command line the user has typed in
  std::vector< std::string >&       results )     // results to populate the dropdown list with
{
	for ( const auto& channel : GetLogChannels() )
	{
		if ( args.size() && !( ch_str_starts_with( channel.name, args[ 0 ].data(), args[ 0 ].size() ) ) )
			continue;

		results.push_back( channel.name.data );
	}
}


CONCMD_NAME_DROP( log_channel_hide, "ch.log.channel.hide", log_channel_dropdown )
{
    if ( args.size() == 0 )
        return;

    log_channel_t* channel = Log_GetChannelByName( args[0].c_str() );
    if ( !channel )
        return;

    channel->shown = false;

	Log_RunCallbacksChannelShown();
}


CONCMD_NAME_DROP( log_channel_show, "ch.log.channel.show", log_channel_dropdown )
{
    if ( args.size() == 0 )
        return;

    log_channel_t* channel = Log_GetChannelByName( args[ 0 ].c_str() );
    if ( !channel )
        return;

    channel->shown = true;

	Log_RunCallbacksChannelShown();
}


constexpr const char* LOG_CHANNEL_DUMP_HEADER     = "Channel Name%*s  | Shown  | Developer Level | Color\n";
constexpr size_t      LOG_CHANNEL_DUMP_HEADER_LEN = 53;


// TODO: this is not very good, clean this up, adding developer levels broke it and i haven't fixed it yet
CONCMD_NAME( log_channel_dump, "ch.log.dump.channels" )
{
    // Calculate max name length
	size_t           maxNameLength = 0;
	size_t           maxLength     = 0;
	constexpr size_t logNameLen = 13;

    for ( const auto& channel : GetLogChannels() )
	{
		maxNameLength = std::max( logNameLen, std::max( maxNameLength, channel.name.size ) );
		// maxLength = std::max( maxLength, maxNameLength + 23 );
		maxLength = std::max( maxLength, maxNameLength + ( LOG_CHANNEL_DUMP_HEADER_LEN - logNameLen ) );
	}

	log_t group = Log_GroupBegin( gLC_Logging );

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

    // Display log_t Channels
    for ( const auto& channel : GetLogChannels() )
	{
		Log_GroupF(
		  group,
		  "%s%s%*s | %s | %d | %s\n",
		  Log_ColorToUnix( channel.color ),
		  channel.name.data,
		  maxNameLength - channel.name.size,
		  "",
		  channel.shown ? "Shown " : "Hidden",
		  channel.verbosity_level,
		  Log_ColorToStr( channel.color ) );
    }

	Log_GroupEnd( group );
}


CONCMD_NAME( log_color_dump, "ch.log.dump.colors" )
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


CONCMD_NAME_VA( log_dump, "ch.log.dump", "Dump Logging History to file" )
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


static void log_verbose_dropdown(
  const std::vector< std::string >& args,         // arguments currently typed in by the user
  const std::string&                fullCommand,  // the full command line the user has typed in
  std::vector< std::string >&       results )     // results to populate the dropdown list with
{
	for ( const log_channel_t& channel : GetLogChannels() )
	{
		// if ( args.size() && !channel.name.starts_with( args[ 0 ] ) )
		if ( args.size() )
		{
			if ( args[ 0 ].size() > channel.name.size )
				continue;

			if ( ch_strncasecmp( channel.name.data, args[ 0 ].data(), args[ 0 ].size() ) != 0 )
				continue;
		}

		std::string value = vstring( "%s %d", channel.name.data, channel.verbosity_level );
		results.push_back( value );
	}
}


CONCMD_NAME_DROP_VA( ch_log_verbose, "ch.log.verbosity", log_verbose_dropdown, 0, "Change Log Developer Level of a Channel" )
{
	if ( !args.size() )
	{
		Log_Msg( ch_log_verbose_cmd.GetPrintMessage().c_str() );
		return;
	}

	log_channel_t* channel = Log_GetChannelByName( args[ 0 ].c_str() );

	if ( !channel )
	{
		Log_ErrorF( "Log Channel Not Found: %s\n", args[ 0 ].c_str() );
		return;
	}

	if ( args.size() > 1 )
	{
		long out = 0;
		if ( !ch_to_long( args[ 1 ].data(), out ) )
		{
			Log_ErrorF( "Failed to convert requested verbosity to int: %s\n", args[ 1 ].c_str() );
		}
		else
		{
			channel->verbosity_level = std::clamp( out, 0L, 4L );
			Log_MsgF( "Set Verbosity Level of Log Channel \"%s\" to %d\n", channel->name.data, channel->verbosity_level );
		}
	}
	else
	{
		Log_MsgF( "Log Channel \"%s\" - Verbosity Level: %d\n", channel->name.data, channel->verbosity_level );
	}
}

