/*
build_number.h ( Authored by Demez )
*/

#pragma once

#include "core/platform.h"

/*
 *    Time in seconds since the chocolate epoch.
 *    Same as the seconds from the unix epoch from the first commit.
 * 
 *    @return size_t
 *        The number of seconds since the chocolate epoch. 
 *        time since 2/15/2021 14:11:26 from the current time.
 */
CORE_API size_t Core_GetBuildNumber();

