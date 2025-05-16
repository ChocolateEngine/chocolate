// Loads Sidury Map Files (smf)

#include "mapmanager.h"

#include <filesystem>

#include "core/console.h"
#include "core/filesystem.h"
#include "core/util.h"
#include "igraphics.h"
#include "main.h"
#include "map_system.h"
#include "skybox.h"


LOG_CHANNEL_REGISTER( Map, ELogColor_DarkGreen );

std::vector< std::string > gMapList;
static bool                gRebuildMapList  = true;
static float               gRebuildMapTimer = 0.f;


CONVAR_FLOAT( map_list_rebuild_timer, 30.f, CVARF_ARCHIVE, "Timer for rebuilding the map list" );


CONCMD_VA( map_list_rebuild, "Rebuild the map list now" )
{
	gRebuildMapList = true;
}


void map_dropdown(
  const std::vector< std::string >& args,         // arguments currently typed in by the user
  const std::string&                fullCommand,  // the full command line the user has typed in
  std::vector< std::string >&       results )           // results to populate the dropdown list with
{
	const std::vector< std::string >& mapList = MapManager_GetMapList();

	for ( const auto& map : mapList )
	{
		if ( args.size() && !map.starts_with( args[ 0 ] ) )
			continue;

		results.push_back( map );
	}
}


CONCMD_DROP( map, map_dropdown )
{
	if ( args.size() == 0 )
	{
		Log_Warn( gLC_Map, "No Map Path/Name specified!\n" );
		return;
	}

	if ( !MapManager_FindMap( args[ 0 ] ) )
	{
		Log_Error( "Failed to Find map\n" );
		return;
	}

	if ( !MapManager_LoadMap( args[ 0 ] ) )
	{
		Log_Error( "Failed to Load map\n" );
		return;
	}
}


CONCMD( map_save )
{
	EditorContext_t* ctx = Editor_GetContext();

	if ( ctx == nullptr )
	{
		Log_Error( "No Map Open!\n" );
		return;
	}

	if ( args.size() )
	{
		MapManager_WriteMap( ctx->aMap, args[ 0 ] );
		return;
	}

	MapManager_WriteMap( ctx->aMap, ctx->aMap.aMapPath );
}


void MapManager_Update()
{
	if ( gRebuildMapTimer > 0.f )
		gRebuildMapTimer -= gFrameTime;
	else
		gRebuildMapList = true;
}


void MapManager_RebuildMapList()
{
	gMapList.clear();

	std::vector< ch_string > mapFolders = FileSys_ScanDir( "maps", ReadDir_AllPaths | ReadDir_NoFiles );

	for ( const auto& mapFolder : mapFolders )
	{
		if ( ch_str_ends_with( mapFolder, "..", 2 ) )
			continue;

		// Check for legacy map file and new map file
		const char*    strings[]     = { mapFolder.data, CH_PATH_SEP_STR "mapInfo.smf" };
		const u64      stringLens[]  = { mapFolder.size, 14 };
		ch_string_auto mapInfoPath   = ch_str_join( 2, strings, stringLens );

		const char*    strings2[]    = { mapFolder.data, CH_PATH_SEP_STR "mapData.smf" };
		const u64      stringLens2[] = { mapFolder.size, 14 };
		ch_string_auto mapDataPath   = ch_str_join( 2, strings2, stringLens2 );

		if ( !FileSys_IsFile( mapInfoPath.data, mapInfoPath.size, true ) && !FileSys_IsFile( mapDataPath.data, mapDataPath.size, true ) )
		{
			continue;
		}

		ch_string_auto mapName = FileSys_GetFileName( mapFolder.data, mapFolder.size );
		gMapList.emplace_back( mapName.data, mapName.size );
	}

	ch_str_free( mapFolders.data(), mapFolders.size() );
}


const std::vector< std::string >& MapManager_GetMapList()
{
	if ( gRebuildMapList )
	{
		MapManager_RebuildMapList();
		gRebuildMapList  = false;
		gRebuildMapTimer = map_list_rebuild_timer;
	}

	return gMapList;
}


bool MapManager_FindMap( const std::string& path )
{
	ch_string mapPath;

	if ( FileSys_IsAbsolute( path.c_str() ) )
	{
		mapPath = ch_str_copy( path.data(), path.size() );
	}
	else
	{
		const char* strings[] = { "maps/", path.c_str() };
		const u64   lengths[] = { 5, path.size() };
		mapPath               = ch_str_join( 2, strings, lengths );
	}

	ch_string absPath = FileSys_FindDir( mapPath.data, mapPath.size );

	bool      found   = absPath.data != nullptr;

	ch_str_free( mapPath.data );
	ch_str_free( absPath.data );

	return found;
}


static bool MapManager_LoadScene( chmap::Scene& scene )
{
	EditorContext_t* context = nullptr;
	ch_handle_t      handle  = Editor_CreateContext( &context );

	if ( handle == CH_INVALID_HANDLE )
		return false;

	std::unordered_map< u32, ch_handle_t > entityHandles;

	for ( chmap::Entity& mapEntity : scene.entites )
	{
		ch_handle_t entHandle = Entity_Create();

		if ( entHandle == CH_INVALID_HANDLE )
		{
			Editor_FreeContext( handle );
			return false;
		}

		Entity_SetName( entHandle, mapEntity.name.data, mapEntity.name.size );

		Entity_t* ent                 = Entity_GetData( entHandle );

		entityHandles[ mapEntity.id ] = entHandle;

		ent->aTransform.aPos          = mapEntity.pos;
		ent->aTransform.aAng          = mapEntity.ang;
		ent->aTransform.aScale        = mapEntity.scale;

		// Check Built in components (TODO: IMPROVE THIS)
		for ( chmap::Component& comp : mapEntity.components )
		{
			// Load a renderable
			// if ( ch_str_equals( comp.name, "renderable", 10 ) )
			if ( CH_STR_EQUALS_STATIC( comp.name, "renderable" ) )
			{
				auto it = comp.values.find( "path" );
				if ( it == comp.values.end() )
				{
					Log_Error( gLC_Map, "Failed to find renderable model path in component\n" );
					continue;
				}

				if ( it->second.type != chmap::EComponentType_String )
					continue;

				ent->aModel = graphics->LoadModel( it->second.aString.data );

				if ( ent->aModel == CH_INVALID_HANDLE )
					continue;

				ent->aRenderable = graphics->CreateRenderable( ent->aModel );

				// Load other renderable data
				// for ( const auto& [ name, compValue ] : comp.values )
				// {
				// }
			}
			else if ( ch_str_equals( comp.name, "light", 5 ) )
			{
				auto it = comp.values.find( "type" );
				if ( it == comp.values.end() )
				{
					Log_Error( gLC_Map, "Failed to find light type in component\n" );
					continue;
				}

				if ( it->second.type != chmap::EComponentType_String )
					continue;

				if ( ch_str_equals( it->second.aString, "world", 5 ) )
				{
					ent->apLight = graphics->CreateLight( ELightType_World );
				}
				else if ( ch_str_equals( it->second.aString, "point", 5 ) )
				{
					ent->apLight = graphics->CreateLight( ELightType_Point );
				}
				else if ( ch_str_equals( it->second.aString, "spot", 4 ) )
				{
					ent->apLight            = graphics->CreateLight( ELightType_Spot );

					// ???
					ent->apLight->aInnerFov = 0.f;
				}
				// else if ( ch_str_equals( it->second.apString, lightTypeLen, "capsule", 7 ) )
				// {
				// 	ent->apLight = graphics->CreateLight( ELightType_Capsule );
				// }
				else
				{
					Log_ErrorF( gLC_Map, "Unknown Light Type: %s\n", it->second.aString.data );
					continue;
				}

				// Read the rest of the light data
				for ( const auto& [ name, compValue ] : comp.values )
				{
					if ( ch_str_equals( name.data(), name.size(), "color", 5 ) )
					{
						if ( compValue.type != chmap::EComponentType_Vec4 )
							continue;

						ent->apLight->color = compValue.aVec4;
					}
					else if ( ch_str_equals( name.data(), name.size(), "radius", 6 ) )
					{
						if ( compValue.type == chmap::EComponentType_Int )
							ent->apLight->aRadius = compValue.aInteger;

						else if ( compValue.type == chmap::EComponentType_Double )
							ent->apLight->aRadius = compValue.aDouble;
					}
				}
			}
			else if ( ch_str_equals( comp.name, "phys_object", 11 ) )
			{
				auto it = comp.values.find( "path" );
				if ( it == comp.values.end() )
				{
					Log_Error( gLC_Map, "Failed to find physics object path in component\n" );
					continue;
				}

				auto itType = comp.values.find( "type" );
				if ( itType == comp.values.end() )
				{
					Log_Error( gLC_Map, "Failed to find physics object type in component\n" );
					continue;
				}

				PhysShapeType     shapeType;
				PhysicsObjectInfo settings{};

				if ( ch_str_equals( itType->second.aString, "convex", 6 ) )
				{
					shapeType            = PhysShapeType::Convex;
					settings.aMotionType = PhysMotionType::Dynamic;
				}
				else if ( ch_str_equals( itType->second.aString, "static_compound", 15 ) )
				{
					shapeType            = PhysShapeType::StaticCompound;
					settings.aMotionType = PhysMotionType::Dynamic;
				}
				else if ( ch_str_equals( itType->second.aString, "mesh", 4 ) )
				{
					shapeType            = PhysShapeType::Mesh;
					settings.aMotionType = PhysMotionType::Static;
				}
				else
				{
					shapeType            = PhysShapeType::Convex;
					settings.aMotionType = PhysMotionType::Dynamic;
				}


				IPhysicsShape* shape = GetPhysEnv()->LoadShape( it->second.aString.data, it->second.aString.size, shapeType );

				if ( !shape )
					continue;

				settings.aPos         = context->aView.aPos;
				settings.aAng         = context->aView.aAng;
				settings.aStartActive = true;
				settings.aCustomMass  = true;
				settings.aMass        = 10.f;

				ent->apPhysicsObject  = GetPhysEnv()->CreateObject( shape, settings );
			}
		}
	}

	// Check entity parents
	for ( chmap::Entity& mapEntity : scene.entites )
	{
		if ( mapEntity.parent == UINT32_MAX )
			continue;

		auto itID     = entityHandles.find( mapEntity.id );
		auto itParent = entityHandles.find( mapEntity.parent );

		if ( itID == entityHandles.end() || itParent == entityHandles.end() )
		{
			Log_ErrorF( "Failed to parent entity %d", mapEntity.id );
			continue;
		}

		Entity_SetParent( itID->second, itParent->second );
	}
}


bool MapManager_LoadMap( const std::string& path )
{
	ch_string_auto mapPath;

	if ( FileSys_IsAbsolute( path.c_str() ) )
	{
		mapPath = ch_str_copy( path.data(), path.size() );
	}
	else
	{
		const char* strings[] = { "maps/", path.c_str() };
		const u64   lengths[] = { 5, path.size() };
		mapPath               = ch_str_join( 2, strings, lengths );
	}

	ch_string_auto absPath = FileSys_FindDir( mapPath.data, mapPath.size );

	if ( !absPath.data )
	{
		Log_WarnF( gLC_Map, "Map does not exist: \"%s\"\n", path.c_str() );
		return false;
	}

	chmap::Map* map = chmap::Load( absPath.data, absPath.size );

	if ( map == nullptr )
	{
		Log_ErrorF( gLC_Map, "Failed to Load Map: \"%s\"\n", path.c_str() );
		return false;
	}

	// Set New Search Path
	FileSys_InsertSearchPath( 0, absPath.data, absPath.size );

	// Only load the primary scene for now
	// Each scene gets it's own editor context
	// TODO: make an editor project system
	if ( !MapManager_LoadScene( map->scenes[ map->primaryScene ] ) )
	{
		Log_ErrorF( gLC_Map, "Failed to Load Primary Scene: \"%s\" - Scene \"%s\"\n", path.c_str(), map->scenes[ map->primaryScene ].name );
		FileSys_RemoveSearchPath( absPath.data, absPath.size );
		return false;
	}

	if ( map->skybox )
	{
		Editor_GetContext()->aMap.aSkybox = map->skybox;
	}

	Editor_SetContext( gEditorContextIdx );

	return true;
}


void MapManager_WriteMap( SiduryMap& map, const std::string& srPath )
{
	// Must be in a map to save it
	//if ( !Game_InMap() )
	return;

	std::string basePath = srPath;
	if ( FileSys_IsRelative( srPath.c_str() ) )
	{
		basePath = std::filesystem::current_path().string() + "/maps/" + srPath;
	}

	// Find an Empty Filename/Path to use so we don't overwrite anything
	std::string outPath       = basePath;
	size_t      renameCounter = 0;
	// while ( FileSys_Exists( outPath ) )
	// {
	// 	outPath = vstring( "%s_%zd", basePath.c_str(), ++renameCounter );
	// }

	// Open the file handle
	if ( !std::filesystem::create_directories( outPath ) )
	{
		Log_ErrorF( gLC_Map, "Failed to create directory for map: %s\n", outPath.c_str() );
		return;
	}

	std::string mapDataPath = outPath + "/mapData.smf";

	// FileSys_SaveFile( mapDataPath, {} );

	// Write the data
	FILE*       fp          = fopen( mapDataPath.c_str(), "wb" );

	if ( fp == nullptr )
	{
		Log_ErrorF( gLC_Map, "Failed to open file handle for mapData.smf: \"%s\"\n", mapDataPath.c_str() );
		return;
	}

	// Write Map Header
	SiduryMapHeader_t mapHeader;
	mapHeader.aVersion   = CH_MAP_VERSION;
	mapHeader.aSignature = CH_MAP_SIGNATURE;
	fwrite( &mapHeader, sizeof( SiduryMapHeader_t ), 1, fp );

	// Write the commands
	// fwrite( outBuffer.c_str(), sizeof( char ), outBuffer.size(), fp );
	fclose( fp );

	// If we had to use a custom name for it, rename the old file, and rename the new file to what we wanted to save it as
	if ( renameCounter > 0 )
	{
	}
}
