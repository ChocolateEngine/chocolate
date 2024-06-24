#pragma once

#include "platform.h"
#include "util.h"

#include <functional>

template< typename T >
struct ChVector;

enum class LogColor: unsigned char
{
	Default,

	Black,
	White,

	DarkBlue,
	DarkGreen,
	DarkCyan,
	DarkRed,
	DarkPurple,
	DarkYellow,
	DarkGray,

	Blue,
	Green,
	Cyan,
	Red,
	Purple,
	Yellow,
	Gray,

	Max = Gray,
	Count,
};


// Unix Color Macros (TODO: REMOVE THIS AND RENAME TO ANSI) 
#define UNIX_CLR_DEFAULT     "\033[0m"
#define UNIX_CLR_BLACK       "\033[0;30m"
#define UNIX_CLR_WHITE       "\033[1;37m"

#define UNIX_CLR_DARK_BLUE   "\033[0;34m"
#define UNIX_CLR_DARK_GREEN  "\033[0;32m"
#define UNIX_CLR_DARK_CYAN   "\033[0;36m"
#define UNIX_CLR_DARK_RED    "\033[0;31m"
#define UNIX_CLR_DARK_PURPLE "\033[0;35m"
#define UNIX_CLR_DARK_YELLOW "\033[0;33m"
#define UNIX_CLR_DARK_GRAY   "\033[0;30m"

#define UNIX_CLR_BLUE        "\033[1;34m"
#define UNIX_CLR_GREEN       "\033[1;32m"
#define UNIX_CLR_CYAN        "\033[1;36m"
#define UNIX_CLR_RED         "\033[1;31m"
#define UNIX_CLR_PURPLE      "\033[1;35m"
#define UNIX_CLR_YELLOW      "\033[1;33m"
#define UNIX_CLR_GRAY        "\033[1;30m"


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


// Unicode Variant for Windows
//#define UANSI_CLR_DEFAULT     USTR( "\033[0m" )
//#define UANSI_CLR_BLACK       USTR( "\033[0;30m" )
//#define UANSI_CLR_WHITE       USTR( "\033[1;37m" )
//
//#define UANSI_CLR_DARK_BLUE   USTR( "\033[0;34m" )
//#define UANSI_CLR_DARK_GREEN  USTR( "\033[0;32m" )
//#define UANSI_CLR_DARK_CYAN   USTR( "\033[0;36m" )
//#define UANSI_CLR_DARK_RED    USTR( "\033[0;31m" )
//#define UANSI_CLR_DARK_PURPLE USTR( "\033[0;35m" )
//#define UANSI_CLR_DARK_YELLOW USTR( "\033[0;33m" )
//#define UANSI_CLR_DARK_GRAY   USTR( "\033[0;30m" )
//
//#define UANSI_CLR_BLUE        USTR( "\033[1;34m" )
//#define UANSI_CLR_GREEN       USTR( "\033[1;32m" )
//#define UANSI_CLR_CYAN        USTR( "\033[1;36m" )
//#define UANSI_CLR_RED         USTR( "\033[1;31m" )
//#define UANSI_CLR_PURPLE      USTR( "\033[1;35m" )
//#define UANSI_CLR_YELLOW      USTR( "\033[1;33m" )
//#define UANSI_CLR_GRAY        USTR( "\033[1;30m" )


enum class LogType: unsigned char
{
	Normal = 0,
	Dev,
	Dev2,
	Dev3,
	Dev4,
	Input,    // "] %s" - user console input
	Raw,      // "%s" - kind of a hack for quick new line printing
	Warning,  // "[Channel] [WARNING] "%s"
	Error,    // "[Channel] [ERROR] %s"
	Fatal     // "[Channel] [FATAL] %s"
};


struct LogChannel_t
{
	std::string_view aName;
	bool             aShown;
	LogColor         aColor;
	int              aDevLevel;
};

using LogChannel = unsigned char;
using LogGroup   = unsigned int;


struct Log
{
	LogChannel  aChannel;
	LogType     aType;
	std::string aMessage;
	std::string aFormatted;
};


struct LogColorBuf_t
{
	LogColor    aColor;
	const char* apStr;
	u32         aLen;
};


/*ReBuildConsoleOutput*/
typedef void ( *LogChannelShownCallbackF )();


constexpr LogColor   LOG_COLOR_WARNING   = LogColor::Yellow;
constexpr LogColor   LOG_COLOR_ERROR     = LogColor::Red;
constexpr LogChannel INVALID_LOG_CHANNEL = 255;

// ----------------------------------------------------------------
// Logging API

void                     CORE_API  Log_Init();

// Manage Console Color
void                     CORE_API  Log_SetColor( LogColor color );
LogColor                 CORE_API  Log_GetColor();
const char               CORE_API *Log_ColorToStr( LogColor color );

// Split a string by unix colors into a vector of LogColorBuf_t
// does no new memory allocations, only stores color, a starting char pointer, and a length
void                     CORE_API  Log_SplitStringColors( LogColor sMainColor, std::string_view sBuffer, ChVector< LogColorBuf_t >& srColorList, bool sNoColors = false );

const char               CORE_API *Log_ColorToUnix( LogColor color );
LogColor                 CORE_API  Log_UnixCodeToColor( bool sIsLight, int sColor );
LogColor                 CORE_API  Log_UnixToColor( const char* spColor, size_t sLen );

// Manage Log Channels
CORE_API LogChannel                Log_RegisterChannel( const char* sName, LogColor sColor = LogColor::Default );
LogChannel               CORE_API  Log_GetChannel( const char *sName );
LogColor                 CORE_API  Log_GetChannelColor( LogChannel handle );
std::string_view         CORE_API  Log_GetChannelName( LogChannel handle );
bool                     CORE_API  Log_ChannelIsShown( LogChannel handle );
int                      CORE_API  Log_GetChannelDevLevel( LogChannel handle );
unsigned char            CORE_API  Log_GetChannelCount();

// Log Information
void                     CORE_API  Log_BuildHistoryString( std::string& srOutput, int sMaxSize = -1 );
const std::vector< Log > CORE_API &Log_GetLogHistory();
const Log                CORE_API *Log_GetLastLog();
bool                     CORE_API  Log_IsVisible( const Log& log );
int                      CORE_API  Log_GetDevLevel();

void                     CORE_API  Log_AddChannelShownCallback( LogChannelShownCallbackF callback );

// ----------------------------------------------------------------
// System printing, skip logging

void CORE_API                      PrintF( const char* str, ... );
void CORE_API                      Print( const char* str );

// ----------------------------------------------------------------
// Log Group Functions

// Returns a Handle to the current log group
LogGroup CORE_API                  Log_GroupBeginEx( LogChannel channel, LogType sType );
LogGroup CORE_API                  Log_GroupBegin( LogChannel channel );
LogGroup CORE_API                  Log_GroupBegin();

void CORE_API                      Log_GroupEnd( LogGroup sGroup );

void CORE_API                      Log_Group( LogGroup sGroup, const char* spBuf );
void CORE_API                      Log_GroupF( LogGroup sGroup, const char* spFmt, ... );
void CORE_API                      Log_GroupV( LogGroup sGroup, const char* spFmt, va_list args );

// ----------------------------------------------------------------
// Extended Logging Functions

void CORE_API                      Log_Ex( LogChannel channel, LogType sLevel, const char* spBuf );
void CORE_API                      Log_ExF( LogChannel channel, LogType sLevel, const char* spFmt, ... );
void CORE_API                      Log_ExV( LogChannel channel, LogType sLevel, const char* spFmt, va_list args );

// ----------------------------------------------------------------
// Standard Logging Functions

// Lowest severity.
void CORE_API                      Log_Msg( LogChannel channel, const char* spBuf );
void CORE_API                      Log_MsgF( LogChannel channel, const char* spFmt, ... );

// Medium severity.
void CORE_API                      Log_Warn( LogChannel channel, const char* spBuf );
void CORE_API                      Log_WarnF( LogChannel channel, const char* spFmt, ... );

// High severity.
void CORE_API                      Log_Error( LogChannel channel, const char* spBuf );
void CORE_API                      Log_ErrorF( LogChannel channel, const char* spFmt, ... );

// Extreme severity.
void CORE_API                      Log_Fatal( LogChannel channel, const char* spBuf );
void CORE_API                      Log_FatalF( LogChannel channel, const char* spFmt, ... );

// Dev only.
void CORE_API                      Log_Dev( LogChannel channel, u8 sLvl, const char* spBuf );
void CORE_API                      Log_DevF( LogChannel channel, u8 sLvl, const char* spFmt, ... );

// ----------------------------------------------------------------
// Default to general channel.

// Lowest severity.
void CORE_API                      Log_Msg( const char* spFmt );
void CORE_API                      Log_MsgF( const char* spFmt, ... );

// Medium severity.
void CORE_API                      Log_Warn( const char* spFmt );
void CORE_API                      Log_WarnF( const char* spFmt, ... );

// High severity.
void CORE_API                      Log_Error( const char* spFmt );
void CORE_API                      Log_ErrorF( const char* spFmt, ... );

// Extreme severity.
void CORE_API                      Log_Fatal( const char* spFmt );
void CORE_API                      Log_FatalF( const char* spFmt, ... );

// Dev only.
void CORE_API                      Log_Dev( u8 sLvl, const char* spFmt );
void CORE_API                      Log_DevF( u8 sLvl, const char* spFmt, ... );

// ----------------------------------------------------------------
// Helper Macros

#define LOG_REGISTER_CHANNEL( name, ... )         LogChannel g##name##Channel = Log_RegisterChannel( #name, __VA_ARGS__ );
#define LOG_REGISTER_CHANNEL2( name, ... )        LogChannel gLC_##name = Log_RegisterChannel( #name, __VA_ARGS__ );
#define LOG_REGISTER_CHANNEL_EX( var, name, ... ) LogChannel var = Log_RegisterChannel( name, __VA_ARGS__ );

#define LOG_CHANNEL( name )                       extern LogChannel g##name##Channel;
#define LOG_CHANNEL2( name )                      extern LogChannel gLC_##name;

