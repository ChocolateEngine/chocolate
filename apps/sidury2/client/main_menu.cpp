#include "cl_main.h"
#include "main.h"

#include "imgui/imgui.h"


// =======================================================================
// Main Menu
// =======================================================================


std::vector< std::string > gMapList;
static bool                gRebuildMapList  = true;
static float               gRebuildMapTimer = 0.f;

static bool                gClientMenuShown = true;

CONVAR( cl_map_list_rebuild_timer, 30.f, CVARF_ARCHIVE, "Timer for rebuilding the map list" );

ConVarRef snd_volume( "snd_volume" );


CONCMD_VA( cl_map_list_rebuild, "Rebuild the map list now" )
{
	gRebuildMapList = true;
}


void CenterMouseOnScreen();


bool CL_IsMenuShown()
{
	return gClientMenuShown;
}


void CL_UpdateMenuShown()
{
	bool wasShown    = gClientMenuShown;
	gClientMenuShown = gui->IsConsoleShown();

	if ( wasShown != gClientMenuShown )
	{
		SDL_SetRelativeMouseMode( (SDL_bool)!gClientMenuShown );

		if ( gClientMenuShown )
		{
			CenterMouseOnScreen();
		}
	}
}


void CL_DrawMainMenu()
{
	PROF_SCOPE();

	if ( !ImGui::Begin( "Chocolate Engine - Sidury" ) )
	{
		ImGui::End();
		return;
	}

	if ( ImGui::Button( "connect localhost" ) )
	{
		Con_QueueCommand( "connect localhost" );
	}

	if ( gRebuildMapTimer > 0.f )
		gRebuildMapTimer -= gFrameTime;
	else
		gRebuildMapList = true;

	if ( gRebuildMapList )
	{
		gMapList.clear();

		for ( const auto& mapFolder : FileSys_ScanDir( "maps", ReadDir_AllPaths | ReadDir_NoFiles ) )
		{
			if ( mapFolder.ends_with( ".." ) )
				continue;

			// Check for legacy map file and new map file
			if ( !FileSys_IsFile( mapFolder + "/mapInfo.smf", true ) && !FileSys_IsFile( mapFolder + "/mapData.smf", true ) )
				continue;

			std::string mapName = FileSys_GetFileName( mapFolder );
			gMapList.push_back( mapName );
		}

		gRebuildMapList  = false;
		gRebuildMapTimer = cl_map_list_rebuild_timer;
	}

	ImGui::Separator();

	const ImVec2      itemSize   = ImGui::GetItemRectSize();
	const float       textHeight = ImGui::GetTextLineHeight();
	const ImGuiStyle& style      = ImGui::GetStyle();

	// Quick Map List
	{
		// ImGui::Text( "Quick Map List" );

		ImVec2 mapChildSize = itemSize;
		mapChildSize.x -= ( style.FramePadding.x + style.ItemInnerSpacing.x ) * 2;
		mapChildSize.y += ( style.FramePadding.y + style.ItemInnerSpacing.y );
		mapChildSize.y += textHeight * 8.f;

		if ( ImGui::BeginChild( "Quick Map List", mapChildSize, true ) )
		{
			for ( const std::string& map : gMapList )
			{
				if ( ImGui::Selectable( map.c_str() ) )
				{
					Con_QueueCommand( vstring( "map \"%s\"", map.c_str() ) );
				}
			}
		}

		ImGui::EndChild();
	}

	// Quick Settings
	{
		ImVec2 childSize = itemSize;
		childSize.x -= ( style.FramePadding.x + style.ItemInnerSpacing.x ) * 2;
		childSize.y += ( style.FramePadding.y + style.ItemInnerSpacing.y );
		childSize.y += textHeight * 2.f;

		// if ( ImGui::BeginChild( "Quick Settings", childSize, true ) )
		if ( ImGui::BeginChild( "Quick Settings", {}, true ) )
		{
			float vol = snd_volume.GetFloat();
			if ( ImGui::SliderFloat( "Volume", &vol, 0.f, 1.f ) )
			{
				snd_volume.SetValue( vol );
			}
		}

		ImGui::EndChild();
	}

	ImGui::End();
}
