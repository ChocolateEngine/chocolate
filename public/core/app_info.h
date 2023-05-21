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



