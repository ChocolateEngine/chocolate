#include "../../inc/core/gui.h"
#include "../../inc/core/renderer/renderer.h"

#include "../../inc/imgui/imgui.h"
#include "../../inc/imgui/imgui_impl_vulkan.h"
#include "../../inc/imgui/imgui_impl_sdl.h"

void GuiSystem::Update( float dt )
{
	BaseSystem::Update( dt );

	DrawGui(  );
}

void GuiSystem::DrawGui(  )
{
	if ( !apWindow )
		return;

	ImGui_ImplVulkan_NewFrame(  );
	ImGui_ImplSDL2_NewFrame( apWindow );
	ImGui::NewFrame(  );

	static bool wasConsoleOpen = false;
	static Uint32 prevtick = 0;
	static Uint32 curtick = 0;

	if ( !aDrawnFrame )
		apCommandManager->Execute( Renderer::Commands::IMGUI_INITIALIZED );

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
	ImGui::Text("%.1f FPS (%.3f ms/frame)", ImGui::GetIO().Framerate, 1000.0f / ImGui::GetIO().Framerate);
	//ImGui::Text("%.1f FPS (%.3f ms/frame)", 1000.f / delta, (float)delta);  // ms/frame is inaccurate
	ImGui::End(  );
	prevtick = SDL_GetTicks();

	aDrawnFrame = true;
}


bool CheckAddLastCommand( ImGuiInputTextCallbackData* data, Console* console, int& lastCommandIndex )
{
	if ( console->GetCommandHistory().empty() )
		return false;

	bool upPressed = ImGui::IsKeyPressed( ImGui::GetKeyIndex(ImGuiKey_UpArrow) );
	bool downPressed = ImGui::IsKeyPressed( ImGui::GetKeyIndex(ImGuiKey_DownArrow) );

	if ( upPressed )
	{
		lastCommandIndex--;

		if ( lastCommandIndex == -2 )
			lastCommandIndex = console->GetCommandHistory().size() - 1;
	}

	if ( downPressed )
	{
		lastCommandIndex++;

		if ( lastCommandIndex == console->GetCommandHistory().size() )
			lastCommandIndex = -1;
	}

	if ( upPressed || downPressed )
	{
		snprintf( data->Buf, 256, (lastCommandIndex == -1) ? "" : console->GetCommandHistory()[lastCommandIndex].c_str() );
		data->CursorPos = std::string(data->Buf).length();
	}

	return upPressed || downPressed;
}


bool CheckEnterPress( char* buf, Console* console, int& lastCommandIndex )
{
	bool isPressed = ImGui::IsKeyPressed( ImGui::GetKeyIndex(ImGuiKey_Enter), false );

	if ( isPressed )
	{
		console->Add( buf );
		snprintf( buf, 256, "" );
		ImGui::SetKeyboardFocusHere(  );
		lastCommandIndex = -1;
	}

	return isPressed;
}


int ConsoleInputCallback( ImGuiInputTextCallbackData* data )
{
	static int lastCommandIndex = -1;

	Console* console = (Console*)data->UserData;

	data->BufDirty = CheckAddLastCommand( data, console, lastCommandIndex );

	if ( !data->BufDirty )
		data->BufDirty = CheckEnterPress( data->Buf, console, lastCommandIndex );

	console->SetTextBuffer( data->Buf );
	data->BufTextLen = console->GetTextBuffer().length();

	return 0;
}


void GuiSystem::DrawConsole( bool wasConsoleOpen )
{
	// blech
	static bool wasUpArrowPressed = false;
	//static bool wasUpArrowPressed = false;

	static int lastCommandIndex = 0;

	ImGui::Begin( "Developer Console" );
	static char buf[ 256 ] = "";

	if ( !wasConsoleOpen )
		ImGui::SetKeyboardFocusHere(0);

	ImGui::BeginChild("ConsoleOutput", ImVec2( ImGui::GetWindowSize().x - 32, ImGui::GetWindowSize().y - 64 ), true);
	ImGui::Text( apConsole->GetConsoleHistory().c_str() );
	ImGui::EndChild();

	snprintf( buf, 256, apConsole->GetTextBuffer(  ).c_str() );
	ImGui::InputText( "send", buf, 256, ImGuiInputTextFlags_CallbackAlways, &ConsoleInputCallback, apConsole );

	ImVec2 dropDownPos( ImGui::GetWindowPos().x, ImGui::GetWindowSize().y + ImGui::GetWindowPos().y );

	ImGui::End(  );

	std::vector< std::string > cvarAutoComplete = apConsole->GetAutoCompleteList();

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

		for ( std::string item: cvarAutoComplete )
		{
			if ( ImGui::Selectable( item.c_str(), false ) )
			{
				apConsole->SetTextBuffer( item );
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

void GuiSystem::InitCommands(  )
{
	auto showConsole 	= std::bind( &GuiSystem::ShowConsole, this );
	apCommandManager->Add( GuiSystem::Commands::SHOW_CONSOLE, Command<  >( showConsole ) );
	auto assignWin 		= std::bind( &GuiSystem::AssignWindow, this, std::placeholders::_1 );
	apCommandManager->Add( GuiSystem::Commands::ASSIGN_WINDOW, Command< SDL_Window* >( assignWin ) );
}

void GuiSystem::AssignWindow( SDL_Window* spWindow )
{
	apWindow = spWindow;
}

GuiSystem::GuiSystem(  ) : BaseGuiSystem(  )
{
	aSystemType = GUI_C;
}

GuiSystem::~GuiSystem(  )
{

}
