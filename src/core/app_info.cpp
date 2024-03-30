#include "core/core.h"
#include "core/json5.h"
#include "core/app_info.h"


static AppInfo_t gAppInfo;

std::vector< std::string > gAppInfoPaths;

constexpr const char* gAppInfoFileName = PATH_SEP_STR "app_info.json5";


void Core_WriteDefaultAppInfo()
{
}


static bool Core_SetAppInfoString( char*& spDst, const char* spSrc )
{
	if ( !spSrc )
		return false;

	size_t len  = strlen( spSrc );
	void*  data = realloc( (void*)spDst, len * sizeof( char ) );

	if ( data == nullptr )
	{
		Log_ErrorF( "Failed to allocate memory for string: %s\n", spSrc );
		return false;
	}

	memcpy( data, spSrc, len * sizeof( char ) );
	spDst = static_cast< char* >( data );
	return true;
}


void Core_HandleSearchPathType( JsonObject_t& cur, ESearchPathType sType )
{
	if ( cur.aType != EJsonType_Array )
	{
		Log_WarnF( "Invalid Type for \"binPaths\" value in app_info.json5: %s - Expected Array\n", Json_TypeToStr( cur.aType ) );
		return;
	}

	for ( size_t j = 0; j < cur.aObjects.size(); j++ )
	{
		if ( cur.aObjects[ j ].aType != EJsonType_String )
		{
			Log_WarnF( "Invalid Value Type for \"binPaths\" array in app_info.json5: %s - Expected String\n", Json_TypeToStr( cur.aType ) );
			continue;
		}

		FileSys_AddSearchPath( cur.aObjects[ j ].apString, sType );
	}
}


bool Core_ParseSearchPaths( JsonObject_t& root )
{
	for ( size_t i = 0; i < root.aObjects.size(); i++ )
	{
		JsonObject_t& cur = root.aObjects[ i ];

		if ( strcmp( cur.apName, "binPaths" ) == 0 )
		{
			Core_HandleSearchPathType( cur, ESearchPathType_Binary );
		}
		else if ( strcmp( cur.apName, "paths" ) == 0 )
		{
			Core_HandleSearchPathType( cur, ESearchPathType_Path );
		}
		else if ( strcmp( cur.apName, "sourceAssets" ) == 0 )
		{
			Core_HandleSearchPathType( cur, ESearchPathType_SourceAssets );
		}
		else
		{
			Log_WarnF( "Unknown Search Path Key in app_info.json5: \"%s\"\n", cur.apName );
			continue;
		}
	}

	return true;
}


bool Core_GetAppInfoJson( JsonObject_t& srRoot, const std::string& appPath )
{
	std::string fullPath;

	if ( appPath.empty() )
		fullPath = FileSys_GetExePath() + PATH_SEP_STR + FileSys_GetWorkingDir() + gAppInfoFileName;
	else
		fullPath = appPath + PATH_SEP_STR + gAppInfoFileName;

	if ( !FileSys_IsFile( fullPath, true ) )
	{
		// Log_Error( "Failed to Find app_info.json5 file, use -def-app-info to write a default one\n" );
		Log_Error( "Failed to Find app_info.json5 file\n" );
		SDL_ShowSimpleMessageBox( SDL_MESSAGEBOX_ERROR, "App Info Error", "No app_info.json5 file found!", NULL );
		return false;
	}

	std::vector< char > data = FileSys_ReadFile( fullPath );

	if ( data.empty() )
	{
		Log_Error( "app_info.json5 file is empty!\n" );
		SDL_ShowSimpleMessageBox( SDL_MESSAGEBOX_ERROR, "App Info Error", "App Info file is empty!", NULL );
		return false;
	}

	EJsonError err = Json_Parse( &srRoot, data.data() );

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

	for ( size_t i = 0; i < root.aObjects.size(); i++ )
	{
		JsonObject_t& cur = root.aObjects[ i ];

		if ( strcmp( cur.apName, "name" ) == 0 )
		{
			if ( cur.aType != EJsonType_String )
			{
				Log_WarnF( "Invalid Type for \"name\" value in app_info.json5: %s\n", Json_TypeToStr( cur.aType ) );
				continue;
			}

			size_t len  = strlen( cur.apString );
			void*  data = realloc( (void*)gAppInfo.apName, ( len + 1 ) * sizeof( char ) );

			if ( data == nullptr )
			{
				Log_ErrorF( "Failed to allocate memory for \"name\" value: %s\n", cur.apString );
				Json_Free( &root );
				return false;
			}

			memcpy( data, cur.apString, len * sizeof( char ) );
			gAppInfo.apName = static_cast< char* >( data );
			gAppInfo.apName[ len ] = '\0';
			continue;
		}
		else if ( strcmp( cur.apName, "windowTitle" ) == 0 )
		{
			if ( cur.aType != EJsonType_String )
			{
				Log_WarnF( "Invalid Type for \"windowTitle\" value in app_info.json5: %s\n", Json_TypeToStr( cur.aType ) );
				continue;
			}

			size_t len  = strlen( cur.apString );
			void*  data = realloc( (void*)gAppInfo.apWindowTitle, ( len + 1 ) * sizeof( char ) );

			if ( data == nullptr )
			{
				Log_ErrorF( "Failed to allocate memory for \"windowTitle\" value: %s\n", cur.apString );
				Json_Free( &root );
				return false;
			}

			memcpy( data, cur.apString, len * sizeof( char ) );
			gAppInfo.apWindowTitle = static_cast< char* >( data );
			gAppInfo.apWindowTitle[ len ] = '\0';
			continue;
		}
		else if ( strcmp( cur.apName, "searchPaths" ) == 0 )
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
			Log_WarnF( "Unknown Key in app_info.json5: \"%s\"\n", cur.apName );
			continue;
		}
	}

	Json_Free( &root );

	if ( gAppInfo.apName == nullptr )
	{
		std::string name = FileSys_GetWorkingDir();
		Log_WarnF( "\"name\" value is undefined in app_info.json5, defaulting to %s\n", name.c_str() );

		if ( !Core_SetAppInfoString( gAppInfo.apName, name.c_str() ) )
		{
			return false;
		}
	}

	if ( gAppInfo.apWindowTitle == nullptr )
	{
		std::string name = FileSys_GetWorkingDir();
		Log_WarnF( "\"windowTitle\" value is undefined in app_info.json5, defaulting to %s\n", name.c_str() );

		if ( !Core_SetAppInfoString( gAppInfo.apWindowTitle, name.c_str() ) )
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
		free( (void*)gAppInfo.apName );

	if ( gAppInfo.apWindowTitle )
		free( (void*)gAppInfo.apWindowTitle );

	gAppInfo.apName = nullptr;
	gAppInfo.apWindowTitle = nullptr;
}


bool Core_AddAppInfo( const std::string& appPath )
{
	JsonObject_t root;
	if ( !Core_GetAppInfoJson( root, appPath ) )
	{
		return false;
	}

	FileSys_SetAppPathMacro( appPath );

	for ( size_t i = 0; i < root.aObjects.size(); i++ )
	{
		JsonObject_t& cur = root.aObjects[ i ];

		if ( strcmp( cur.apName, "searchPaths" ) == 0 )
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

	Log_DevF( 1, "Loaded App Info File: %s\n", appPath.data() );
	return true;
}


void Core_ReloadSearchPaths()
{
	FileSys_ClearSearchPaths();
	FileSys_ClearBinPaths();
	FileSys_ClearSourcePaths();

	for ( const std::string& appPath : gAppInfoPaths )
	{
		JsonObject_t root;
		if ( !Core_GetAppInfoJson( root, "" ) )
			return;

		for ( size_t i = 0; i < root.aObjects.size(); i++ )
		{
			JsonObject_t& cur = root.aObjects[ i ];

			if ( strcmp( cur.apName, "searchPaths" ) == 0 )
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

