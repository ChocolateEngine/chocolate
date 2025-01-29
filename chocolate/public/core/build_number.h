/*
build_number.h ( Authored by Demez )
*/

#pragma once

#include "core/platform.h"

/*
 *    Time in seconds since the chocolate epoch.
 *    Same as the seconds from the unix epoch from the first commit.
 * 
 *    @return s64
 *        The number of seconds since the chocolate epoch. 
 *        time since 2021-02-15 19:11:26 UTC from the current time.
 *        2021-02-15 14:11:26 in the original time zone, EST.
 */
CORE_API size_t      Core_GetBuildNumber();

// Returns the Unix Time of The very first commit to the chocolate engine repo
// The date is 2021-02-15 19:11:26 UTC, or 2021-02-15 14:11:26 in the original time zone, EST.
CORE_API size_t      Core_GetChocolateEpoch();

/*
 *    @return size_t
 *        Time in seconds when the engine was last built
 */
CORE_API size_t      Core_GetBuildTimeUnix();

/*
 *    @return const char*
 *        The __DATE__ Preproccessor Macro
 */
CORE_API const char* Core_GetBuildDate();

/*
 *    @return const char*
 *        The __TIME__ Preproccessor Macro
 */
CORE_API const char* Core_GetBuildTime();

