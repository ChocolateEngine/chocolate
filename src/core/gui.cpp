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

	if ( !aDrawnFrame )
		apCommandManager->Execute( Renderer::Commands::IMGUI_INITIALIZED );

	if ( aConsoleShown )
	{
		ImGui::Begin( "Developer Console" );
		static char buf[ 256 ] = "";

		if ( !wasConsoleOpen )
		{
			ImGui::SetKeyboardFocusHere(0);
		}

		if ( ImGui::InputText( "send", buf, 256, ImGuiInputTextFlags_EnterReturnsTrue ) )
		{
			apConsole->Add( buf );
			snprintf(buf, 256, "");
			ImGui::SetKeyboardFocusHere(0);
		}

		ImGui::End(  );
	}

	wasConsoleOpen = aConsoleShown;

	// ImGui::Begin( "Dev Info", (bool*)0, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar );
	ImGui::Begin( "Dev Info", (bool*)0, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoScrollbar );
	ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
	ImGui::End(  );

	aDrawnFrame = true;
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
