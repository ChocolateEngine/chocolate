#pragma once

#include "platform.h"
#include "core/util.h"

template< typename T >
struct ChVector;

using ELogColor = u8;
enum ELogColor_ : ELogColor
{
	ELogColor_Default,

	ELogColor_Black,
	ELogColor_White,

	ELogColor_DarkBlue,
	ELogColor_DarkGreen,
	ELogColor_DarkCyan,
	ELogColor_DarkRed,
	ELogColor_DarkPurple,
	ELogColor_DarkYellow,
	ELogColor_DarkGray,

	ELogColor_Blue,
	ELogColor_Green,
	ELogColor_Cyan,
	ELogColor_Red,
	ELogColor_Purple,
	ELogColor_Yellow,
	ELogColor_Gray,

	ELogColor_Max = ELogColor_Gray,
	ELogColor_Count,
};


// ANSI Color Macros
#define ANSI_CLR_DEFAULT     "\033[0m"
#define ANSI_CLR_BLACK       "\033[0;30m"
#define ANSI_CLR_WHITE       "\033[1;37m"

#define ANSI_CLR_DARK_BLUE   "\033[0;34m"
#define ANSI_CLR_DARK_GREEN  "\033[0;32m"
#define ANSI_CLR_DARK_CYAN   "\033[0;36m"
#define ANSI_CLR_DARK_RED    "\033[0;31m"
#define ANSI_CLR_DARK_PURPLE "\033[0;35m"
#define ANSI_CLR_DARK_YELLOW "\033[0;33m"
#define ANSI_CLR_DARK_GRAY   "\033[0;30m"

#define ANSI_CLR_BLUE        "\033[1;34m"
#define ANSI_CLR_GREEN       "\033[1;32m"
#define ANSI_CLR_CYAN        "\033[1;36m"
#define ANSI_CLR_RED         "\033[1;31m"
#define ANSI_CLR_PURPLE      "\033[1;35m"
#define ANSI_CLR_YELLOW      "\033[1;33m"
#define ANSI_CLR_GRAY        "\033[1;30m"


using ELogType = u8;
enum ELogType_: ELogType
{
	ELogType_Normal = 0,  // [Channel] %s
	ELogType_Verbose,     // [Channel] [V1] %s
	ELogType_Verbose2,    // [Channel] [V2] %s
	ELogType_Verbose3,    // [Channel] [V3] %s
	ELogType_Verbose4,    // [Channel] [V4] %s
	ELogType_Raw,         // %s
	ELogType_Warning,     // [Channel] [WARNING] %s
	ELogType_Error,       // [Channel] [ERROR] %s
	ELogType_Fatal,       // [Channel] [FATAL] %s
	ELogType_Count
};


struct log_channel_t
{
	ch_string name;
	bool      shown;
	ELogColor color;
	u8        verbosity_level;
};


// log channel handle - max of 255 log channels
using log_channel_h_t = u8;


struct log_t
{
	log_channel_h_t channel;
	ELogType        aType;
	ch_string       aMessage;
	ch_string       aFormatted;
};


struct LogColorBuf_t
{
	ELogColor   color;
	const char* apStr;
	u32         aLen;
};


/*ReBuildConsoleOutput*/
typedef void           ( *LogChannelShownCallbackF )();


constexpr ELogColor    LOG_COLOR_WARNING   = ELogColor_Yellow;
constexpr ELogColor    LOG_COLOR_ERROR     = ELogColor_Red;
constexpr log_channel_h_t   INVALID_LOG_CHANNEL = 255;

// ----------------------------------------------------------------
// Logging API

CORE_API void               Log_Init();
CORE_API void               Log_Shutdown();

// Manage Console Color
CORE_API void               Log_SetColor( ELogColor color );
CORE_API ELogColor          Log_GetColor();
CORE_API const char*        Log_ColorToStr( ELogColor color );

// Split a string by unix colors into a vector of LogColorBuf_t
// does no new memory allocations, only stores color, a starting char pointer, and a length
CORE_API void               Log_SplitStringColors( ELogColor sMainColor, const ch_string& sBuffer, ChVector< LogColorBuf_t >& srColorList, bool sNoColors = false );

CORE_API const char*        Log_ColorToUnix( ELogColor color );
CORE_API ch_string          Log_ColorToUnixStr( ELogColor color );
CORE_API ELogColor          Log_UnixCodeToColor( bool sIsLight, int sColor );
CORE_API ELogColor          Log_UnixToColor( const char* spColor, size_t sLen );

// Manage log_t Channels
CORE_API log_channel_h_t    Log_RegisterChannel( const char* sName, ELogColor sColor = ELogColor_Default );
CORE_API log_channel_h_t    Log_GetChannel( const char* sName );
CORE_API ELogColor          Log_GetChannelColor( log_channel_h_t handle );
CORE_API ch_string          Log_GetChannelName( log_channel_h_t handle );
CORE_API bool               Log_ChannelIsShown( log_channel_h_t handle );
CORE_API int                Log_GetChannelDevLevel( log_channel_h_t handle );
CORE_API unsigned char      Log_GetChannelCount();

// log_t Information
CORE_API ch_string          Log_BuildHistoryString( int sMaxSize = -1 );
CORE_API const std::vector< log_t >& Log_GetLogHistory();
CORE_API const log_t*                Log_GetLastLog();
CORE_API bool                        Log_IsVisible( const log_t& log );
CORE_API int                         Log_GetDevLevel();

CORE_API void                        Log_AddChannelShownCallback( LogChannelShownCallbackF callback );

// ----------------------------------------------------------------
// log_t Group Functions

// Returns a ch_handle_t to the current log group
CORE_API log_t                       Log_GroupBeginEx( log_channel_h_t channel, ELogType sType );
CORE_API log_t                       Log_GroupBegin( log_channel_h_t channel );
CORE_API log_t                       Log_GroupBegin();

CORE_API void                        Log_GroupEnd( log_t& sGroup );

// change to Log_Build
CORE_API void                        Log_Group( log_t& sGroup, const char* spBuf );
CORE_API void                        Log_GroupF( log_t& sGroup, const char* spFmt, ... );
CORE_API void                        Log_GroupV( log_t& sGroup, const char* spFmt, va_list args );

// ----------------------------------------------------------------
// Extended Logging Functions

CORE_API void                        Log_Ex( log_channel_h_t channel, ELogType sLevel, const char* spBuf );
CORE_API void                        Log_ExF( log_channel_h_t channel, ELogType sLevel, const char* spFmt, ... );
CORE_API void                        Log_ExV( log_channel_h_t channel, ELogType sLevel, const char* spFmt, va_list args );

// ----------------------------------------------------------------
// Standard Logging Functions
// TODO: change to ch_log_msg, ch_log_warn, etc.?

// Lowest severity.
CORE_API void                        Log_Msg( log_channel_h_t channel, const char* spBuf );
CORE_API void                        Log_MsgF( log_channel_h_t channel, const char* spFmt, ... );

// Medium severity.
CORE_API void                        Log_Warn( log_channel_h_t channel, const char* spBuf );
CORE_API void                        Log_WarnF( log_channel_h_t channel, const char* spFmt, ... );

// High severity.
CORE_API void                        Log_Error( log_channel_h_t channel, const char* spBuf );
CORE_API void                        Log_ErrorF( log_channel_h_t channel, const char* spFmt, ... );

// Extreme severity.
CORE_API void                        Log_Fatal( log_channel_h_t channel, const char* spBuf );
CORE_API void                        Log_FatalF( log_channel_h_t channel, const char* spFmt, ... );

// Dev only.
CORE_API void                        Log_Dev( log_channel_h_t channel, u8 sLvl, const char* spBuf );
CORE_API void                        Log_DevF( log_channel_h_t channel, u8 sLvl, const char* spFmt, ... );

// ----------------------------------------------------------------
// Default to general channel.

// Lowest severity.
CORE_API void                        Log_Msg( const char* spFmt );
CORE_API void                        Log_MsgF( const char* spFmt, ... );

// Medium severity.
CORE_API void                        Log_Warn( const char* spFmt );
CORE_API void                        Log_WarnF( const char* spFmt, ... );

// High severity.
CORE_API void                        Log_Error( const char* spFmt );
CORE_API void                        Log_ErrorF( const char* spFmt, ... );

// Extreme severity.
CORE_API void                        Log_Fatal( const char* spFmt );
CORE_API void                        Log_FatalF( const char* spFmt, ... );

// Dev only.
CORE_API void                        Log_Dev( u8 sLvl, const char* spFmt );
CORE_API void                        Log_DevF( u8 sLvl, const char* spFmt, ... );


// ----------------------------------------------------------------
// Helper Macros

// register log channel with a custom name
#define LOG_CHANNEL_REGISTER_EX( var, name, ... ) log_channel_h_t var = Log_RegisterChannel( name, __VA_ARGS__ );

// register a log channel prefixed with gLC_, for global log channel
#define LOG_CHANNEL_REGISTER( name, ... )         log_channel_h_t gLC_##name = Log_RegisterChannel( #name, __VA_ARGS__ );

// extern log channel
#define LOG_CHANNEL( name )                       extern log_channel_h_t gLC_##name;
