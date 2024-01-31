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
constexpr ImVec4 ToImCol( LogColor col )
{
	switch (col)
	{
		// hmm
		case LogColor::Black:
			return {0.3, 0.3, 0.3, 1};
		case LogColor::White:
			return {1, 1, 1, 1};

		case LogColor::DarkBlue:
			return {0, 0.3, 0.8, 1};
		case LogColor::DarkGreen:
			return {0.25, 0.57, 0.25, 1};
		case LogColor::DarkCyan:
			return {0, 0.35, 0.75, 1};
		case LogColor::DarkRed:
			return {0.7, 0, 0.25, 1};
		case LogColor::DarkPurple:
			return {0.45, 0, 0.7, 1};
		case LogColor::DarkYellow:
			return {0.6, 0.6, 0, 1};
		case LogColor::DarkGray:
			return {0.45, 0.45, 0.45, 1};

		case LogColor::Blue:
			return {0, 0.4, 1, 1};
		case LogColor::Green:
			return {0.4, 0.9, 0.4, 1};
		case LogColor::Cyan:
			return {0, 0.85, 1, 1};
		case LogColor::Red:
			return {0.9, 0, 0.4, 1};
		case LogColor::Purple:
			return {0.6, 0, 0.9, 1};
		case LogColor::Yellow:
			return {1, 1, 0, 1};
		case LogColor::Gray:
			return {0.7, 0.7, 0.7, 1};

		case LogColor::Default:
		default:
			return ImGui::GetStyleColorVec4( ImGuiCol_Text );
	}
}


constexpr const char* ToImHex( LogColor col )
{
	switch ( col )
	{
		case LogColor::Black:
			return "\0334C4C4CFFm";
		case LogColor::White:
			return "\033FFFFFFFFm";

		case LogColor::DarkBlue:
			return "\033004CCCFF";
		case LogColor::DarkGreen:
			return "\033409140FFm";
		case LogColor::DarkCyan:
			return "\0330059BFFFm";
		case LogColor::DarkRed:
			return "\033B30040FFm";
		case LogColor::DarkPurple:
			return "\0334300B3FFm";
		case LogColor::DarkYellow:
			return "\033999900FFm";
		case LogColor::DarkGray:
			return "\033737373FFm";

		case LogColor::Blue:
			return "\0330066FFFFm";
		case LogColor::Green:
			return "\03366E666FFm";
		case LogColor::Cyan:
			return "\03300D9FFFFm";
		case LogColor::Red:
			return "\033E60066FFm";
		case LogColor::Purple:
			return "\0339900E6FFm";
		case LogColor::Yellow:
			return "\033FFFF00FFm";
		case LogColor::Gray:
			return "\033B3B3B3FFm";

		case LogColor::Default:
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
		for ( uint32_t i = 0, row = 0; i < Con_GetConVarCount(); i++ )
		{
			ConVarBase* cvarBase = Con_GetConVar( i );
			if ( !cvarBase )
				continue;

			// Only ConVars here, no ConCommands, that will be in another tab
			if ( typeid( *cvarBase ) != typeid( ConVar ) )
				continue;

			ConVar* cvar = static_cast< ConVar* >( cvarBase );

			ImGui::TableNextRow();

			ImGui::TableSetColumnIndex( 0 );
			ImGui::TextUnformatted( cvar->aName );

			// ImGui::TableSetColumnIndex( 1 );
			// if ( cvar->aDesc )
			{
				ImGui::PushStyleColor( ImGuiCol_Text, ToImCol( LogColor::Cyan ) );
				ImGui::TextWrapped( cvar->aDesc ? cvar->aDesc : "" );
				ImGui::PopStyleColor();
			}

			ImGui::TableSetColumnIndex( 1 );
			ImGui::TextUnformatted( cvar->GetChar() );

			ImGui::TableSetColumnIndex( 2 );
			if ( !ch_strcmplen( cvar->apData->aDefaultValueLen, cvar->apData->apDefaultValue, cvar->GetValueLen(), cvar->apData->apValue ) )
			{
				ImGui::PushStyleColor( ImGuiCol_Text, ToImCol( LogColor::Yellow ) );
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

			ImGui::TableSetColumnIndex( 3 );
			if ( cvar->apData->apFunc )
			{
				ImGui::PushStyleColor( ImGuiCol_Text, ToImCol( LogColor::Yellow ) );
				ImGui::TextUnformatted( "Has Callback" );
				ImGui::PopStyleColor();
			}
			else
			{
				ImGui::TextUnformatted( "No Callback" );
			}

			ImGui::TableSetColumnIndex( 4 );

			std::string cvarFlags = "";

			for ( size_t fi = 0; fi < Con_GetCvarFlagCount(); fi++ )
			{
				ConVarFlag_t flag = ( 1 << fi );
				if ( cvar->aFlags & flag )
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
	// 	ImGui::Text( cvarBase->aName );
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

