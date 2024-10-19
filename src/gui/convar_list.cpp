#include "gui.h"
#include "util.h"

#include "core/core.h"

#include "imgui/imgui.h"
#include "imgui/imgui_impl_sdl2.h"


static int                        gCmdDropDownIndex = -1;
static int                        gCmdHistoryIndex  = -1;

static std::string                gCmdUserInput     = "";
static std::vector< std::string > gCmdAutoComplete;
static int                        gCmdUserCursor = 0;


// NOTE: all these colors are just kinda randomly picked,
// change if you think a certain color is bad
constexpr ImVec4 ToImCol( ELogColor col )
{
	switch (col)
	{
		// hmm
		case ELogColor_Black:
			return {0.3, 0.3, 0.3, 1};
		case ELogColor_White:
			return {1, 1, 1, 1};

		case ELogColor_DarkBlue:
			return {0, 0.3, 0.8, 1};
		case ELogColor_DarkGreen:
			return {0.25, 0.57, 0.25, 1};
		case ELogColor_DarkCyan:
			return {0, 0.35, 0.75, 1};
		case ELogColor_DarkRed:
			return {0.7, 0, 0.25, 1};
		case ELogColor_DarkPurple:
			return {0.45, 0, 0.7, 1};
		case ELogColor_DarkYellow:
			return {0.6, 0.6, 0, 1};
		case ELogColor_DarkGray:
			return {0.45, 0.45, 0.45, 1};

		case ELogColor_Blue:
			return {0, 0.4, 1, 1};
		case ELogColor_Green:
			return {0.4, 0.9, 0.4, 1};
		case ELogColor_Cyan:
			return {0, 0.85, 1, 1};
		case ELogColor_Red:
			return {0.9, 0, 0.4, 1};
		case ELogColor_Purple:
			return {0.6, 0, 0.9, 1};
		case ELogColor_Yellow:
			return {1, 1, 0, 1};
		case ELogColor_Gray:
			return {0.7, 0.7, 0.7, 1};

		case ELogColor_Default:
		default:
			return ImGui::GetStyleColorVec4( ImGuiCol_Text );
	}
}


constexpr const char* ToImHex( ELogColor col )
{
	switch ( col )
	{
		case ELogColor_Black:
			return "\0334C4C4CFFm";
		case ELogColor_White:
			return "\033FFFFFFFFm";

		case ELogColor_DarkBlue:
			return "\033004CCCFF";
		case ELogColor_DarkGreen:
			return "\033409140FFm";
		case ELogColor_DarkCyan:
			return "\0330059BFFFm";
		case ELogColor_DarkRed:
			return "\033B30040FFm";
		case ELogColor_DarkPurple:
			return "\0334300B3FFm";
		case ELogColor_DarkYellow:
			return "\033999900FFm";
		case ELogColor_DarkGray:
			return "\033737373FFm";

		case ELogColor_Blue:
			return "\0330066FFFFm";
		case ELogColor_Green:
			return "\03366E666FFm";
		case ELogColor_Cyan:
			return "\03300D9FFFFm";
		case ELogColor_Red:
			return "\033E60066FFm";
		case ELogColor_Purple:
			return "\0339900E6FFm";
		case ELogColor_Yellow:
			return "\033FFFF00FFm";
		case ELogColor_Gray:
			return "\033B3B3B3FFm";

		case ELogColor_Default:
		default:
			return "\033FFFFFFFFm";
	}
}


void GuiSystem::DrawConVarList( bool wasOpen )
{
	PROF_SCOPE();

	if ( !ImGui::Begin( "ConVar List", 0, ImGuiWindowFlags_NoScrollbar ) )
	{
		ImGui::End();
		return;
	}

	// TODO: Tabs for ConVar list and ConCommand list
	
	static char buf[ 256 ] = "";

	if ( ImGui::InputText( "Search (Does Nothing atm)", buf, 256 ) )
	{
	}

	ImGui::BeginChild( "ConVar List Child", ImVec2( 0, -ImGui::GetFrameHeightWithSpacing() ), true );

	if ( ImGui::BeginTable( "table1", 5, ImGuiTableFlags_RowBg | ImGuiTableFlags_Borders | ImGuiTableFlags_Resizable ) )
	{
		// Only ConVars here, no ConCommands, that will be in another tab
		u32 i = 0, row = 0;
		for ( const auto& [ cvarName, cvarData ] : Con_GetConVarMap() )
		{
			ch_string cvarDesc = Con_GetConVarDesc( cvarName.data() );

			ImGui::TableNextRow();

			ImGui::TableSetColumnIndex( 0 );
			ImGui::TextUnformatted( cvarName.data() );

			// ImGui::TableSetColumnIndex( 1 );
			// if ( cvar->aDesc )
			{
				ImGui::PushStyleColor( ImGuiCol_Text, ToImCol( ELogColor_Cyan ) );
				ImGui::TextWrapped( cvarDesc.data ? cvarDesc.data : "" );
				ImGui::PopStyleColor();
			}

			ImGui::TableSetColumnIndex( 1 );
			ImGui::TextUnformatted( cvarName.data() );

			ImGui::TableSetColumnIndex( 2 );

			// TODO: update this for new convar system
#if 0
			if ( !ch_str_equals( cvar->apData->apDefaultValue, cvar->apData->aDefaultValueLen, cvar->apData->apValue, cvar->GetValueLen() ) )
			{
				ImGui::PushStyleColor( ImGuiCol_Text, ToImCol( ELogColor_Yellow ) );
				ImGui::TextUnformatted( "modified" );
				ImGui::PopStyleColor();

				if ( ImGui::IsItemHovered() )
				{
					ImGui::BeginTooltip();
					ImGui::Text( "Default Value: %s", cvar->apData->apDefaultValue );
					ImGui::EndTooltip();
				}
			}
			else
			{
				ImGui::TextUnformatted( "default" );
			}
#endif

			ImGui::TableSetColumnIndex( 3 );

#if 0
			if ( cvar->apData->apFunc )
			{
				ImGui::PushStyleColor( ImGuiCol_Text, ToImCol( ELogColor_Yellow ) );
				ImGui::TextUnformatted( "Has Callback" );
				ImGui::PopStyleColor();
			}
			else
			{
				ImGui::TextUnformatted( "No Callback" );
			}
#endif

			ImGui::TableSetColumnIndex( 4 );

			std::string cvarFlags = "";

			for ( size_t fi = 0; fi < Con_GetCvarFlagCount(); fi++ )
			{
				ConVarFlag_t flag = ( 1 << fi );
				if ( cvarData->aFlags & flag )
				{
					if ( !cvarFlags.empty() )
					{
						cvarFlags += " | ";
					}

					cvarFlags += Con_GetCvarFlagName( flag );
				}
			}

			ImGui::TextUnformatted( cvarFlags.c_str() );

			row++;
		}

		ImGui::EndTable();
	}

	// display all
	// for ( uint32_t i = 0; i < Con_GetConVarCount(); i++ )
	// {
	// 	ConVarBase* cvarBase = Con_GetConVar( i );
	// 	if ( !cvarBase )
	// 		continue;
	// 
	// 	ImGui::Text( cvarBase->name );
	// }

	ImGui::EndChild();

	ImGui::End();
}

void GuiSystem::ShowConVarList()
{
	aConsoleShown = !aConsoleShown;
}


void GuiSystem::InitConVarList()
{
}

