#pragma once

constexpr u16         MAP_VERSION       = 1;
constexpr u16         CH_MAP_VERSION    = 2;
constexpr const char* CH_MAP_IDENTIFIER = "SDMF";

// #define CH_MAP_SIGNATURE ( 'F' << 24 | 'M' << 16 | 'D' << 8 | 'S' )
#define CH_MAP_SIGNATURE ( 'F' << 16 | 'M' << 8 | 'S' )

using MapHandle_t = size_t;

struct SceneDraw_t;
struct Renderable_t;


enum EMapHeaderFlag : u16
{
	EMapHeaderFlag_None              = 0,
	EMapHeaderFlag_Compressed        = ( 1 << 0 ),
	EMapHeaderFlag_ContainsVisData   = ( 1 << 1 ),
	EMapHeaderFlag_ContainsLightMaps = ( 1 << 2 ),
};


// Keep the number on these for potential backwards compatability
enum ESiduryMapCmd
{
	ESiduryMapCmd_Invalid       = 0,
	ESiduryMapCmd_EntityData    = 1,
	ESiduryMapCmd_ComponentData = 2,

	ESiduryMapCmd_Count,

	// ESiduryMapCmd_NewEntity,
	// ESiduryMapCmd_NewComponent,
};


// matches the names in the keyvalues file
struct MapInfo
{
	unsigned int version = 0;

	std::string  mapName;
	std::string  modelPath;

	glm::vec3    ang;

	glm::vec3    spawnPos;
	glm::vec3    spawnAng;

	// TODO: make a component
	std::string  skybox;
};


struct SiduryMapHeader_t
{
	u16            aVersion            = 0;
	int            aSignature          = 0;
	EMapHeaderFlag aFlags              = EMapHeaderFlag_None;
};


struct SiduryMap
{
	std::string                    aMapPath = "";
	MapHandle_t                    aMapHandle;  // TODO: USE THIS

	// Kept for Legacy Sidury Maps
	MapInfo*                       aMapInfo    = nullptr;

	ChHandle_t                     aRenderable = CH_INVALID_HANDLE;
};


bool                              MapManager_FindMap( const std::string& srPath );
bool                              MapManager_LoadMap( const std::string& srPath );
// SiduryMap*       MapManager_CreateMap();
void                              MapManager_WriteMap( const std::string& srPath );
void                              MapManager_CloseMap();

bool                              MapManager_HasMap();
std::string_view                  MapManager_GetMapName();
std::string_view                  MapManager_GetMapPath();

void                              MapManager_Update();

#if CH_CLIENT
#endif

MapInfo*                          MapManager_ParseMapInfo( const std::string& srPath );

// ------------------------------------------------------------------------
// Functions to become obsolete for Sidury Map Format Version 2

bool                              MapManager_LoadWorldModel();
// void            MapManager_ParseEntities( const std::string &path );

// glm::vec3        MapManager_GetSpawnPos();
// glm::vec3        MapManager_GetSpawnAng();
