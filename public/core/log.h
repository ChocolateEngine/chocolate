#pragma once

#include "platform.h"
#include "util.h"


/* Colors!!! */
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
	Fatal     // "[Channel] a[FATAL] %s"
};


/* Manage Console Color */
void                  CORE_API  LogSetColor( LogColor color );
LogColor              CORE_API  LogGetColor();
const char            CORE_API *LogColorToStr( LogColor color );


constexpr LogColor LOG_COLOR_WARNING = LogColor::Yellow;
constexpr LogColor LOG_COLOR_ERROR   = LogColor::Red;


/* Channels!!! */
struct LogChannel_t
{
	bool            aShown;
	std::string     aName;
	LogColor        aColor;
};

using LogChannel = unsigned char;
constexpr LogChannel INVALID_LOG_CHANNEL = 255;

/* Register Log Channel */
LogChannel                  CORE_API  LogRegisterChannel( const char *sName, LogColor sColor = LogColor::Default );
LogChannel                  CORE_API  LogGetChannel( const char *sName );
LogColor                    CORE_API  LogGetChannelColor( LogChannel handle );
const std::string           CORE_API &LogGetChannelName( LogChannel handle );
bool                        CORE_API  LogChannelIsShown( LogChannel handle );
unsigned char               CORE_API  LogGetChannelCount();


struct Log
{
	LogChannel      aChannel;
	LogType         aType;
	std::string     aMessage;
	std::string     aFormatted;
};

const std::string           CORE_API &LogGetHistoryStr( int maxSize );
const std::vector< Log >    CORE_API &LogGetLogHistory();
const Log                   CORE_API *LogGetLastLog();
bool                        CORE_API  LogIsVisible( const Log& log );


/*ReBuildConsoleOutput*/
typedef std::function< void(  ) > LogChannelShownCallbackF;

void CORE_API LogAddChannelShownCallback( LogChannelShownCallbackF callback );


#define LOG_REGISTER_CHANNEL( name, ... ) LogChannel g##name##Channel = LogRegisterChannel( #name, __VA_ARGS__ );
#define LOG_CHANNEL( name ) extern LogChannel g##name##Channel;

/* Deprecated */
void CORE_API Print( const char* str, ... );
void CORE_API Puts( const char* str );


// ----------------------------------------------------------------
// Specify a custom channel.

/* General Logging Function.  */
void CORE_API LogEx(  LogChannel channel, LogType sLevel, const char *spBuf );
void CORE_API LogExF( LogChannel channel, LogType sLevel, const char *spFmt, ... );

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

