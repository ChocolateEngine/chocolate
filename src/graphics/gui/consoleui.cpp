#include "gui.h"
#include "../renderer.h"
#include "util.h"

#include "imgui/imgui.h"
#include "imgui/imgui_impl_vulkan.h"
#include "imgui/imgui_impl_sdl.h"

extern Renderer* renderer;

static LogChannel gConsoleChannel = LogGetChannel( "Console" );

CONVAR( con_spam_test, 0 );

CONVAR( conui_max_history, -1 );  // idk
CONVAR( conui_color_test, 0 );

void ReBuildConsoleOutput();

void CvarRebuildConsoleOutput( std::vector< std::string > args )
{
	ReBuildConsoleOutput();
}

// 3 is all colors (slowest), 2 is errors, warnings and validation layers, 1 is errors and warnings, only, 0 is no colors (fastest)
CONVAR( conui_colors, 3, CvarRebuildConsoleOutput );


static int         gCmdDropDownIndex = -1;
static int         gCmdHistoryIndex = -1;

static std::string gCmdUserInput = "";
static int         gCmdUserCursor = 0;


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
		case LogColor::DarkMagenta:
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
		case LogColor::Magenta:
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


bool CheckAddDropDownCommand( ImGuiInputTextCallbackData* data )
{
	//static std::string originalTextBuffer = "";
	static std::string prevTextBuffer = "";

	bool upPressed = ImGui::IsKeyPressed( ImGui::GetKeyIndex(ImGuiKey_UpArrow) );
	bool downPressed = ImGui::IsKeyPressed( ImGui::GetKeyIndex(ImGuiKey_DownArrow) );
	downPressed |= ImGui::IsKeyPressed( ImGui::GetKeyIndex(ImGuiKey_Tab) );

	const auto& autoComplete = console->GetAutoCompleteList();

	// wrap around if over max size
	if ( upPressed && --gCmdDropDownIndex == -2 )
		gCmdDropDownIndex = autoComplete.size() - 1;

	if ( downPressed && ++gCmdDropDownIndex == autoComplete.size() )
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
			console->CalculateAutoCompleteList( data->Buf );
			bufDirty = true;
		}
		else if ( keyPressed )
		{
			// An arrow key or tab is pressed, so fill the buffer with the selected item from the auto complete list
			//snprintf( data->Buf, data->BufSize, console->GetAutoCompleteList()[commandIndex].c_str() );
			snprintf( data->Buf, data->BufSize, (console->GetAutoCompleteList()[gCmdDropDownIndex] + " ").c_str() );
			bufDirty = true;
		}
		else if ( inDropDown )
		{
			// An item in auto complete list is selected, but the user typed something in
			// So set back to original text recalculate the auto complete list
			gCmdUserCursor = data->CursorPos;
			gCmdDropDownIndex = 0;
			gCmdUserInput = data->Buf;
			console->CalculateAutoCompleteList( data->Buf );
			bufDirty = true;
		}
	}

	if ( bufDirty )
	{
		console->SetTextBuffer( data->Buf, false );
		data->BufTextLen = console->GetTextBuffer().length();

		if ( inDropDown )
			data->CursorPos = console->GetTextBuffer().length();
		else if ( !textBufChanged )
			data->CursorPos = gCmdUserCursor;
		else
			gCmdUserCursor = data->CursorPos;

		prevTextBuffer = console->GetTextBuffer();
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

	const auto& history = console->GetCommandHistory();

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
		snprintf( data->Buf, 256, (gCmdHistoryIndex == -1) ? gCmdUserInput.c_str() : history[gCmdHistoryIndex].c_str() );
		console->SetTextBuffer( data->Buf );
		prevTextBuffer = data->Buf;

		if ( gCmdHistoryIndex == -1 )
			data->CursorPos = gCmdUserCursor;
		else
			data->CursorPos = console->GetTextBuffer().length();
	}

	return ( upPressed || downPressed );
}


bool CheckEnterPress( char* buf )
{
	bool isPressed = ImGui::IsKeyPressed( ImGui::GetKeyIndex(ImGuiKey_Enter), false );
	isPressed |= ImGui::IsKeyPressed( ImGui::GetKeyIndex(ImGuiKey_KeyPadEnter), false );

	if ( isPressed )
	{
		console->QueueCommand( buf );
		snprintf( buf, 256, "" );
		console->SetTextBuffer( buf );
		ImGui::SetKeyboardFocusHere(  );

		gCmdUserInput = "";
		gCmdHistoryIndex = -1;
		gCmdDropDownIndex = -1;
	}

	return isPressed;
}


int ConsoleInputCallback( ImGuiInputTextCallbackData* data )
{
	static int lastCommandIndex = -1;

	if ( gCmdHistoryIndex != -1 )
	{
		bool keyPressed = ImGui::IsKeyPressed( ImGui::GetKeyIndex(ImGuiKey_Tab) );
		keyPressed |= ImGui::IsKeyPressed( ImGui::GetKeyIndex(ImGuiKey_RightArrow) );

		if ( keyPressed )
			data->BufDirty = CheckAddDropDownCommand( data );
	}
	else
	{
		data->BufDirty = CheckAddDropDownCommand( data );
	}

	if ( console->GetAutoCompleteList().empty() || gCmdHistoryIndex != -1 )
	{
		data->BufDirty = CheckAddLastCommand( data );
	}

	if ( !data->BufDirty )
	{
		data->BufDirty = CheckEnterPress( data->Buf );
	}

	if ( data->BufDirty )
	{
		data->BufTextLen = console->GetTextBuffer().length();
	}

	return 0;
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
		ImGui::Text( "Color: %hhu - %s", i, LogColorToStr( (LogColor)i ) );
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
	//ImGui::BeginPopup( "cvars_modal", ImGuiWindowFlags_Popup );
	//ImGui::BeginPopupModal( "cvars_modal" );

	ImGui::SetNextWindowPos( dropDownPos, ImGuiCond_Always, ImVec2(0.0f, 0.0f));

	ImGui::PushStyleVar( ImGuiStyleVar_WindowPadding, ImVec2(0,0) );

	if ( ImGui::Begin( "cvars_box", NULL,
		ImGuiWindowFlags_NoBringToFrontOnFocus |
		ImGuiWindowFlags_AlwaysAutoResize |
		ImGuiWindowFlags_NoBackground |
		ImGuiWindowFlags_NoDecoration |
		ImGuiWindowFlags_NoTitleBar |
		ImGuiWindowFlags_NoMove |
		//ImGuiWindowFlags_NoNavFocus |
		ImGuiWindowFlags_NoFocusOnAppearing ) )
	{
		if ( ImGui::BeginListBox( " " ) )
		{
			for ( int i = 0; i < cvarAutoComplete.size(); i++ )
			{
				std::string item = cvarAutoComplete[i];

				item += " " + console->GetConVarValue( item );

				if ( ImGui::Selectable( item.c_str(), gCmdDropDownIndex == i ) )
				{
					// should we keep the value in here too?
					console->SetTextBuffer( cvarAutoComplete[i] );
					//break;
				}
			}

			ImGui::EndListBox(  );
		}

		ImGui::End(  );
	}

	ImGui::PopStyleVar();

	//ImGui::EndPopup(  );
}


void DrawLogChannelButtons()
{
}


LOG_CHANNEL( Validation );


LogColor GetConsoleTextColor( const Log& log )
{
	LogColor color = LogColor::Default;

	if ( conui_colors == 0.f )
		return color;

	if ( log.aType == LogType::Warning )
		color = LOG_COLOR_WARNING;

	else if ( log.aType == LogType::Error || log.aType == LogType::Fatal )
		color = LOG_COLOR_ERROR;

	else if ( (conui_colors == 2.f && log.aChannel == gValidationChannel) || conui_colors >= 3.f )
		color = LogGetChannelColor( log.aChannel );

	return color;
}


struct ConLogBuffer
{
	LogColor    aColor;
	std::string aBuffer;
};


static std::vector< ConLogBuffer > gConHistory;
static size_t gConHistoryIndex = SIZE_MAX;


void ReBuildConsoleOutput()
{
	gConHistory.clear();

	ConLogBuffer* buffer = &gConHistory.emplace_back();
	const std::vector< Log >& logs = LogGetLogHistory();

	for ( const auto& log: logs )
	{
		if ( !LogChannelIsShown( log.aChannel ) )
			continue;

		LogColor color = GetConsoleTextColor( log );

		if ( color != buffer->aColor && buffer->aBuffer.size() )
			buffer = &gConHistory.emplace_back( color, log.aFormatted );

		else if ( buffer->aBuffer.empty() )
		{
			buffer->aColor = color;
			buffer->aBuffer += log.aFormatted;
		}

		else
			buffer->aBuffer += log.aFormatted;
	}

	gConHistoryIndex = logs.size();
}


void UpdateConsoleOutput()
{
	const std::vector< Log >& logs = LogGetLogHistory();

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
		ConLogBuffer* buffer = &gConHistory.at( gConHistory.size() - 1 );
		const Log& log = logs[gConHistoryIndex];
		LogColor nextColor = GetConsoleTextColor( log );

		if ( buffer->aColor != nextColor )
			buffer = &gConHistory.emplace_back( nextColor, log.aFormatted );

		else if ( buffer->aBuffer.empty() )
		{
			buffer->aColor = nextColor;
			buffer->aBuffer += log.aFormatted;
		}

		else
			buffer->aBuffer += log.aFormatted;
	}
}


void DrawConsoleOutput()
{
	// um
	ImVec4* colors = ImGui::GetStyle().Colors;
	ImVec4 bgColor = colors[ImGuiCol_FrameBg];

	bgColor.x /= 2;
	bgColor.y /= 2;
	bgColor.z /= 2;

	ImGui::PushStyleColor( ImGuiCol_ChildBg, bgColor );
	
	// ImGui::BeginChildFrame
	ImGui::BeginChild( "ConsoleOutput", ImVec2( ImGui::GetWindowSize().x - 32, ImGui::GetWindowSize().y - 64 ), true );
	static int scrollMax = ImGui::GetScrollMaxY();

	UpdateConsoleOutput();

	for ( size_t i = 0; i < gConHistory.size(); i++ )
	{
		const ConLogBuffer& buffer = gConHistory.at( i );

		// show buffer, clear it, then set the new color
		ImGui::PushStyleColor( ImGuiCol_Text, ToImCol( buffer.aColor ) );
		ImGui::TextUnformatted( buffer.aBuffer.c_str() );
		ImGui::PopStyleColor();
	}

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
	if ( con_spam_test.GetBool() )
		LogMsg( "TEST\n" );

	ImGui::Begin( "Developer Console" );
	static char buf[ 256 ] = "";

	DrawColorTest();
	DrawFilterBox();

	if ( !wasConsoleOpen )
		ImGui::SetKeyboardFocusHere(0);

	DrawConsoleOutput();

	snprintf( buf, 256, console->GetTextBuffer(  ).c_str() );
	ImGui::InputText( "send", buf, 256, ImGuiInputTextFlags_CallbackAlways, &ConsoleInputCallback );

	if ( !wasConsoleOpen )
		ImGui::SetKeyboardFocusHere(0);

	ImVec2 dropDownPos( ImGui::GetWindowPos().x, ImGui::GetWindowSize().y + ImGui::GetWindowPos().y );

	ImGui::End(  );

	const std::vector< std::string >& cvarAutoComplete = console->GetAutoCompleteList();

	if ( !cvarAutoComplete.empty() )
	{
		DrawInputDropDownBox( cvarAutoComplete, dropDownPos );
	}
}

void GuiSystem::ShowConsole(  )
{
	aConsoleShown = !aConsoleShown;
}

bool GuiSystem::IsConsoleShown(  )
{
	return aConsoleShown;
}

void GuiSystem::InitConsole()
{
	LogAddChannelShownCallback( ReBuildConsoleOutput );

	gConHistory.resize( 4096 );
}

