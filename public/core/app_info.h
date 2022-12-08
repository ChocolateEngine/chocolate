/*
app_info.h ( Authored by Demez )

Loads an app_info.json5 file in the app's folder
*/

#pragma once

#include "core/platform.h"


// built in app info
struct AppInfo_t
{
	char* apName        = nullptr;
	char* apWindowTitle = nullptr;  // kinda weird having this here
};


CORE_API bool             Core_LoadAppInfo();
CORE_API void             Core_DestroyAppInfo();

CORE_API const AppInfo_t& Core_GetAppInfo();


/*
 *    Time in seconds since the chocolate epoch.
 *    Same as the seconds from the unix epoch from the first commit.
 * 
 *    @return size_t
 *        The number of seconds since the chocolate epoch. 
 *        time since 2/15/2021 14:11:26 from the current time.
 */
CORE_API size_t           Core_GetBuildNumber();

