#pragma once

#include "platform.h"
#include "util.h"

#include <functional>

enum class LogColor: unsigned char
{
	Default,

	Black,
	White,

	DarkBlue,
	DarkGreen,
	DarkCyan,
	DarkRed,
	DarkMagenta,
	DarkYellow,
	DarkGray,

	Blue,
	Green,
	Cyan,
	Red,
	Magenta,
	Yellow,
	Gray,

	Max = Gray,
	Count,
};


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
	bool        aShown;
	std::string aName;
	LogColor    aColor;
};

using LogChannel = unsigned char;


struct Log
{
	LogChannel  aChannel;
	LogType     aType;
	std::string aMessage;
	std::string aFormatted;
};


/*ReBuildConsoleOutput*/
typedef std::function< void() > LogChannelShownCallbackF;


constexpr LogColor   LOG_COLOR_WARNING   = LogColor::Yellow;
constexpr LogColor   LOG_COLOR_ERROR     = LogColor::Red;
constexpr LogChannel INVALID_LOG_CHANNEL = 255;

void CORE_API                         Log_Init();

// Manage Console Color
void                        CORE_API  Log_SetColor( LogColor color );
LogColor                    CORE_API  Log_GetColor();
const char                  CORE_API *Log_ColorToStr( LogColor color );

// Manage Log Channels
CORE_API LogChannel                   Log_RegisterChannel( const char* sName, LogColor sColor = LogColor::Default );
LogChannel                  CORE_API  Log_GetChannel( const char *sName );
LogColor                    CORE_API  Log_GetChannelColor( LogChannel handle );
const std::string           CORE_API &Log_GetChannelName( LogChannel handle );
bool                        CORE_API  Log_ChannelIsShown( LogChannel handle );
unsigned char               CORE_API  Log_GetChannelCount();

// Log Information
const std::string           CORE_API &Log_GetHistoryStr( int maxSize );
const std::vector< Log >    CORE_API &Log_GetLogHistory();
const Log                   CORE_API *Log_GetLastLog();
bool                        CORE_API  Log_IsVisible( const Log& log );

void                        CORE_API  Log_AddChannelShownCallback( LogChannelShownCallbackF callback );


#define LOG_REGISTER_CHANNEL( name, ... ) LogChannel g##name##Channel = Log_RegisterChannel( #name, __VA_ARGS__ );
#define LOG_REGISTER_CHANNEL2( name, ... ) LogChannel gLC_##name = Log_RegisterChannel( #name, __VA_ARGS__ );
#define LOG_REGISTER_CHANNEL_EX( var, name, ... ) LogChannel var = Log_RegisterChannel( name, __VA_ARGS__ );

#define LOG_CHANNEL( name ) extern LogChannel g##name##Channel;
#define LOG_CHANNEL2( name ) extern LogChannel gLC_##name;

// ----------------------------------------------------------------
// System printing, skip logging

void CORE_API PrintF( const char* str, ... );
void CORE_API Print( const char* str );


// ----------------------------------------------------------------
// Extended Logging Functions

void CORE_API Log_Ex(  LogChannel channel, LogType sLevel, const char *spBuf );
void CORE_API Log_ExF( LogChannel channel, LogType sLevel, const char *spFmt, ... );
void CORE_API Log_ExV( LogChannel channel, LogType sLevel, const char* spFmt, va_list args );


// ----------------------------------------------------------------
// Standard Logging Functions

// Lowest severity.
void CORE_API Log_Msg( LogChannel channel, const char* spBuf );
void CORE_API Log_MsgF( LogChannel channel, const char* spFmt, ... );

// Medium severity.
void CORE_API Log_Warn( LogChannel channel, const char* spBuf );
void CORE_API Log_WarnF( LogChannel channel, const char* spFmt, ... );

// High severity.
void CORE_API Log_Error( LogChannel channel, const char* spBuf );
void CORE_API Log_ErrorF( LogChannel channel, const char* spFmt, ... );

// Extreme severity.
void CORE_API Log_Fatal( LogChannel channel, const char* spBuf );
void CORE_API Log_FatalF( LogChannel channel, const char* spFmt, ... );

// Dev only.
void CORE_API Log_Dev( LogChannel channel, u8 sLvl, const char* spBuf );
void CORE_API Log_DevF( LogChannel channel, u8 sLvl, const char* spFmt, ... );


// ----------------------------------------------------------------
// Default to general channel.

// Lowest severity.
void CORE_API Log_Msg( const char* spFmt );
void CORE_API Log_MsgF( const char* spFmt, ... );

// Medium severity.
void CORE_API Log_Warn( const char* spFmt );
void CORE_API Log_WarnF( const char* spFmt, ... );

// High severity.
void CORE_API Log_Error( const char* spFmt );
void CORE_API Log_ErrorF( const char* spFmt, ... );

// Extreme severity.
void CORE_API Log_Fatal( const char* spFmt );
void CORE_API Log_FatalF( const char* spFmt, ... );

// Dev only.
void CORE_API Log_Dev( u8 sLvl, const char* spFmt );
void CORE_API Log_DevF( u8 sLvl, const char* spFmt, ... );

