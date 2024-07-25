/*
app_info.h ( Authored by Demez )

Loads an app_info.json5 file in the app's folder
*/

#pragma once

#include "core/platform.h"
#include "core/json5.h"


// built in app info
struct AppInfo_t
{
	char* apName        = nullptr;
	char* apWindowTitle = nullptr;  // kinda weird having this here
};


// Leave empty to load the current app's info
CORE_API bool             Core_GetAppInfoJson( JsonObject_t& srRoot, const char* appPath, s64 appPathLen = -1 );
CORE_API bool             Core_LoadAppInfo();
CORE_API bool             Core_AddAppInfo( const char* appPath, s64 appPathLen = -1 );
CORE_API void             Core_DestroyAppInfo();
CORE_API void             Core_ReloadSearchPaths();

CORE_API const AppInfo_t& Core_GetAppInfo();



