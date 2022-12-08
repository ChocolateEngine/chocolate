#include "core/build_number.h"


constexpr size_t gChocolateEpoch = 1613416286;

/*
 *    Time in seconds since the chocolate epoch.
 *    Same as the seconds from the unix epoch from the first commit.
 * 
 *    @return s64
 *        The number of seconds since the chocolate epoch. 
 *        time since 2021-02-15 19:11:26 UTC from the current time.
 *        2021-02-15 14:11:26 in the original time zone, EST.
 */
size_t Core_GetBuildNumber()
{
	static size_t buildNumber = 0;
	if ( buildNumber != 0 )
		return buildNumber;

	// allocate a tempory buffer to add null ternimators to
	char* buildTime = CH_STACK_NEW( char, 25 );

	if ( buildTime == nullptr )
	{
		Log_Fatal( "Failed to allocate memory for temporary build time buffer" );
		return 0;
	}

	memcpy( buildTime, __TIMESTAMP__ "\0", 25 );
	
	// Add null terminators
	buildTime[ 7 ]  = '\0';
	buildTime[ 10 ] = '\0';
	buildTime[ 13 ] = '\0';
	buildTime[ 16 ] = '\0';
	buildTime[ 19 ] = '\0';

	// Convert to multiple integers
	int year        = atoi( &buildTime[ 20 ] );
	int month       = 0;
	int day         = atoi( &buildTime[ 8 ] );
	int hour        = atoi( &buildTime[ 11 ] );
	int min         = atoi( &buildTime[ 14 ] );
	int sec         = atoi( &buildTime[ 17 ] );

	// special case for month, smh
	if ( strcmp( "Jan", &buildTime[ 4 ] ) == 0 )
		month = 0;
	else if ( strcmp( "Feb", &buildTime[ 4 ] ) == 0 )
		month = 1;
	else if ( strcmp( "Mar", &buildTime[ 4 ] ) == 0 )
		month = 2;
	else if ( strcmp( "Apr", &buildTime[ 4 ] ) == 0 )
		month = 3;
	else if ( strcmp( "May", &buildTime[ 4 ] ) == 0 )
		month = 4;
	else if ( strcmp( "Jun", &buildTime[ 4 ] ) == 0 )
		month = 5;
	else if ( strcmp( "Jul", &buildTime[ 4 ] ) == 0 )
		month = 6;
	else if ( strcmp( "Aug", &buildTime[ 4 ] ) == 0 )
		month = 7;
	else if ( strcmp( "Sep", &buildTime[ 4 ] ) == 0 )
		month = 8;
	else if ( strcmp( "Oct", &buildTime[ 4 ] ) == 0 )
		month = 9;
	else if ( strcmp( "Nov", &buildTime[ 4 ] ) == 0 )
		month = 10;
	else if ( strcmp( "Dec", &buildTime[ 4 ] ) == 0 )
		month = 11;

	time_t rawtime;
	time( &rawtime );  //or: rawtime = time(0);
	struct tm* timeinfo = gmtime( &rawtime );

	// Set build date info
	timeinfo->tm_year   = year - 1900;
	timeinfo->tm_mon    = month;
	timeinfo->tm_mday   = day;
	timeinfo->tm_hour   = hour;
	timeinfo->tm_min    = min;
	timeinfo->tm_sec    = sec;

	// Create a unix timestamp
	time_t date         = mktime( timeinfo );

	// Calcualte the build number
	buildNumber         = date - gChocolateEpoch;

	// Free the temporary string
	CH_STACK_FREE( buildTime );

	return buildNumber;
}

