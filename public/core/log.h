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


/* Lowest severity.  */
void CORE_API LogMsg( const char *spFmt, ... );

/* Lowest severity - Color Option.  */
void CORE_API LogMsg( LogColor color, const char *spFmt, ... );

/* Lowest severity, no format.  */
void CORE_API LogPuts( const char *spBuf, LogColor color = LogColor::Default );

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

/* Set Console Color */
void CORE_API LogSetColor( LogColor color );

