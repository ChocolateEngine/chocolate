// Loads Sidury Map Files (smf)

#include "core/filesystem.h"
#include "core/console.h"
#include "util.h"
#include "main.h"
#include "game_shared.h"
#include "igraphics.h"
#include "map_manager.h"

#if CH_SERVER
	#include "../server/sv_main.h"
#endif

#include "speedykeyv/KeyValue.h"

#include <filesystem>


LOG_REGISTER_CHANNEL2( Map, LogColor::DarkGreen );

SiduryMap* gpMap = nullptr;


void MapManager_Update()
{
}


void MapManager_CloseMap()
{
	if ( gpMap == nullptr )
		return;

	// for ( Entity entity : gpMap->aMapEntities )
	// {
	// 	Entity_DeleteEntity( entity );
	// }
	// 
	// gpMap->aMapEntities.clear();

	delete gpMap;
	gpMap = nullptr;
}


bool MapManager_FindMap( const std::string& path )
{
	std::string absPath = FileSys_FindDir( FileSys_IsAbsolute( path.c_str() ) ? path : "maps/" + path );

	if ( absPath == "" )
		return false;

	return true;
}


// Temporary, will be removed
bool MapManager_LoadLegacyV1Map( const std::string &path )
{
	MapInfo* mapInfo = MapManager_ParseMapInfo( path + "/mapInfo.smf" );

	if ( mapInfo == nullptr )
		return false;

	gpMap           = new SiduryMap;
	gpMap->aMapPath = path;
	gpMap->aMapInfo = mapInfo;

#if CH_CLIENT
	// Skybox_SetMaterial( mapInfo->skybox );
#endif

	if ( !MapManager_LoadWorldModel() )
	{
		MapManager_CloseMap();
		return false;
	}

	return true;
}


bool MapManager_LoadMap( const std::string &path )
{
	if ( gpMap )
		MapManager_CloseMap();

	std::string absPath = FileSys_FindDir( FileSys_IsAbsolute( path.c_str() ) ? path : "maps/" + path );

	if ( absPath.empty() )
	{
		Log_WarnF( gLC_Map, "Map does not exist: \"%s\"", path.c_str() );
		return false;
	}

	Log_DevF( gLC_Map, 1, "Loading Map: %s\n", path.c_str() );

	std::string mapInfoPath = FileSys_FindFile( absPath + "/mapInfo.smf" );
	if ( FileSys_FindFile( absPath + "/mapInfo.smf" ).size() )
	{
		// It's a Legacy Map
		return MapManager_LoadLegacyV1Map( absPath );
	}

	return false;
}


bool MapManager_HasMap()
{
	return gpMap != nullptr;
}


std::string_view MapManager_GetMapName()
{
	if ( !gpMap )
		return "";

	return gpMap->aMapInfo->mapName;
}


std::string_view MapManager_GetMapPath()
{
	if ( !gpMap )
		return "";

	return gpMap->aMapPath;
}


bool MapManager_LoadWorldModel()
{
	ChHandle_t model = graphics->LoadModel( gpMap->aMapInfo->modelPath );

	if ( model == CH_INVALID_HANDLE )
	{
		Log_ErrorF( gLC_Map, "Failed to load world model\n" );
		return false;
	}

	ChHandle_t renderHandle = graphics->CreateRenderable( model );

	if ( renderHandle == CH_INVALID_HANDLE )
	{
		Log_ErrorF( gLC_Map, "Failed to create renderable for world model\n" );
		return false;
	}

	Renderable_t* renderable = graphics->GetRenderableData( renderHandle );
	renderable->aModelMatrix = Util_ToMatrix( nullptr, &gpMap->aMapInfo->ang, nullptr );
	gpMap->aRenderable       = renderHandle;

	return true;
}


// here so we don't need calculate sizes all the time for string comparing
struct MapInfoKeys
{
	std::string version   = "version";
	std::string mapName   = "mapName";
	std::string modelPath = "modelPath";
	std::string ang       = "ang";
	std::string physAng   = "physAng";
	std::string skybox    = "skybox";
	std::string spawnPos  = "spawnPos";
	std::string spawnAng  = "spawnAng";
}
gMapInfoKeys;


MapInfo *MapManager_ParseMapInfo( const std::string &path )
{
	if ( !FileSys_Exists( path ) )
	{
		Log_WarnF( gLC_Map, "Map Info does not exist: \"%s\"", path.c_str() );
		return nullptr;
	}

	std::vector< char > rawData = FileSys_ReadFile( path );

	if ( rawData.empty() )
	{
		Log_WarnF( gLC_Map, "Failed to read file: %s\n", path.c_str() );
		return nullptr;
	}

	// append a null terminator for c strings
	rawData.push_back( '\0' );

	KeyValueRoot kvRoot;
	KeyValueErrorCode err = kvRoot.Parse( rawData.data() );

	if ( err != KeyValueErrorCode::NO_ERROR )
	{
		Log_WarnF( gLC_Map, "Failed to parse file: %s\n", path.c_str() );
		return nullptr;
	}

	// parsing time
	kvRoot.Solidify();

	KeyValue *kvShader = kvRoot.children;

	MapInfo *mapInfo = new MapInfo;

	KeyValue *kv = kvShader->children;
	for ( int i = 0; i < kvShader->childCount; i++ )
	{
		if ( kv->hasChildren )
		{
			Log_MsgF( gLC_Map, "Skipping extra children in kv file: %s", path.c_str() );
			kv = kv->next;
			continue;
		}

		else if ( gMapInfoKeys.version == kv->key.string )
		{
			mapInfo->version = ToLong( kv->value.string, 0 );
			if ( mapInfo->version != MAP_VERSION )
			{
				Log_ErrorF( gLC_Map, "Invalid Version: %ud - Expected Version %ud\n", mapInfo->version, MAP_VERSION );
				delete mapInfo;
				return nullptr;
			}
		}

		else if ( gMapInfoKeys.mapName == kv->key.string )
		{
			mapInfo->mapName = kv->value.string;
		}

		else if ( gMapInfoKeys.modelPath == kv->key.string )
		{
			mapInfo->modelPath = kv->value.string;

			if ( mapInfo->modelPath == "" )
			{
				Log_WarnF( gLC_Map, "Empty Model Path for map \"%s\"\n", path.c_str() );
				delete mapInfo;
				return nullptr;
			}
		}

		else if ( gMapInfoKeys.skybox == kv->key.string )
			mapInfo->skybox = kv->value.string;

		// we skip this one cause it was made with incorrect matrix rotations
		// else if ( gMapInfoKeys.ang == kv->key.string )
		// 	continue;

		// physAng contains the correct values, so we only use that now
		else if ( gMapInfoKeys.physAng == kv->key.string )
			mapInfo->ang = KV_GetVec3( kv->value.string );

		else if ( gMapInfoKeys.spawnPos == kv->key.string )
			mapInfo->spawnPos = KV_GetVec3( kv->value.string );

		else if ( gMapInfoKeys.spawnAng == kv->key.string )
			mapInfo->spawnAng = KV_GetVec3( kv->value.string );

		kv = kv->next;
	}

	return mapInfo;
}

