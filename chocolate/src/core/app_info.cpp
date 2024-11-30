#include "core/core.h"
#include "core/json5.h"
#include "core/app_info.h"


static AppInfo_t gAppInfo;

std::vector< std::string > gAppInfoPaths;

constexpr const char* gAppInfoFileName    = CH_PATH_SEP_STR "app_info.json5";
const u64             gAppInfoFileNameLen = strlen( gAppInfoFileName );


void Core_WriteDefaultAppInfo()
{
}


static bool Core_SetAppInfoString( char*& spDst, const char* spSrc )
{
	if ( !spSrc )
		return false;

	char* newData = ch_str_realloc( spDst, spSrc, strlen( spSrc ) ).data;

	if ( newData == nullptr )
	{
		Log_ErrorF( "Failed to allocate memory for string: %s\n", spSrc );
		
		if ( spDst )
			ch_str_free( spDst );

		return false;
	}

	spDst = newData;
	return true;
}


void Core_HandleSearchPathType( JsonObject_t& cur, ESearchPathType sType )
{
	if ( cur.aType != EJsonType_Array )
	{
		Log_WarnF( "Invalid Type for \"binPaths\" value in app_info.json5: %s - Expected Array\n", Json_TypeToStr( cur.aType ) );
		return;
	}

	for ( size_t j = 0; j < cur.aObjects.aCount; j++ )
	{
		if ( cur.aObjects.apData[ j ].aType != EJsonType_String )
		{
			Log_WarnF( "Invalid Value Type for \"binPaths\" array in app_info.json5: %s - Expected String\n", Json_TypeToStr( cur.aType ) );
			continue;
		}

		FileSys_AddSearchPath( cur.aObjects.apData[ j ].aString.data, cur.aObjects.apData[ j ].aString.size, sType );
	}
}


bool Core_ParseSearchPaths( JsonObject_t& root )
{
	for ( size_t i = 0; i < root.aObjects.aCount; i++ )
	{
		JsonObject_t& cur = root.aObjects.apData[ i ];

		if ( ch_str_equals( cur.name, "binPaths", 8 ) )
		{
			Core_HandleSearchPathType( cur, ESearchPathType_Binary );
		}
		else if ( ch_str_equals( cur.name, "paths", 5 ) )
		{
			Core_HandleSearchPathType( cur, ESearchPathType_Path );
		}
		else if ( ch_str_equals( cur.name, "sourceAssets", 12 ) )
		{
			Core_HandleSearchPathType( cur, ESearchPathType_SourceAssets );
		}
		else
		{
			Log_WarnF( "Unknown Search Path Key in app_info.json5: \"%s\"\n", cur.name.data );
			continue;
		}
	}

	return true;
}


bool Core_GetAppInfoJson( JsonObject_t& srRoot, const char* appPath, s64 appPathLen )
{
	ch_string fullPath;

	if ( appPath && appPathLen < 0 )
	{
		appPathLen = strlen( appPath );
	}

	if ( !appPath || appPathLen == 0 )
	{
		const char* strings[] = { FileSys_GetExePath().data, PATH_SEP_STR, FileSys_GetWorkingDir().data, gAppInfoFileName };
		const u64   lengths[] = { FileSys_GetExePath().size, 1, FileSys_GetWorkingDir().size, gAppInfoFileNameLen };
		fullPath              = ch_str_join( 4, strings, lengths );
	}
	else
	{

		const char* strings[] = { appPath, PATH_SEP_STR, gAppInfoFileName };
		const u64   lengths[] = { appPathLen, 1, gAppInfoFileNameLen };
		fullPath              = ch_str_join( 3, strings, lengths );
	}

	if ( !FileSys_IsFile( fullPath.data, fullPath.size, true ) )
	{
		// Log_Error( "Failed to Find app_info.json5 file, use -def-app-info to write a default one\n" );
		Log_Error( "Failed to Find app_info.json5 file\n" );
		SDL_ShowSimpleMessageBox( SDL_MESSAGEBOX_ERROR, "App Info Error", "No app_info.json5 file found!", NULL );
		return false;
	}

	ch_string_auto data = FileSys_ReadFile( fullPath.data, fullPath.size );

	if ( !data.data )
	{
		Log_Error( "app_info.json5 file is empty!\n" );
		SDL_ShowSimpleMessageBox( SDL_MESSAGEBOX_ERROR, "App Info Error", "App Info file is empty!", NULL );
		return false;
	}

	EJsonError err = Json_Parse( &srRoot, data.data );

	if ( err != EJsonError_None )
	{
		Log_ErrorF( "Error Parsing App Info: %d\n", err );
		return false;
	}

	return true;
}


bool Core_LoadAppInfo()
{
	JsonObject_t root;
	if ( !Core_GetAppInfoJson( root, "" ) )
	{
		return false;
	}

	for ( size_t i = 0; i < root.aObjects.aCount; i++ )
	{
		JsonObject_t& cur = root.aObjects.apData[ i ];

		if ( ch_str_equals( cur.name, "name", 4 ) )
		{
			if ( cur.aType != EJsonType_String )
			{
				Log_WarnF( "Invalid Type for \"name\" value in app_info.json5: %s\n", Json_TypeToStr( cur.aType ) );
				continue;
			}

			gAppInfo.apName = ch_str_copy( cur.aString.data, cur.aString.size ).data;

			if ( gAppInfo.apName == nullptr )
			{
				Log_ErrorF( "Failed to allocate memory for \"name\" value: %s\n", cur.aString.data );
				Json_Free( &root );
				return false;
			}

			continue;
		}
		else if ( ch_str_equals( cur.name, "windowTitle", 11 ) )
		{
			if ( cur.aType != EJsonType_String )
			{
				Log_WarnF( "Invalid Type for \"windowTitle\" value in app_info.json5: %s\n", Json_TypeToStr( cur.aType ) );
				continue;
			}

			gAppInfo.apWindowTitle = ch_str_copy( cur.aString.data, cur.aString.size ).data;

			if ( gAppInfo.apWindowTitle == nullptr )
			{
				Log_ErrorF( "Failed to allocate memory for \"windowTitle\" value: %s\n", cur.aString.data );
				Json_Free( &root );
				return false;
			}

			continue;
		}
		else if ( ch_str_equals( cur.name, "searchPaths", 11 ) )
		{
			if ( cur.aType != EJsonType_Object )
			{
				Log_WarnF( "Invalid Type for \"searchPaths\" value in app_info.json5: %s - needs to be Object {}\n", Json_TypeToStr( cur.aType ) );
				continue;
			}

			if ( !Core_ParseSearchPaths( cur ) )
			{
				Log_Error( "Failed to parse app info search paths\n" );
				Json_Free( &root );
				return false;
			}
		}
		else
		{
			Log_WarnF( "Unknown Key in app_info.json5: \"%s\"\n", cur.name );
			continue;
		}
	}

	Json_Free( &root );

	if ( gAppInfo.apName == nullptr )
	{
		ch_string name = FileSys_GetWorkingDir();
		Log_WarnF( "\"name\" value is undefined in app_info.json5, defaulting to %s\n", name.data );

		if ( !Core_SetAppInfoString( gAppInfo.apName, name.data ) )
		{
			return false;
		}
	}

	if ( gAppInfo.apWindowTitle == nullptr )
	{
		ch_string name = FileSys_GetWorkingDir();
		Log_WarnF( "\"windowTitle\" value is undefined in app_info.json5, defaulting to %s\n", name.data );

		if ( !Core_SetAppInfoString( gAppInfo.apWindowTitle, name.data ) )
		{
			return false;
		}
	}

	Log_Dev( 1, "Loaded Main App Info File\n" );
	return true;
}


void Core_DestroyAppInfo()
{
	if ( gAppInfo.apName )
		ch_str_free( gAppInfo.apName );

	if ( gAppInfo.apWindowTitle )
		ch_str_free( gAppInfo.apWindowTitle );

	gAppInfo.apName = nullptr;
	gAppInfo.apWindowTitle = nullptr;
}


bool Core_AddAppInfo( const char* appPath, s64 appPathLen )
{
	if ( appPath == nullptr )
	{
		Log_Error( "App Path is NULL\n" );
		return false;
	}

	if ( appPathLen < 0 )
	{
		appPathLen = strlen( appPath );
	}

	if ( appPathLen == 0 )
	{
		Log_Error( "App Path is Empty\n" );
		return false;
	}

	JsonObject_t root;
	if ( !Core_GetAppInfoJson( root, appPath ) )
	{
		return false;
	}

	FileSys_SetAppPathMacro( appPath, appPathLen );

	for ( size_t i = 0; i < root.aObjects.aCount; i++ )
	{
		JsonObject_t& cur = root.aObjects.apData[ i ];

		if ( ch_str_equals( cur.name, "searchPaths", 11 ) )
		{
			if ( cur.aType != EJsonType_Object )
			{
				Log_WarnF( "Invalid Type for \"searchPaths\" value in app_info.json5: %s - needs to be Object {}\n", Json_TypeToStr( cur.aType ) );
				continue;
			}

			if ( !Core_ParseSearchPaths( cur ) )
			{
				Log_Error( "Failed to parse app info search paths\n" );
				Json_Free( &root );
				FileSys_SetAppPathMacro( "" );
				return false;
			}
		}
	}

	Json_Free( &root );
	FileSys_SetAppPathMacro( "" );

	Log_DevF( 1, "Loaded App Info File: %s\n", appPath );
	return true;
}


void Core_ReloadSearchPaths()
{
	FileSys_ClearSearchPaths();
	FileSys_ClearBinPaths();
	FileSys_ClearSourcePaths();

	// TODO: gAppInfoPaths is unused??? huh???
	for ( const std::string& appPath : gAppInfoPaths )
	{
		JsonObject_t root;
		if ( !Core_GetAppInfoJson( root, appPath.data(), appPath.size() ) )
			return;

		for ( size_t i = 0; i < root.aObjects.aCount; i++ )
		{
			JsonObject_t& cur = root.aObjects.apData[ i ];

			if ( ch_str_equals( cur.name, "searchPaths", 11 ) )
			{
				if ( cur.aType != EJsonType_Object )
				{
					Log_WarnF( "Invalid Type for \"searchPaths\" value in app_info.json5: %s - needs to be Object {}\n", Json_TypeToStr( cur.aType ) );
					continue;
				}

				if ( !Core_ParseSearchPaths( cur ) )
				{
					Log_Error( "Failed to parse app info search paths\n" );
					Json_Free( &root );
					return;
				}
			}
		}

		Json_Free( &root );
	}
}


const AppInfo_t& Core_GetAppInfo()
{
	return gAppInfo;
}

