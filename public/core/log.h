#pragma once

#include "platform.h"
#include "util.h"

#include <functional>

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
	ELogType_Normal = 0,
	ELogType_Dev,
	ELogType_Dev2,
	ELogType_Dev3,
	ELogType_Dev4,
	ELogType_Raw,      // "%s" - kind of a hack for quick new line printing
	ELogType_Warning,  // "[Channel] [WARNING] "%s"
	ELogType_Error,    // "[Channel] [ERROR] %s"
	ELogType_Fatal     // "[Channel] [FATAL] %s"
};


struct LogChannelDef_t
{
	ch_string aName;
	bool      aShown;
	ELogColor  aColor;
	int       aDevLevel;  // TODO: change to verbosity_level;
};


using LogChannel = unsigned char;


struct Log
{
	LogChannel aChannel;
	ELogType    aType;
	ch_string  aMessage;
	ch_string  aFormatted;
};


using LogGroup = Log&;


struct LogColorBuf_t
{
	ELogColor    aColor;
	const char* apStr;
	u32         aLen;
};


/*ReBuildConsoleOutput*/
typedef void           ( *LogChannelShownCallbackF )();


constexpr ELogColor    LOG_COLOR_WARNING   = ELogColor_Yellow;
constexpr ELogColor    LOG_COLOR_ERROR     = ELogColor_Red;
constexpr LogChannel   INVALID_LOG_CHANNEL = 255;

// ----------------------------------------------------------------
// Logging API

CORE_API void          Log_Init();
CORE_API void          Log_Shutdown();

// Manage Console Color
CORE_API void          Log_SetColor( ELogColor color );
CORE_API ELogColor     Log_GetColor();
CORE_API const char*   Log_ColorToStr( ELogColor color );

// Split a string by unix colors into a vector of LogColorBuf_t
// does no new memory allocations, only stores color, a starting char pointer, and a length
CORE_API void          Log_SplitStringColors( ELogColor sMainColor, const ch_string& sBuffer, ChVector< LogColorBuf_t >& srColorList, bool sNoColors = false );

CORE_API const char*   Log_ColorToUnix( ELogColor color );
CORE_API ch_string     Log_ColorToUnixStr( ELogColor color );
CORE_API ELogColor     Log_UnixCodeToColor( bool sIsLight, int sColor );
CORE_API ELogColor     Log_UnixToColor( const char* spColor, size_t sLen );

// Manage Log Channels
CORE_API LogChannel    Log_RegisterChannel( const char* sName, ELogColor sColor = ELogColor_Default );
CORE_API LogChannel    Log_GetChannel( const char* sName );
CORE_API ELogColor     Log_GetChannelColor( LogChannel handle );
CORE_API ch_string     Log_GetChannelName( LogChannel handle );
CORE_API bool          Log_ChannelIsShown( LogChannel handle );
CORE_API int           Log_GetChannelDevLevel( LogChannel handle );
CORE_API unsigned char Log_GetChannelCount();

// Log Information
CORE_API ch_string     Log_BuildHistoryString( int sMaxSize = -1 );
CORE_API const std::vector< Log >& Log_GetLogHistory();
CORE_API const Log*                Log_GetLastLog();
CORE_API bool                      Log_IsVisible( const Log& log );
CORE_API int                       Log_GetDevLevel();

CORE_API void                      Log_AddChannelShownCallback( LogChannelShownCallbackF callback );

// ----------------------------------------------------------------
// System printing, skip logging

CORE_API void                      ch_printf( const char* str, ... );
CORE_API void                      ch_print( const char* str );

// ----------------------------------------------------------------
// Log Group Functions

// Returns a ch_handle_t to the current log group
CORE_API LogGroup                  Log_GroupBeginEx( LogChannel channel, ELogType sType );
CORE_API LogGroup                  Log_GroupBegin( LogChannel channel );
CORE_API LogGroup                  Log_GroupBegin();

CORE_API void                      Log_GroupEnd( LogGroup sGroup );

CORE_API void                      Log_Group( LogGroup sGroup, const char* spBuf );
CORE_API void                      Log_GroupF( LogGroup sGroup, const char* spFmt, ... );
CORE_API void                      Log_GroupV( LogGroup sGroup, const char* spFmt, va_list args );

// ----------------------------------------------------------------
// Extended Logging Functions

CORE_API void                      Log_Ex( LogChannel channel, ELogType sLevel, const char* spBuf );
CORE_API void                      Log_ExF( LogChannel channel, ELogType sLevel, const char* spFmt, ... );
CORE_API void                      Log_ExV( LogChannel channel, ELogType sLevel, const char* spFmt, va_list args );

// ----------------------------------------------------------------
// Standard Logging Functions
// TODO: change to ch_log_msg, ch_log_warn, etc.?

// Lowest severity.
CORE_API void                      Log_Msg( LogChannel channel, const char* spBuf );
CORE_API void                      Log_MsgF( LogChannel channel, const char* spFmt, ... );

// Medium severity.
CORE_API void                      Log_Warn( LogChannel channel, const char* spBuf );
CORE_API void                      Log_WarnF( LogChannel channel, const char* spFmt, ... );

// High severity.
CORE_API void                      Log_Error( LogChannel channel, const char* spBuf );
CORE_API void                      Log_ErrorF( LogChannel channel, const char* spFmt, ... );

// Extreme severity.
CORE_API void                      Log_Fatal( LogChannel channel, const char* spBuf );
CORE_API void                      Log_FatalF( LogChannel channel, const char* spFmt, ... );

// Dev only.
CORE_API void                      Log_Dev( LogChannel channel, u8 sLvl, const char* spBuf );
CORE_API void                      Log_DevF( LogChannel channel, u8 sLvl, const char* spFmt, ... );

// ----------------------------------------------------------------
// Default to general channel.

// Lowest severity.
CORE_API void                      Log_Msg( const char* spFmt );
CORE_API void                      Log_MsgF( const char* spFmt, ... );

// Medium severity.
CORE_API void                      Log_Warn( const char* spFmt );
CORE_API void                      Log_WarnF( const char* spFmt, ... );

// High severity.
CORE_API void                      Log_Error( const char* spFmt );
CORE_API void                      Log_ErrorF( const char* spFmt, ... );

// Extreme severity.
CORE_API void                      Log_Fatal( const char* spFmt );
CORE_API void                      Log_FatalF( const char* spFmt, ... );

// Dev only.
CORE_API void                      Log_Dev( u8 sLvl, const char* spFmt );
CORE_API void                      Log_DevF( u8 sLvl, const char* spFmt, ... );

// ----------------------------------------------------------------
// Helper Macros

// register log channel with a custom name
#define LOG_CHANNEL_REGISTER_EX( var, name, ... ) LogChannel var = Log_RegisterChannel( name, __VA_ARGS__ );

// register a log channel prefixed with gLC_, for global log channel
#define LOG_CHANNEL_REGISTER( name, ... )         LogChannel gLC_##name = Log_RegisterChannel( #name, __VA_ARGS__ );

// extern log channel
#define LOG_CHANNEL( name )                       extern LogChannel gLC_##name;
