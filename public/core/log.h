#pragma once

#include "platform.h"
#include "util.h"


/* Colors!!! */
enum class LogColor
{
	Default,

	Black,
	DarkBlue,
	DarkGreen,
	DarkCyan,
	DarkRed,
	DarkMagenta,
	DarkYellow,
	DarkGray,

	Gray,
	Blue,
	Green,
	Cyan,
	Red,
	Magenta,
	Yellow,
	White,

	Max = White,
};


enum class LogType: unsigned char
{
	Normal = 0,
	Dev,
	Dev2,
	Dev3,
	Dev4,
	Input,
	Warning,
	Error,
	Fatal
};


/* Manage Console Color */
void     CORE_API LogSetColor( LogColor color );
LogColor CORE_API LogGetColor();


/* Channels!!! */
struct LogChannel_t
{
	bool aShown;
	std::string aName;
	LogColor aColor;
};

using LogChannel = unsigned char;
constexpr LogChannel INVALID_LOG_CHANNEL = 255;
constexpr int        LOG_MAX_LENGTH      = 2048;

/* Register Log Channel */
LogChannel CORE_API LogRegisterChannel( const char *sName, LogColor sColor = LogColor::Default );

#define LOG_REGISTER_CHANNEL( name, ... ) LogChannel g##name##Channel = LogRegisterChannel( #name, __VA_ARGS__ );
#define LOG_CHANNEL( name ) extern LogChannel g##name##Channel;

void CORE_API LogGetHistoryStr( std::string& output, int maxSize );

/* Deprecated */
void CORE_API Print( const char* str, ... );
void CORE_API Puts( const char* str );


// ----------------------------------------------------------------
// Specify a custom channel.

/* General Logging Function.  */
void CORE_API Log(  LogChannel channel, LogType sLevel, const char *spBuf );
void CORE_API LogF( LogChannel channel, LogType sLevel, const char *spFmt, ... );

/* Lowest severity.  */
void CORE_API LogMsg( LogChannel channel, const char *spFmt, ... );

/* Lowest severity, no format.  */
void CORE_API LogPuts( LogChannel channel, const char *spBuf );

/* Medium severity.  */
void CORE_API LogWarn( LogChannel channel, const char *spFmt, ... );

/* High severity.  */
void CORE_API LogError( LogChannel channel, const char *spFmt, ... );

/* Extreme severity.  */
void CORE_API LogFatal( LogChannel channel, const char *spFmt, ... );

/* Dev only.  */
void CORE_API LogDev( LogChannel channel, u8 sLvl, const char *spFmt, ... );

/* Dev only.  */
void CORE_API LogPutsDev( LogChannel channel, u8 sLvl, const char *spBuf );


// ----------------------------------------------------------------
// Default to general channel.

/* Lowest severity.  */
void CORE_API LogMsg( const char *spFmt, ... );

/* Lowest severity, no format.  */
void CORE_API LogPuts( const char *spBuf );

/* Medium severity.  */
void CORE_API LogWarn( const char *spFmt, ... );

/* High severity.  */
void CORE_API LogError( const char *spFmt, ... );

/* Extreme severity.  */
void CORE_API LogFatal( const char *spFmt, ... );

/* Dev only.  */
void CORE_API LogDev( u8 sLvl, const char *spFmt, ... );

/* Dev only.  */
void CORE_API LogPutsDev( u8 sLvl, const char *spBuf );

