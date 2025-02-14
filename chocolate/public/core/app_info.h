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
CORE_API bool             core_app_info_json( JsonObject_t& srRoot, const char* appPath, size_t appPathLen = 0 );
CORE_API const AppInfo_t& core_app_info_get();
CORE_API bool             core_app_info_load();
CORE_API void             core_app_info_free();

CORE_API bool             core_search_paths_add_app( const char* appPath, size_t appPathLen = 0 );
CORE_API void             core_search_paths_reload();

