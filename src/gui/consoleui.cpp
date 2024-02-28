#include "gui.h"
#include "util.h"

#include "core/core.h"

#include "imgui/imgui.h"
#include "imgui/imgui_impl_sdl2.h"


CONVAR( conui_spam_test, 0 );

CONVAR( conui_max_history, -1 );  // idk
CONVAR( conui_color_test, 0 );
CONVAR( conui_max_buffer_size, 8192, "Maximum amount of characters in a string in the console output, it can go over the limit, but only to fit the remaining log message" );

CONVAR_CMD( conui_show_convar_list, 0 )
{
	gui->aConVarListShown = conui_show_convar_list.GetBool();
}

void ReBuildConsoleOutput();
void UpdateConsoleOutput();


CONVAR_CMD_EX( conui_colors, 2, CVARF_ARCHIVE, "2 is all colors, 1 is errors and warnings, only, 0 is no colors" )
{
	ReBuildConsoleOutput();
}


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


bool CheckAddDropDownCommand( ImGuiInputTextCallbackData* data )
{
	//static std::string originalTextBuffer = "";
	static std::string prevTextBuffer = "";

	bool upPressed = ImGui::IsKeyPressed( ImGui::GetKeyIndex(ImGuiKey_UpArrow) );
	bool downPressed = ImGui::IsKeyPressed( ImGui::GetKeyIndex(ImGuiKey_DownArrow) );
	downPressed |= ImGui::IsKeyPressed( ImGui::GetKeyIndex(ImGuiKey_Tab) );

	// wrap around if over max size
	if ( upPressed && --gCmdDropDownIndex == -2 )
		gCmdDropDownIndex = gCmdAutoComplete.size() - 1;

	if ( downPressed && ++gCmdDropDownIndex == gCmdAutoComplete.size() )
		gCmdDropDownIndex = -1;

	bool keyPressed = upPressed || downPressed;

	// Reset the selected command index if the buffer is empty
	if ( strncmp(data->Buf, "", data->BufSize) == 0 && !keyPressed )
		gCmdDropDownIndex = -1;

	bool inDropDown = gCmdDropDownIndex != -1;
	bool textBufChanged = prevTextBuffer != data->Buf;

	// If the user typed something in, then update the original text inputted
	// relies on the if (inDropDown) below to set commandIndex back to -1 if there was something selected
	if ( textBufChanged && !inDropDown )
		gCmdUserInput = data->Buf;

	bool bufDirty = false;

	if ( keyPressed || textBufChanged )
	{
		if ( !inDropDown )
		{
			// No items selected in auto complete list
			// Set back to original text and recalculate the auto complete list
			snprintf( data->Buf, data->BufSize, gCmdUserInput.c_str() );
			gCmdAutoComplete.clear();
			Con_BuildAutoCompleteList( data->Buf, gCmdAutoComplete );
			bufDirty = true;
		}
		else if ( keyPressed )
		{
			// An arrow key or tab is pressed, so fill the buffer with the selected item from the auto complete list

			// Testing: if a ConVar, add a space after it
			// otherwise, leave it as is
			// For ConCommand dropdowns, if we add a space, we need to press backspace and space in order to show the dropdown, kinda weird
			// but for convars, i basically never press space, and just start typing

			ConVarBase* cvarBase = Con_GetConVarBase( gCmdAutoComplete[ gCmdDropDownIndex ].c_str() );
			if ( cvarBase )
			{
				if ( typeid( *cvarBase ) == typeid( ConVar ) )
					snprintf( data->Buf, data->BufSize, ( gCmdAutoComplete[ gCmdDropDownIndex ] + " " ).c_str() );
				else
					snprintf( data->Buf, data->BufSize, gCmdAutoComplete[ gCmdDropDownIndex ].c_str() );
			}
			else
			{
				snprintf( data->Buf, data->BufSize, gCmdAutoComplete[ gCmdDropDownIndex ].c_str() );
			}

			bufDirty = true;
		}
		else if ( inDropDown )
		{
			// An item in auto complete list is selected, but the user typed something in
			// So set back to original text recalculate the auto complete list
			gCmdUserCursor    = data->CursorPos;
			gCmdDropDownIndex = 0;
			gCmdAutoComplete.clear();
			Con_BuildAutoCompleteList( data->Buf, gCmdAutoComplete );
			bufDirty = true;
		}
	}

	if ( bufDirty )
	{
		data->BufTextLen = strlen( data->Buf );

		if ( inDropDown )
			data->CursorPos = data->BufTextLen;
		else if ( !textBufChanged )
			data->CursorPos = gCmdUserCursor;
		else
			gCmdUserCursor = data->CursorPos;

		prevTextBuffer = data->Buf;
	}
	else if ( !inDropDown && gCmdUserCursor != data->CursorPos )
	{
		gCmdUserCursor = data->CursorPos;
	}

	return bufDirty;
}


// not in a drop down command, so feel free to add in a command from history if we hit up or down arrow
bool CheckAddLastCommand( ImGuiInputTextCallbackData* data )
{
	static std::string prevTextBuffer = "";

	const auto& history = Con_GetCommandHistory();

	if ( history.empty() )
		return false;

	bool upPressed = ImGui::IsKeyPressed( ImGui::GetKeyIndex(ImGuiKey_UpArrow) );
	bool downPressed = ImGui::IsKeyPressed( ImGui::GetKeyIndex(ImGuiKey_DownArrow) );

	bool textBufChanged = prevTextBuffer != data->Buf;

	// If the user typed something in, then update the original text inputted
	// relies on the if (inDropDown) below to set commandIndex back to -1 if there was something selected
	if ( textBufChanged && !(upPressed || downPressed) )
	{
		gCmdUserInput = data->Buf;
		//gCmdUserCursor = data->CursorPos;
		gCmdHistoryIndex = -1;
	}

	if ( gCmdHistoryIndex == -1 )
		gCmdUserCursor = data->CursorPos;

	// wrap around if over max size
	if ( upPressed && --gCmdHistoryIndex == -2 )
		gCmdHistoryIndex = history.size() - 1;

	if ( downPressed && ++gCmdHistoryIndex == history.size() )
		gCmdHistoryIndex = -1;

	if ( upPressed || downPressed )
	{
		snprintf( data->Buf, 256, ( gCmdHistoryIndex == -1 ) ? gCmdUserInput.c_str() : history[ gCmdHistoryIndex ].c_str() );
		gCmdUserInput = data->Buf;
		prevTextBuffer = data->Buf;

		if ( gCmdHistoryIndex == -1 )
			data->CursorPos = gCmdUserCursor;
		else
			data->CursorPos = gCmdUserInput.length();
	}

	return ( upPressed || downPressed );
}


bool CheckEnterPress( char* buf )
{
	bool isPressed = ImGui::IsKeyPressed( ImGui::GetKeyIndex( ImGuiKey_Enter ), false );
	isPressed |= ImGui::IsKeyPressed( ImGui::GetKeyIndex( ImGuiKey_KeypadEnter ), false );

	if ( isPressed )
	{
		Con_QueueCommand( buf );
		snprintf( buf, 256, "" );
		ImGui::SetKeyboardFocusHere( -1 );

		gCmdUserInput = "";
		gCmdHistoryIndex = -1;
		gCmdDropDownIndex = -1;

		// TODO: add this to the actual in-game console history before returning somehow
		// if you loaded a map, you won't see the command in the game console until the map finished loading
	}

	return isPressed;
}


int ConsoleInputCallback( ImGuiInputTextCallbackData* data )
{
	static int lastCommandIndex = -1;

	if ( gCmdHistoryIndex != -1 )
	{
		bool keyPressed = ImGui::IsKeyPressed( ImGui::GetKeyIndex( ImGuiKey_Tab ) );
		keyPressed |= ImGui::IsKeyPressed( ImGui::GetKeyIndex( ImGuiKey_RightArrow ) );

		if ( keyPressed )
			data->BufDirty = CheckAddDropDownCommand( data );
	}
	else
	{
		data->BufDirty = CheckAddDropDownCommand( data );
	}

	if ( gCmdAutoComplete.empty() || gCmdHistoryIndex != -1 )
	{
		data->BufDirty = CheckAddLastCommand( data );
	}

	bool enterPressed = false;

	if ( !data->BufDirty )
	{
		enterPressed   = CheckEnterPress( data->Buf );
		data->BufDirty = enterPressed;
	}

	if ( data->BufDirty )
	{
		data->BufTextLen = strlen( data->Buf );
	}

	return enterPressed;
}


int FilterBoxCallback( ImGuiInputTextCallbackData* data, bool exclude )
{
	return 0;
}

int FilterBoxCallbackEx( ImGuiInputTextCallbackData* data )
{
	return FilterBoxCallback( data, true );
}

int FilterBoxCallbackIn( ImGuiInputTextCallbackData* data )
{
	return FilterBoxCallback( data, false );
}


void DrawColorTest()
{
	if ( !conui_color_test )
		return;

	ImVec4* colors = ImGui::GetStyle().Colors;
	ImVec4 bgColor = colors[ImGuiCol_FrameBg];

	bgColor.x /= 2;
	bgColor.y /= 2;
	bgColor.z /= 2;

	ImGui::PushStyleColor( ImGuiCol_ChildBg, bgColor );

	// ImGui::BeginChildFrame
	ImGui::BeginChild( "ColorTest" );

	for ( unsigned char i = 0; i < (unsigned char)LogColor::Count; i++ )
	{
		ImGui::PushStyleColor( ImGuiCol_Text, ToImCol( (LogColor)i ) );
		ImGui::Text( "Color: %hhu - %s", i, Log_ColorToStr( (LogColor)i ) );
		ImGui::PopStyleColor();
	}

	ImGui::EndChild();
	ImGui::PopStyleColor();
}

void DrawFilterBox()
{
	return;

	static char bufEx[ 256 ] = "";
	static char bufIn[ 256 ] = "";
	ImGui::InputText( "Exclude", bufEx, 256, ImGuiInputTextFlags_CallbackAlways, &FilterBoxCallbackEx );
	ImGui::InputText( "Include", bufIn, 256, ImGuiInputTextFlags_CallbackAlways, &FilterBoxCallbackIn );
}


void DrawInputDropDownBox( const std::vector< std::string >& cvarAutoComplete, ImVec2& dropDownPos )
{
	PROF_SCOPE();

	//ImGui::BeginPopup( "cvars_modal", ImGuiWindowFlags_Popup );
	//ImGui::BeginPopupModal( "cvars_modal" );

	ImGui::SetNextWindowPos( dropDownPos, ImGuiCond_Always, ImVec2(0.0f, 0.0f));

	ImGui::PushStyleVar( ImGuiStyleVar_WindowPadding, ImVec2(0,0) );

	// calculate max dropdown string length
	size_t maxLength = 0;
	std::string maxLengthItem;
	for ( size_t i = 0; i < cvarAutoComplete.size(); i++ )
	{
		std::string item  = cvarAutoComplete[ i ];
		std::string value = Con_GetConVarValue( item ).data();

		if ( value.size() )
			item += " " + value;

		if ( item.size() > maxLength )
		{
			maxLength     = item.size();
			maxLengthItem = item;
		}
	}

	ImVec2      textSize     = ImGui::CalcTextSize( maxLengthItem.c_str(), &maxLengthItem.back() );
	float       itemWidth    = ImGui::GetWindowContentRegionMin().x * 2;
	float       defaultWidth = ImGui::CalcItemWidth();
	ImGuiStyle& imguiStyle   = ImGui::GetStyle();
	
	ImGui::SetNextItemWidth( glm::max( textSize.x, defaultWidth ) + itemWidth + imguiStyle.ScrollbarSize );

	// ImGui::SetNextWindowSize( textSize );

	if ( ImGui::Begin( "##CvarsBox", NULL,
		ImGuiWindowFlags_AlwaysAutoResize |
		ImGuiWindowFlags_NoBackground |
		ImGuiWindowFlags_NoDecoration |
		ImGuiWindowFlags_NoTitleBar |
		ImGuiWindowFlags_NoMove |
		ImGuiWindowFlags_NoSavedSettings |
		//ImGuiWindowFlags_NoNavFocus |
		ImGuiWindowFlags_NoFocusOnAppearing ) )
	{
		if ( ImGui::BeginListBox( "##CvarListBox" ) )
		{
			for ( size_t i = 0; i < cvarAutoComplete.size(); i++ )
			{
				std::string item = cvarAutoComplete[ i ];

				item += " ";
				item += Con_GetConVarValue( cvarAutoComplete[ i ] );

				if ( ImGui::Selectable( item.c_str(), gCmdDropDownIndex == i ) )
				{
					// should we keep the value in here too?
					gCmdUserInput = cvarAutoComplete[ i ];
					//break;
				}
			}

			ImGui::EndListBox();
		}

		ImGui::End();
	}

	ImGui::PopStyleVar();

	//ImGui::EndPopup(  );
}


void DrawLogChannelButtons()
{
}


LogColor GetConsoleTextColor( const Log& log )
{
	if ( conui_colors == 0.f )
		return LogColor::Default;

	if ( log.aType == LogType::Warning )
		return LOG_COLOR_WARNING;

	else if ( log.aType == LogType::Error || log.aType == LogType::Fatal )
		return LOG_COLOR_ERROR;

	else if ( conui_colors >= 2.f )
		return Log_GetChannelColor( log.aChannel );

	return LogColor::Default;
}


struct ConLogBuffer
{
	LogColor    aColor;
	std::string aBuffer;
};


static std::vector< ConLogBuffer > gConHistory;
static size_t gConHistoryIndex = 0;

// can and will be over this limit
constexpr size_t CON_MAX_BUFFER_SIZE = 512;


static void AddToConsoleOutputOld( ConLogBuffer* spBuffer, const Log& srLog )
{
	ChVector< LogColorBuf_t > colorBuffers;
	Log_SplitStringColors( GetConsoleTextColor( srLog ), srLog.aFormatted, colorBuffers );

	for ( LogColorBuf_t& colorBuf : colorBuffers )
	{
		std::string buf( colorBuf.apStr, colorBuf.aLen );

		if ( colorBuf.aColor != spBuffer->aColor && spBuffer->aBuffer.size() || spBuffer->aBuffer.size() > CON_MAX_BUFFER_SIZE )
		{
			spBuffer = &gConHistory.emplace_back( colorBuf.aColor, buf );
		}
		else if ( spBuffer->aBuffer.empty() )
		{
			spBuffer->aColor = colorBuf.aColor;
			spBuffer->aBuffer += buf;
		}
		else
		{
			spBuffer->aBuffer += buf;
		}
	}
}


static void AddToConsoleOutput( ConLogBuffer* spBuffer, const Log& srLog )
{
	ChVector< LogColorBuf_t > colorBuffers;
	LogColor mainColor = GetConsoleTextColor( srLog );
	Log_SplitStringColors( mainColor, srLog.aFormatted, colorBuffers );

	for ( LogColorBuf_t& colorBuf : colorBuffers )
	{
		std::string buf( colorBuf.apStr, colorBuf.aLen );

		// if ( colorBuf.aColor != spBuffer->aColor && spBuffer->aBuffer.size() || spBuffer->aBuffer.size() > CON_MAX_BUFFER_SIZE )
		// if ( mainColor != spBuffer->aColor && spBuffer->aBuffer.size() || spBuffer->aBuffer.size() > CON_MAX_BUFFER_SIZE )
		if ( mainColor != spBuffer->aColor && spBuffer->aBuffer.size() )
		{
			spBuffer = &gConHistory.emplace_back( mainColor );
			// spBuffer->aBuffer += ToImHex( colorBuf.aColor );
		}

		if ( spBuffer->aBuffer.empty() )
		{
			spBuffer->aColor = mainColor;
			spBuffer->aBuffer += buf;
		}
		else
		{
			spBuffer->aBuffer += buf;
		}
	}
}


void ReBuildConsoleOutput()
{
	gConHistory.clear();
	gConHistory.resize( 1 );

	const std::vector< Log >& logs = Log_GetLogHistory();

	for ( const auto& log: logs )
	{
		if ( !Log_IsVisible( log ) )
			continue;

		AddToConsoleOutput( &gConHistory.back(), log );
	}

	gConHistoryIndex = logs.size();
}


void UpdateConsoleOutput()
{
	const std::vector< Log >& logs = Log_GetLogHistory();

	if ( logs.empty() )
		return;

	if ( gConHistory.empty() )
	{
		ReBuildConsoleOutput();
		return;
	}

	if ( gConHistoryIndex == logs.size() )
		return;

	for ( ; gConHistoryIndex < logs.size(); gConHistoryIndex++ )
	{
		const Log& log = logs[ gConHistoryIndex ];

		if ( !Log_IsVisible( log ) )
			continue;

		AddToConsoleOutput( &gConHistory.back(), log );
	}
}


void DrawConsoleOutput()
{
	PROF_SCOPE();

	// um
	ImVec4* colors = ImGui::GetStyle().Colors;
	ImVec4 bgColor = colors[ImGuiCol_FrameBg];

	bgColor.x /= 2;
	bgColor.y /= 2;
	bgColor.z /= 2;

	ImGui::PushStyleColor( ImGuiCol_ChildBg, bgColor );

	auto contentRegion = ImGui::GetContentRegionAvail();
	
	// ImGui::BeginChildFrame
	// ImGui::SetNextItemWidth( -1.f );  // force it to fill the window
	// ImGui::BeginChild( "ConsoleOutput", ImVec2( ImGui::GetWindowSize().x - 32, ImGui::GetWindowSize().y - 64 ), true );
	ImGui::BeginChild( "ConsoleOutput", ImVec2( 0, -ImGui::GetFrameHeightWithSpacing() ), true );
	static int scrollMax = ImGui::GetScrollMaxY();

	UpdateConsoleOutput();

	ImGui::PushStyleVar( ImGuiStyleVar_ItemSpacing, ImVec2( 0, 0 ) );

	// ImGuiWindow* window = ImGui::GetCurrentWindow();

	ImVec2 cursorPos = ImGui::GetCursorPos();
	
	for ( size_t i = 0; i < gConHistory.size(); i++ )
	{
		const ConLogBuffer& buffer = gConHistory.at( i );

		// show buffer, clear it, then set the new color
		ImGui::PushStyleColor( ImGuiCol_Text, ToImCol( buffer.aColor ) );

		bool needWrap   = false;
		bool hasNewLine = false;

		// horrible
		if ( buffer.aBuffer.size() )
		{
			needWrap   = buffer.aBuffer[ buffer.aBuffer.size() - 1 ] == '\n';
			hasNewLine = buffer.aBuffer[ buffer.aBuffer.size() - 1 ] == '\n';
		}

		if ( needWrap )
			ImGui::PushTextWrapPos( 0.0f );

		ImGui::TextUnformatted( buffer.aBuffer.c_str() );
		// ImGui::PushID( i );
		// ImGui::TextSelectable( "##test", buffer.aBuffer.c_str() );
		// ImGui::PopID();

		if ( needWrap )
			ImGui::PopTextWrapPos();

		ImGui::PopStyleColor();
	}

	ImGui::PopStyleVar();

	// ImGui::TextUnformatted( LogGetHistoryStr( conui_max_history.GetFloat() ).c_str() );

	// if we were scrolled all the way down before, make sure we stay scrolled down all the way
	if ( scrollMax == ImGui::GetScrollY() )
		ImGui::SetScrollY( ImGui::GetScrollMaxY() );

	scrollMax = ImGui::GetScrollMaxY();
	ImGui::EndChild();

	ImGui::PopStyleColor();
}


void GuiSystem::DrawConsole( bool wasConsoleOpen )
{
	PROF_SCOPE();

	if ( conui_spam_test.GetBool() )
	{
		Log_Msg(
		  "Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua.\n"
		  "Ut enim ad minim veniam, quis nostrud exercitation ullamco laboris nisi ut aliquip ex ea commodo consequat.\n"
		  "Duis aute irure dolor in reprehenderit in voluptate velit esse cillum dolore eu fugiat nulla pariatur.\n"
		  "Excepteur sint occaecat cupidatat non proident, sunt in culpa qui officia deserunt mollit anim id est laborum\n" );
	}

	if ( !ImGui::Begin( "Developer Console", 0, ImGuiWindowFlags_NoScrollbar ) )
	{
		ImGui::End();
		return;
	}
	
	static char buf[ 256 ] = "";

	DrawColorTest();
	DrawFilterBox();

	ImGui::PushAllowKeyboardFocus( false );
	DrawConsoleOutput();
	ImGui::PopAllowKeyboardFocus();

	snprintf( buf, 256, gCmdUserInput.c_str() );

	ImGuiStyle& style = ImGui::GetStyle();

	ImVec2 what = ImGui::GetItemRectSize();

	ImVec2 sendBtnSize = ImGui::CalcTextSize( "Send" );
	float  inputTextWidth = -ImGui::GetFrameHeightWithSpacing() - sendBtnSize.y - ( style.FramePadding.y * 2 );

	// ImGui::SetNextItemWidth( -1.f );  // force it to fill the window

	// small hack to offset this
	ImGui::SetCursorPosY( ImGui::GetCursorPosY() + 1 );

	ImGui::SetNextItemWidth( inputTextWidth );  // force it to fill the window

	static ImVec2 prevWindowSize   = ImGui::GetWindowSize();
	ImVec2        newWindowSize    = ImGui::GetWindowSize();

	static bool   inResize         = false;

	bool          setKeyboardFocus = !wasConsoleOpen;

	ImVec2 windowMaxPos = ImGui::GetWindowPos();
	windowMaxPos.x += ImGui::GetWindowSize().x;
	windowMaxPos.y += ImGui::GetWindowSize().y;

	bool overWindow = ImGui::IsMouseHoveringRect( ImGui::GetWindowPos(), windowMaxPos );

	// quite hacky setup for text focus
	// if ( overWindow || inResize )
	{
		if ( ImGui::IsItemClicked() )
		{
			setKeyboardFocus = true;
		}

		// window resizing
		// else if ( ImGui::IsMouseDragging( ImGuiMouseButton_Left ) )
		// {
		// 	inResize         = prevWindowSize.x != newWindowSize.x || prevWindowSize.y != newWindowSize.y;
		// 	setKeyboardFocus = !inResize;
		// }
	}
	// else
	// {
	// 	if ( inResize )
	// 		setKeyboardFocus = true;
	// 
	// 	inResize = false;
	// }

	prevWindowSize = newWindowSize;

	if ( setKeyboardFocus )
	{
		ImGui::SetKeyboardFocusHere();
	}

	ImGui::InputText( "##send", buf, 256, ImGuiInputTextFlags_CallbackAlways, &ConsoleInputCallback );
	ImGui::SetItemDefaultFocus();

	ImGui::SameLine();

	ImGui::PushAllowKeyboardFocus( false );

	if ( ImGui::Button( "Send" ) )
	// if ( false )
	{
		Con_QueueCommand( buf );
		snprintf( buf, 256, "" );
		ImGui::SetKeyboardFocusHere();
	
		gCmdUserInput     = "";
		gCmdHistoryIndex  = -1;
		gCmdDropDownIndex = -1;
	}

	ImGui::PopAllowKeyboardFocus();

	// ImVec2 windowSize = ImGui::GetWindowSize();
	// windowSize.y += 2;
	// ImGui::SetWindowSize( windowSize );

	ImVec2 dropDownPos( ImGui::GetWindowPos().x, ImGui::GetWindowSize().y + ImGui::GetWindowPos().y );
	
	// this is correct, but there's an issue of window ordering
	// auto popupPos  = ImGui::GetItemRectMin();
	// ImVec2 dropDownPos( popupPos.x, popupPos.y + ImGui::GetFrameHeight() );
	// dropDownPos.y += ImGui::GetFrameHeight();

	// test
	// if ( ImGui::Button( "Toggle ConVar List" ) )
	// {
	// 	aConVarListShown = !aConVarListShown;
	// }

	ImGui::End();

	if ( !gCmdAutoComplete.empty() )
	{
		DrawInputDropDownBox( gCmdAutoComplete, dropDownPos );
	}
}

void GuiSystem::ShowConsole()
{
	aConsoleShown = !aConsoleShown;
}

bool GuiSystem::IsConsoleShown()
{
	return aConsoleShown;
}

void GuiSystem::InitConsole()
{
	Log_AddChannelShownCallback( ReBuildConsoleOutput );
}

