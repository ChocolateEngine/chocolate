#include "gui.h"
#include "renderer.h"
#include "util.h"

#include "imgui/imgui.h"
#include "imgui/imgui_impl_vulkan.h"
#include "imgui/imgui_impl_sdl.h"

GuiSystem* gui = new GuiSystem;

extern Renderer* renderer;

void GuiSystem::Update( float sDT )
{
	DrawGui(  );
}

void GuiSystem::DrawGui(  )
{
	if ( !apWindow )
		return;

	static bool wasConsoleOpen = false;
	static Uint32 prevtick = 0;
	static Uint32 curtick = 0;

	if ( !aDrawnFrame )
		renderer->EnableImgui(  );

	if ( aConsoleShown )
		DrawConsole( wasConsoleOpen );

	wasConsoleOpen = aConsoleShown;

	ImGuiWindowFlags devFlags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize;
	if ( !aConsoleShown )
		devFlags |= ImGuiWindowFlags_NoMove;

	//curtick = SDL_GetTicks();
	//Uint32 delta = curtick - prevtick;;

	// NOTE: imgui framerate is a rough estimate, might wanna have our own fps counter?
	ImGui::Begin( "Dev Info", (bool*)0, devFlags );
	float frameRate = ImGui::GetIO().Framerate;
	ImGui::Text("%.1f FPS (%.3f ms/frame)", frameRate, 1000.0f / frameRate);
	//ImGui::Text("%.1f FPS (%.3f ms/frame)", 1000.f / delta, (float)delta);  // ms/frame is inaccurate

	std::string debugMessages;
	for (int i = 0; i < aDebugMessages.size(); i++)
	{
		debugMessages += "\n";

		if (aDebugMessages[i] != "")
			debugMessages += aDebugMessages[i];
	}

	aDebugMessages.clear();
	if ( !debugMessages.empty() )
		ImGui::Text(debugMessages.c_str());

	ImGui::End(  );
	prevtick = SDL_GetTicks();

	aDrawnFrame = true;
}


static int         gCmdDropDownIndex = -1;
static int         gCmdHistoryIndex = -1;

static std::string gCmdUserInput = "";
static int         gCmdUserCursor = 0;


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
		console->Add( buf );
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


CONVAR( con_spam_test, 0 );
CONVAR( con_gui_max_history, -1 );  // idk


void GuiSystem::DrawConsole( bool wasConsoleOpen )
{
	if ( con_spam_test.GetBool() )
		LogMsg( "TEST\n" );

	// blech
	static bool wasUpArrowPressed = false;
	//static bool wasUpArrowPressed = false;

	static int lastCommandIndex = 0;

	ImGui::Begin( "Developer Console" );
	static char buf[ 256 ] = "";

	if ( !wasConsoleOpen )
		ImGui::SetKeyboardFocusHere(0);

	ImGui::BeginChild("ConsoleOutput", ImVec2( ImGui::GetWindowSize().x - 32, ImGui::GetWindowSize().y - 64 ), true);
	static int scrollMax = ImGui::GetScrollMaxY();

	ImGui::TextUnformatted( LogGetHistoryStr( con_gui_max_history.GetFloat() ).c_str() );

	if ( scrollMax == ImGui::GetScrollY() )
		ImGui::SetScrollY( ImGui::GetScrollMaxY() );

	scrollMax = ImGui::GetScrollMaxY();
	ImGui::EndChild();

	snprintf( buf, 256, console->GetTextBuffer(  ).c_str() );
	ImGui::InputText( "send", buf, 256, ImGuiInputTextFlags_CallbackAlways, &ConsoleInputCallback );

	if ( !wasConsoleOpen )
		ImGui::SetKeyboardFocusHere(0);

	ImVec2 dropDownPos( ImGui::GetWindowPos().x, ImGui::GetWindowSize().y + ImGui::GetWindowPos().y );

	ImGui::End(  );

	const std::vector< std::string >& cvarAutoComplete = console->GetAutoCompleteList();

	if ( !cvarAutoComplete.empty() )
	// if ( 0 )
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

}

void GuiSystem::StyleImGui()
{
	auto& io = ImGui::GetIO();

#if 0
	if ( !cmdline->Find( "-no-imgui-font" ) )
	{
		// auto fontPath = filesys->FindFile( "fonts/CascadiaCode.ttf" );
		// auto fontPath = filesys->FindFile( "fonts/CascadiaMono.ttf" );
		auto fontPath = filesys->FindFile( "fonts/Roboto-Medium.ttf" );
		if ( fontPath != "" )
		{
			ImFontConfig config;
			config.OversampleH = 2;
			config.OversampleV = 1;
			config.GlyphExtraSpacing.x = 1.0f;

			// io.Fonts->AddFontFromFileTTF( fontPath.c_str(), 25.0f );
			/*ImFont* newFont = io.Fonts->AddFontFromFileTTF(fontPath.c_str(), 15.f, &config);
			bool built = io.Fonts->Build();
			if ( !built )
				LogWarn( "[Gui] ImGui Failed to Build Font\n" );*/

		}
	}
#endif

	if ( cmdline->Find( "-no-vgui-style" ) )
		return;

	// Classic VGUI2 Style Color Scheme
	ImVec4* colors = ImGui::GetStyle().Colors;

	colors[ImGuiCol_Text]                              = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
	colors[ImGuiCol_TextDisabled]              = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
	colors[ImGuiCol_WindowBg]                          = ImVec4(0.29f, 0.34f, 0.26f, 1.00f);
	colors[ImGuiCol_ChildBg]                                = ImVec4(0.29f, 0.34f, 0.26f, 1.00f);
	colors[ImGuiCol_PopupBg]                                = ImVec4(0.24f, 0.27f, 0.20f, 1.00f);
	colors[ImGuiCol_Border]                          = ImVec4(0.54f, 0.57f, 0.51f, 0.50f);
	colors[ImGuiCol_BorderShadow]              = ImVec4(0.14f, 0.16f, 0.11f, 0.52f);
	colors[ImGuiCol_FrameBg]                                = ImVec4(0.24f, 0.27f, 0.20f, 1.00f);
	colors[ImGuiCol_FrameBgHovered]          = ImVec4(0.27f, 0.30f, 0.23f, 1.00f);
	colors[ImGuiCol_FrameBgActive]            = ImVec4(0.30f, 0.34f, 0.26f, 1.00f);
	colors[ImGuiCol_TitleBg]                                = ImVec4(0.24f, 0.27f, 0.20f, 1.00f);
	colors[ImGuiCol_TitleBgActive]            = ImVec4(0.29f, 0.34f, 0.26f, 1.00f);
	colors[ImGuiCol_TitleBgCollapsed]          = ImVec4(0.00f, 0.00f, 0.00f, 0.51f);
	colors[ImGuiCol_MenuBarBg]                        = ImVec4(0.24f, 0.27f, 0.20f, 1.00f);
	colors[ImGuiCol_ScrollbarBg]                    = ImVec4(0.35f, 0.42f, 0.31f, 1.00f);
	colors[ImGuiCol_ScrollbarGrab]            = ImVec4(0.28f, 0.32f, 0.24f, 1.00f);
	colors[ImGuiCol_ScrollbarGrabHovered]   = ImVec4(0.25f, 0.30f, 0.22f, 1.00f);
	colors[ImGuiCol_ScrollbarGrabActive]    = ImVec4(0.23f, 0.27f, 0.21f, 1.00f);
	colors[ImGuiCol_CheckMark]                        = ImVec4(0.59f, 0.54f, 0.18f, 1.00f);
	colors[ImGuiCol_SliderGrab]                      = ImVec4(0.35f, 0.42f, 0.31f, 1.00f);
	colors[ImGuiCol_SliderGrabActive]          = ImVec4(0.54f, 0.57f, 0.51f, 0.50f);
	colors[ImGuiCol_Button]                          = ImVec4(0.29f, 0.34f, 0.26f, 0.40f);
	colors[ImGuiCol_ButtonHovered]            = ImVec4(0.35f, 0.42f, 0.31f, 1.00f);
	colors[ImGuiCol_ButtonActive]              = ImVec4(0.54f, 0.57f, 0.51f, 0.50f);
	colors[ImGuiCol_Header]                          = ImVec4(0.35f, 0.42f, 0.31f, 1.00f);
	colors[ImGuiCol_HeaderHovered]            = ImVec4(0.35f, 0.42f, 0.31f, 0.6f);
	colors[ImGuiCol_HeaderActive]              = ImVec4(0.54f, 0.57f, 0.51f, 0.50f);
	colors[ImGuiCol_Separator]                        = ImVec4(0.14f, 0.16f, 0.11f, 1.00f);
	colors[ImGuiCol_SeparatorHovered]          = ImVec4(0.54f, 0.57f, 0.51f, 1.00f);
	colors[ImGuiCol_SeparatorActive]                = ImVec4(0.59f, 0.54f, 0.18f, 1.00f);
	colors[ImGuiCol_ResizeGrip]                      = ImVec4(0.19f, 0.23f, 0.18f, 0.00f); // grip invis
	colors[ImGuiCol_ResizeGripHovered]        = ImVec4(0.54f, 0.57f, 0.51f, 1.00f);
	colors[ImGuiCol_ResizeGripActive]          = ImVec4(0.59f, 0.54f, 0.18f, 1.00f);
	colors[ImGuiCol_Tab]                                    = ImVec4(0.35f, 0.42f, 0.31f, 1.00f);
	colors[ImGuiCol_TabHovered]                      = ImVec4(0.54f, 0.57f, 0.51f, 0.78f);
	colors[ImGuiCol_TabActive]                        = ImVec4(0.59f, 0.54f, 0.18f, 1.00f);
	colors[ImGuiCol_TabUnfocused]              = ImVec4(0.24f, 0.27f, 0.20f, 1.00f);
	colors[ImGuiCol_TabUnfocusedActive]      = ImVec4(0.35f, 0.42f, 0.31f, 1.00f);
	//colors[ImGuiCol_DockingPreview]          = ImVec4(0.59f, 0.54f, 0.18f, 1.00f);
	//colors[ImGuiCol_DockingEmptyBg]          = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
	colors[ImGuiCol_PlotLines]                        = ImVec4(0.61f, 0.61f, 0.61f, 1.00f);
	colors[ImGuiCol_PlotLinesHovered]          = ImVec4(0.59f, 0.54f, 0.18f, 1.00f);
	colors[ImGuiCol_PlotHistogram]            = ImVec4(1.00f, 0.78f, 0.28f, 1.00f);
	colors[ImGuiCol_PlotHistogramHovered]   = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
	colors[ImGuiCol_TextSelectedBg]          = ImVec4(0.59f, 0.54f, 0.18f, 1.00f);
	colors[ImGuiCol_DragDropTarget]          = ImVec4(0.73f, 0.67f, 0.24f, 1.00f);
	colors[ImGuiCol_NavHighlight]              = ImVec4(0.59f, 0.54f, 0.18f, 1.00f);
	colors[ImGuiCol_NavWindowingHighlight]  = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
	colors[ImGuiCol_NavWindowingDimBg]        = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
	colors[ImGuiCol_ModalWindowDimBg]          = ImVec4(0.80f, 0.80f, 0.80f, 0.35f);

	ImGuiStyle& style = ImGui::GetStyle();
	style.FrameBorderSize = 1.0f;
	style.WindowRounding = 0.0f;
	style.ChildRounding = 0.0f;
	style.FrameRounding = 0.0f;
	style.PopupRounding = 0.0f;
	style.ScrollbarRounding = 0.0f;
	style.GrabRounding = 0.0f;
	style.TabRounding = 0.0f;
}

void GuiSystem::ShowConsole(  )
{
	aConsoleShown = !aConsoleShown;
}

bool GuiSystem::IsConsoleShown(  )
{
	return aConsoleShown;
}

void GuiSystem::AssignWindow( SDL_Window* spWindow )
{
	apWindow = spWindow;
}

constexpr int MAX_DEBUG_MESSAGE_SIZE = 512;

// Append a Debug Message
void GuiSystem::DebugMessage( const char* format, ... )
{
	va_list args;
	va_start(args, format);

	char buf[MAX_DEBUG_MESSAGE_SIZE];
	int len = vsnprintf( buf, MAX_DEBUG_MESSAGE_SIZE, format, args );
	aDebugMessages.push_back(buf);

	va_end(args);
}

// Place a Debug Message in a specific spot
void GuiSystem::DebugMessage( size_t index, const char* format, ... )
{
	aDebugMessages.resize( glm::max(aDebugMessages.size(), index + 1) );

	va_list args;
	va_start( args, format );

	char buf[MAX_DEBUG_MESSAGE_SIZE];
	int len = vsnprintf( buf, MAX_DEBUG_MESSAGE_SIZE, format, args );
	aDebugMessages[index] = buf;

	va_end( args );
}

// Insert a debug message in-between one
void GuiSystem::InsertDebugMessage( size_t index, const char* format, ... )
{
	va_list args;
	va_start( args, format );

	char buf[MAX_DEBUG_MESSAGE_SIZE];
	int len = vsnprintf( buf, MAX_DEBUG_MESSAGE_SIZE, format, args );

	aDebugMessages.insert( aDebugMessages.begin() + index, buf );
	va_end( args );
}

/*
*    Starts a new ImGui frame.
*/
void GuiSystem::StartFrame()
{
	if ( !apWindow )
		return;
		
	ImGui_ImplVulkan_NewFrame();
	ImGui_ImplSDL2_NewFrame( apWindow );
	ImGui::NewFrame();
}

void GuiSystem::Init()
{
	
}

GuiSystem::GuiSystem(  ) : BaseGuiSystem(  )
{
	systems->Add( this );
	gui = this;
}

GuiSystem::~GuiSystem(  )
{

}

