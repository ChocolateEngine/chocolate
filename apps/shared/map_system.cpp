#include "map_system.h"
#include "core/json5.h"


using namespace chmap;


// returns true if the match failed
static bool CheckJsonType( JsonObject_t& object, EJsonType type )
{
	if ( object.aType == type )
		return false;

	Log_ErrorF( "Expected Type \"%s\" for key \"%s\", got \"%s\" instead\n", Json_TypeToStr( type ), object.name.data, Json_TypeToStr( object.aType ) );
	return true;
}


// // TODO: use ch_string instead of char*
// static void CopyString( char*& out, char* string, size_t stringLen )
// {
// 	out = ch_str_copy( string, stringLen ).data;
// }
// 
// 
// static void CopyString( char*& out, const char* string, size_t stringLen )
// {
// 	out = ch_str_copy( string, stringLen ).data;
// }


template< typename VECTOR >
static VECTOR JsonToVector( JsonObject_t& object )
{
	VECTOR vec{};
	if ( object.aObjects.aCount < vec.length() )
	{
		Log_ErrorF( "Not enough items in array to fill Vector with a size of %d: \"%s\"\n", object.name.data, vec.length() );
		return vec;
	}

	for ( int i = 0; i < object.aObjects.aCount; i++ )
	{
		if ( i == vec.length() )
			break;

		if ( object.aObjects.apData[ i ].aType == EJsonType_Int )
			vec[ i ] = object.aObjects.apData[ i ].aInt;

		else if ( object.aObjects.apData[ i ].aType == EJsonType_Double )
			vec[ i ] = object.aObjects.apData[ i ].aDouble;

		else
		{
			Log_ErrorF( "Invalid Json Type for Vector: \"%s\"\n", Json_TypeToStr( object.aObjects.apData[ i ].aType ) );
			return vec;
		}
	}

	return vec;
}


static void LoadComponent( Scene& scene, Entity& entity, JsonObject_t& cur )
{
	if ( CheckJsonType( cur, EJsonType_Object ) )
	{
		Log_Error( "Entity Component is not a Json Object type\n" );
		return;
	}

	// Check for component name
	if ( cur.name.data == nullptr )
	{
		Log_Error( "Entity Component does not have a name!\n" );
		return;
	}

	// Check if this component already exists
	for ( Component& component : entity.components )
	{
		if ( ch_str_equals( component.name, cur.name.data ) )
		{
			Log_ErrorF( "Entity already has a \"%s\" component\n", cur.name.data );
			return;
		}
	}

	Component& comp = entity.components.emplace_back();
	comp.name       = ch_str_copy( cur.name.data, cur.name.size );

	for ( u64 objI = 0; objI < cur.aObjects.aCount; objI++ )
	{
		JsonObject_t& object = cur.aObjects.apData[ objI ];

		// ComponentValue& value = comp.values.emplace_back();
		// CopyString( value.name, object.name.data, strlen( object.name.data ) );

		// TODO: why is this a hash map? just make it a simple struct array
		// I think the only reason was for easy matching to see if a component already exists
		auto it = comp.values.find( object.name.data );
		if ( it != comp.values.end() )
		{
			Log_ErrorF( "Entity already has a \"%s\" component value!\n", object.name.data );
			continue;
		}

		ComponentValue& value = comp.values[ object.name.data ];

		switch ( object.aType )
		{
			default:
				value.type = EComponentType_Invalid;
				break;

			case EJsonType_Double:
				value.type    = EComponentType_Double;
				value.aDouble = object.aDouble;
				break;

			case EJsonType_Int:
				value.type    = EComponentType_Int;
				value.aInteger = object.aInt;
				break;

			case EJsonType_String:
				value.type    = EComponentType_String;
				value.aString = ch_str_copy( object.aString.data, object.aString.size );
				break;

			case EJsonType_Array:
				if ( object.aObjects.aCount == 2 )
				{
					value.type  = EComponentType_Vec2;
					value.aVec2 = JsonToVector< glm::vec2 >( object );
				}
				else if ( object.aObjects.aCount == 3 )
				{
					value.type  = EComponentType_Vec3;
					value.aVec3 = JsonToVector< glm::vec3 >( object );
				}
				else if ( object.aObjects.aCount == 4 )
				{
					value.type  = EComponentType_Vec4;
					value.aVec4 = JsonToVector< glm::vec4 >( object );
				}
				else
				{
					Log_ErrorF( "Invalid Entity Component Vector Size: %d", object.aObjects.aCount );
				}
				break;
		}
	}
}


static void LoadEntity( Scene& scene, JsonObject_t& object )
{
	if ( CheckJsonType( object, EJsonType_Object ) )
	{
		Log_Error( "Entity is not a Json Object type\n" );
		return;
	}

	Entity& entity = scene.entites.emplace_back();

	for ( u64 objI = 0; objI < object.aObjects.aCount; objI++ )
	{
		JsonObject_t& cur = object.aObjects.apData[ objI ];

		if ( ch_str_equals( cur.name, "id", 2 ) )
		{
			if ( CheckJsonType( cur, EJsonType_Int ) )
				continue;

			entity.id = cur.aInt;
		}
		else if ( ch_str_equals( cur.name, "parent", 6 ) )
		{
			if ( CheckJsonType( cur, EJsonType_Int ) )
				continue;

			entity.parent = cur.aInt;
		}
		else if ( ch_str_equals( cur.name, "name", 4 ) )
		{
			if ( CheckJsonType( cur, EJsonType_String ) )
				continue;

			entity.name = ch_str_copy( cur.aString.data, cur.aString.size );
		}
		else if ( ch_str_equals( cur.name, "pos", 3 ) )
		{
			if ( CheckJsonType( cur, EJsonType_Array ) )
				continue;

			entity.pos = JsonToVector< glm::vec3 >( cur );
		}
		else if ( ch_str_equals( cur.name, "ang", 3 ) )
		{
			if ( CheckJsonType( cur, EJsonType_Array ) )
				continue;

			entity.ang = JsonToVector< glm::vec3 >( cur );
		}
		else if ( ch_str_equals( cur.name, "scale", 5 ) )
		{
			if ( CheckJsonType( cur, EJsonType_Array ) )
				continue;

			entity.scale = JsonToVector< glm::vec3 >( cur );
		}
		else if ( ch_str_equals( cur.name, "components", 10 ) )
		{
			if ( CheckJsonType( cur, EJsonType_Object ) )
				continue;

			for ( u64 compI = 0; compI < cur.aObjects.aCount; compI++ )
			{
				LoadComponent( scene, entity, cur.aObjects.apData[ compI ] );
			}
		}
	}
}


static void FreeScene( Scene& scene )
{
	for ( Entity& entity : scene.entites )
	{
		if ( entity.name.data )
			ch_str_free( entity.name.data );

		for ( Component& component : entity.components )
		{
			if ( component.name.data )
				ch_str_free( component.name.data );

			for ( auto& [key, value] : component.values )
			{
				if ( value.aString.data )
					ch_str_free( value.aString.data );
			}
		}
	}

	if ( scene.name.data )
		ch_str_free( scene.name.data );
}


static bool LoadScene( Map* map, const char* scenePath, s64 scenePathLen = -1 )
{
	ch_string_auto data = FileSys_ReadFile( scenePath, scenePathLen );

	if ( !data.data )
		return false;

	JsonObject_t root;
	EJsonError   err = Json_Parse( &root, data.data );

	if ( err != EJsonError_None )
	{
		Log_ErrorF( "Error Parsing Map Scene: %s\n", Json_ErrorToStr( err ) );
		return false;
	}

	ch_string_auto sceneName = FileSys_GetFileNameNoExt( scenePath, scenePathLen );

	Scene       scene;
	scene.name = ch_str_copy( sceneName.data, sceneName.size );

	for ( size_t i = 0; i < root.aObjects.aCount; i++ )
	{
		JsonObject_t& cur = root.aObjects.apData[ i ];

		if ( ch_str_equals( cur.name, "sceneFormatVersion", 18 ) )
		{
			if ( CheckJsonType( cur, EJsonType_Int ) )
			{
				Json_Free( &root );
				FreeScene( scene );
				return false;
			}

			scene.sceneFormatVersion = cur.aInt;

			if ( scene.sceneFormatVersion < CH_MAP_SCENE_VERSION )
			{
				Log_ErrorF( "Scene version mismatch, expected version %d, got version %d\n", CH_MAP_SCENE_VERSION, scene.sceneFormatVersion );
				Json_Free( &root );
				FreeScene( scene );
				return false;
			}
		}
		else if ( ch_str_equals( cur.name, "dateCreated", 11 ) )
		{
			if ( CheckJsonType( cur, EJsonType_Int ) )
				continue;

			scene.dateCreated = cur.aInt;
		}
		else if ( ch_str_equals( cur.name, "dateModified", 12 ) )
		{
			if ( CheckJsonType( cur, EJsonType_Int ) )
				continue;

			scene.dateModified = cur.aInt;
		}
		else if ( ch_str_equals( cur.name, "changeNumber", 12 ) )
		{
			if ( CheckJsonType( cur, EJsonType_Int ) )
				continue;

			scene.changeNumber = cur.aInt;
		}
		else if ( ch_str_equals( cur.name, "entities", 8 ) )
		{
			if ( CheckJsonType( cur, EJsonType_Array ) )
				continue;

			for ( u64 objI = 0; objI < cur.aObjects.aCount; objI++ )
			{
				LoadEntity( scene, cur.aObjects.apData[ objI ] );
			}
		}
	}

	map->scenes.push_back( scene );
	return true;
}


Map* chmap::Load( const char* path, u64 pathLen )
{
	// Load mapInfo.json5 to kick things off
	const char*    strings[]      = { path, PATH_SEP_STR "mapInfo.json5" };
	const size_t   sizes[]        = { pathLen, 16 };
	ch_string_auto mapInfoPath    = ch_str_join( 2, strings, sizes );

	ch_string_auto absMapInfoPath = FileSys_FindFile( mapInfoPath.data, mapInfoPath.size );

	if ( !absMapInfoPath.data )
	{
		Log_ErrorF( "No mapInfo.json5 file in map: \"%s\"\n", path );
		return nullptr;
	}
	
	ch_string_auto data = FileSys_ReadFile( mapInfoPath.data, mapInfoPath.size );

	if ( !data.data )
		return nullptr;

	JsonObject_t root;
	EJsonError   err = Json_Parse( &root, data.data );

	if ( err != EJsonError_None )
	{
		Log_ErrorF( "Error Parsing Map: %s\n", Json_ErrorToStr( err ) );
		return nullptr;
	}

	Map* map = new Map;

	ch_string primaryScene;

	for ( size_t i = 0; i < root.aObjects.aCount; i++ )
	{
		JsonObject_t& cur = root.aObjects.apData[ i ];

		if ( ch_str_equals( cur.name, "version", 7 ) )
		{
			if ( CheckJsonType( cur, EJsonType_Int ) )
			{
				Json_Free( &root );
				Free( map );
				return nullptr;
			}

			map->version = cur.aInt;

			if ( map->version < CH_MAP_VERSION )
			{
				Log_ErrorF( "Map version mismatch, expected version %d, got version %d\n", CH_MAP_VERSION, map->version );
				Json_Free( &root );
				Free( map );
				return nullptr;
			}
		}
		else if ( ch_str_equals( cur.name, "name", 4 ) )
		{
			if ( CheckJsonType( cur, EJsonType_String ) )
				continue;

			if ( map->name.data )
			{
				Log_ErrorF( "Duplicate Name Entry in map: \"%s\"\n", path );
				ch_str_free( map->name.data );
			}

			map->name = ch_str_copy( cur.aString.data, cur.aString.size );
		}
		else if ( ch_str_equals( cur.name, "primaryScene", 12 ) )
		{
			if ( CheckJsonType( cur, EJsonType_String ) )
				continue;

			primaryScene = cur.aString;
		}
		else if ( ch_str_equals( cur.name, "skybox", 6 ) )
		{
			if ( CheckJsonType( cur, EJsonType_String ) )
				continue;

			map->skybox = ch_str_copy( cur.aString.data, cur.aString.size ).data;
		}
		else
		{
			Log_WarnF( "Unknown Key in map info: \"%s\"\n", cur.name.data );
		}
	}

	Json_Free( &root );

	// default name of unnamed map
	if ( map->name.data == nullptr )
	{
		map->name = ch_str_copy( "unnamed map", strlen( "unnamed map" ) );
	}

	if ( map->version == 0 || map->version == UINT32_MAX )
	{
		Log_ErrorF( "Map Version not specified for map \"%s\"\n", path );
		Free( map );
		return nullptr;
	}

	// Load Scenes
	const char*              scenesStr[]   = { path, PATH_SEP_STR "scenes" };
	const size_t             scenesSizes[] = { pathLen, 7 };
	ch_string                scenesDir     = ch_str_join( 2, scenesStr, scenesSizes );

	std::vector< ch_string > scenePaths = FileSys_ScanDir( scenesDir.data, scenesDir.size, ReadDir_NoDirs | ReadDir_Recursive );

	for ( const ch_string& scenePath : scenePaths )
	{
		if ( !ch_str_ends_with( scenePath, ".json5", 6 ) )
			continue;

		const ch_string strings[]     = { scenesDir, scenePath };
		ch_string_auto  scenePathFull = ch_str_join( 2, strings );

		if ( !LoadScene( map, scenePathFull.data ) )	
		{
			Log_ErrorF( "Failed to load map scene: Map \"%s\" - Scene \"%s\"\n", path, scenePath.data );
		}
	}

	ch_str_free( scenePaths.data(), scenePaths.size() );

	if ( map->scenes.empty() )
	{
		Log_ErrorF( "No Scenes Loaded in map: \"%s\"\n", path );
		Free( map );
		return nullptr;
	}

	// Select Primary Scene
	u32 sceneIndex = 0;
	for ( Scene& scene : map->scenes )
	{
		if ( primaryScene == scene.name )
		{
			map->primaryScene = sceneIndex;
			break;
		}

		sceneIndex++;
	}

	if ( map->primaryScene == UINT32_MAX )
	{
		Log_ErrorF( "No Primary Scene specified for map, defaulting to scene 0: \"%s\"\n", path );
		map->primaryScene = 0;
	}

	return map;
}


void chmap::Free( Map* map )
{
	if ( map->name.data )
		ch_str_free( map->name.data );

	if ( map->skybox )
		ch_str_free( map->skybox );

	for ( Scene& scene : map->scenes )
	{
		FreeScene( scene );
	}

	delete map;
}

