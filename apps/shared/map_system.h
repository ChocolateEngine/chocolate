#pragma once

// ======================================================================================================
// The Chocolate Engine Map Loader/Writer
// 
// The map format is setup to be a folder that contains a main map info file
// Each map contains multiple "scenes", and a primary scene is selected to be loaded into for the game
// This file should stand on it's own and be shared for use between the game and the editor
// ======================================================================================================

#include "core/core.h"

namespace chmap
{

constexpr u32 CH_MAP_VERSION       = 1;
constexpr u32 CH_MAP_SCENE_VERSION = 1;


// Component Data for an entity
enum EComponentType
{
	EComponentType_Invalid,
	EComponentType_String,
	EComponentType_Int,
	EComponentType_Double,
	EComponentType_Vec2,
	EComponentType_Vec3,
	EComponentType_Vec4,

	// nested data, like material overrides for a model on a renderable
	// EComponentType_Objects,
	// EComponentType_Array,
};


struct ComponentValue
{
	// char*          name = nullptr;
	EComponentType type = EComponentType_Invalid;

	union
	{
		struct Array_t
		{
			ComponentValue* data;
			u64             size;
		} aObjects;

		int64_t                       aInteger;
		double                        aDouble;
		ch_string                     aString;
		glm::vec2                     aVec2;
		glm::vec3                     aVec3;
		glm::vec4                     aVec4;
	};

	ComponentValue()
	{
	}

	ComponentValue( const ComponentValue& other )
	{
		memcpy( this, &other, sizeof( ComponentValue ) );
	}

	~ComponentValue()
	{
	}
};


struct Component
{
	ch_string                                              name;
	// std::vector< ComponentValue >                     values{};
	std::unordered_map< std::string_view, ComponentValue > values{};
};


// An Entity in a scene
struct Entity
{
	u64                      id     = UINT32_MAX;
	u64                      parent = UINT32_MAX;  // Optional
	ch_string                name;     // Optional

	glm::vec3                pos{};
	glm::vec3                ang{};
	glm::vec3                scale{};

	std::vector< Component > components{};
};


// Scene placed in another scene, sort of as a "prefab"
struct NestedScene
{
	u32       index = UINT32_MAX;  // scene index into the scenes list in the Map struct

	glm::vec3 pos{};
	glm::vec3 ang{};
	glm::vec3 scale{};
};


struct Scene
{
	ch_string                  name;
	u32                        sceneFormatVersion = UINT32_MAX;

	u64                        dateCreated        = 0;
	u64                        dateModified       = 0;
	u32                        changeNumber       = 0;

	std::vector< NestedScene > nestedScenes{};
	std::vector< Entity >      entites{};
};


// Data for the editor
struct Config
{
	std::vector< fs::path > sourceAssetSearchPaths;
	fs::path                assetExportPath;
};


// The main struct containing all data
struct Map
{
	u32                  version      = UINT32_MAX;
	ch_string            name;
	char*                skybox       = nullptr;    // MAY REMOVE ONE DAY IF WE GO WITH A SKYBOX ENTITY COMPONENT

	u32                  primaryScene = UINT32_MAX;  // index of the default scene to load
	std::vector< Scene > scenes{};
};


Map*    Load( const char* path, u64 pathLen );
void    Free( Map* map );
Map*    Create();
bool    Save( Map* map );

Scene*  CreateScene( Map* map );
void    DestroyScene( Map* map, Scene* scene );

Entity* GetEntityParent( Entity& entity );

}
