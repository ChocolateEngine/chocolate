#include "platform.h"
#include "util.h"

/* Lowest severity.  */
void LogMsg( const char *spFmt, ... );

/* Medium severity.  */
void LogWarn( const char *spFmt, ... );

/* High severity.  */
void LogError( const char *spFmt, ... );

/* Extreme severity.  */
void LogFatal( const char *spFmt, ... );

/* Dev only.  */
void LogDev( u8 sLvl, const char *spFmt, ... );
