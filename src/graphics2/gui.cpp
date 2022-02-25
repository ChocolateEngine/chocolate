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

	ImGui_ImplVulkan_NewFrame();
	ImGui_ImplSDL2_NewFrame( apWindow );
	ImGui::NewFrame();

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


bool CheckKeyPress( int& commandIndex, int maxSize, bool addTabInDownKey = false )
{
	bool upPressed = ImGui::IsKeyPressed( ImGui::GetKeyIndex(ImGuiKey_UpArrow) );
	bool downPressed = ImGui::IsKeyPressed( ImGui::GetKeyIndex(ImGuiKey_DownArrow) );

	if ( addTabInDownKey )
		downPressed |= ImGui::IsKeyPressed( ImGui::GetKeyIndex(ImGuiKey_Tab) );

	if ( upPressed && --commandIndex == -2 )
		commandIndex = maxSize - 1;

	if ( downPressed && ++commandIndex == maxSize)
		commandIndex = -1;

	return upPressed || downPressed;
}


bool CheckAddLastCommand( ImGuiInputTextCallbackData* data, Console* console, int& commandIndex )
{
	if ( console->GetCommandHistory().empty() )
		return false;

	bool keyPressed = CheckKeyPress( commandIndex, console->GetCommandHistory().size() );

	if ( keyPressed )
	{
		snprintf( data->Buf, 256, (commandIndex == -1) ? "" : console->GetCommandHistory()[commandIndex].c_str() );
		console->SetTextBuffer( data->Buf );
		data->CursorPos = console->GetTextBuffer().length();
	}

	return keyPressed;
}


bool CheckAddDropDownCommand( ImGuiInputTextCallbackData* data, Console* console, int& commandIndex )
{
	static std::string originalTextBuffer = "";
	static std::string prevTextBuffer = "";

	bool keyPressed = CheckKeyPress( commandIndex, console->GetAutoCompleteList().size(), true );

	// Reset the selected command index if the buffer is empty
	if ( strncmp(data->Buf, "", data->BufSize) == 0 && !keyPressed )
		commandIndex = -1;

	bool inDropDown = commandIndex != -1;
	bool textBufChanged = prevTextBuffer != data->Buf;

	// If the user typed something in, then update the original text inputted
	// relies on the if (inDropDown) below to set commandIndex back to -1 if there was something selected
	if ( textBufChanged && !inDropDown )
		originalTextBuffer = data->Buf;

	bool bufDirty = false;

	if ( keyPressed || textBufChanged )
	{
		if ( !inDropDown )
		{
			// No items selected in auto complete list
			// Set back to original text and recalculate the auto complete list
			snprintf( data->Buf, data->BufSize, originalTextBuffer.c_str() );
			console->CalculateAutoCompleteList( data->Buf );
			bufDirty = true;
		}
		else if ( keyPressed )
		{
			// An arrow key or tab is pressed, so fill the buffer with the selected item from the auto complete list
			//snprintf( data->Buf, data->BufSize, console->GetAutoCompleteList()[commandIndex].c_str() );
			snprintf( data->Buf, data->BufSize, (console->GetAutoCompleteList()[commandIndex] + " ").c_str() );
			bufDirty = true;
		}
		else if ( inDropDown )
		{
			// An item in auto complete list is selected, but the user typed something in
			// So set back to original text recalculate the auto complete list
			commandIndex = -1;
			originalTextBuffer = data->Buf;
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

		prevTextBuffer = console->GetTextBuffer();
	}

	return bufDirty;
}


bool CheckEnterPress( char* buf, Console* console, int& lastCommandIndex, int& dropDownCommandIndex )
{
	bool isPressed = ImGui::IsKeyPressed( ImGui::GetKeyIndex(ImGuiKey_Enter), false );
	isPressed |= ImGui::IsKeyPressed( ImGui::GetKeyIndex(ImGuiKey_KeyPadEnter), false );

	if ( isPressed )
	{
		console->Add( buf );
		snprintf( buf, 256, "" );
		console->SetTextBuffer( buf );
		ImGui::SetKeyboardFocusHere(  );
		lastCommandIndex = -1;
		dropDownCommandIndex = -1;
	}

	return isPressed;
}


// blech, needed for selecting it in the dropdown
static int gDropDownCommandIndex = -1;


int ConsoleInputCallback( ImGuiInputTextCallbackData* data )
{
	static int lastCommandIndex = -1;

	data->BufDirty = CheckAddDropDownCommand( data, console, gDropDownCommandIndex );

	if ( console->GetAutoCompleteList().empty() )
	{
		data->BufDirty = CheckAddLastCommand( data, console, lastCommandIndex );
	}

	if ( !data->BufDirty )
	{
		data->BufDirty = CheckEnterPress( data->Buf, console, lastCommandIndex, gDropDownCommandIndex );
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
		Print( "TEST\n" );

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

	ImGui::TextUnformatted( console->GetConsoleHistoryStr( con_gui_max_history.GetFloat() ).c_str() );

	if ( scrollMax == ImGui::GetScrollY() )
		ImGui::SetScrollY( ImGui::GetScrollMaxY() );

	scrollMax = ImGui::GetScrollMaxY();
	ImGui::EndChild();

	snprintf( buf, 256, console->GetTextBuffer(  ).c_str() );
	ImGui::InputText( "send", buf, 256, ImGuiInputTextFlags_CallbackAlways, &ConsoleInputCallback );

	ImVec2 dropDownPos( ImGui::GetWindowPos().x, ImGui::GetWindowSize().y + ImGui::GetWindowPos().y );

	ImGui::End(  );

	std::vector< std::string > cvarAutoComplete = console->GetAutoCompleteList();

	if ( !cvarAutoComplete.empty() )
	// if ( 0 )
	{
		//ImGui::BeginPopup( "cvars_modal", ImGuiWindowFlags_Popup );
		//ImGui::BeginPopupModal( "cvars_modal" );

		ImGui::SetNextWindowPos( dropDownPos, ImGuiCond_Always, ImVec2(0.0f, 0.0f));

		ImGui::PushStyleVar( ImGuiStyleVar_WindowPadding, ImVec2(0,0) );

		ImGui::Begin( "cvars_box", NULL,
					 ImGuiWindowFlags_NoBringToFrontOnFocus |
					 ImGuiWindowFlags_AlwaysAutoResize |
					 ImGuiWindowFlags_NoBackground |
					 ImGuiWindowFlags_NoDecoration |
					 ImGuiWindowFlags_NoTitleBar |
					 ImGuiWindowFlags_NoMove |
					 //ImGuiWindowFlags_NoNavFocus |
					 ImGuiWindowFlags_NoFocusOnAppearing );

		ImGui::BeginListBox( "" );

		for ( int i = 0; i < cvarAutoComplete.size(); i++ )
		{
			std::string item = cvarAutoComplete[i];

			item += " " + console->GetConVarValue( item );

			if ( ImGui::Selectable( item.c_str(), gDropDownCommandIndex == i ) )
			{
				// should we keep the value in here too?
				console->SetTextBuffer( cvarAutoComplete[i] );
				//break;
			}
		}

		ImGui::EndListBox(  );

		ImGui::End(  );

		ImGui::PopStyleVar();

		//ImGui::EndPopup(  );
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

void GuiSystem::AssignWindow( SDL_Window* spWindow )
{
	apWindow = spWindow;
}

// Append a Debug Message
void GuiSystem::DebugMessage( const char* format, ... )
{
	va_list args;
	va_start(args, format);
	aDebugMessages.push_back( vstring( format, args ) );
	va_end(args);
}

// Place a Debug Message in a specific spot
void GuiSystem::DebugMessage( size_t index, const char* format, ... )
{
	aDebugMessages.resize( glm::max(aDebugMessages.size(), index + 1) );
	VSTRING( aDebugMessages[index], format );
}

// Insert a debug message in-between one
void GuiSystem::InsertDebugMessage( size_t index, const char* format, ... )
{
	va_list args;
	va_start( args, format );
	aDebugMessages.insert( aDebugMessages.begin() + index, vstring( format, args ) );
	va_end( args );
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

