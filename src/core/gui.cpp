#include "../../inc/core/gui.h"
#include "../../inc/core/renderer/renderer.h"

#include "../../inc/imgui/imgui.h"
#include "../../inc/imgui/imgui_impl_vulkan.h"
#include "../../inc/imgui/imgui_impl_sdl.h"

void GuiSystem::DrawGui(  )
{
	if ( !apWindow )
		return;
	ImGui_ImplVulkan_NewFrame(  );
	ImGui_ImplSDL2_NewFrame( apWindow );
	ImGui::NewFrame(  );
	if ( !aDrawnFrame )
		apCommandManager->Execute( Renderer::Commands::IMGUI_INITIALIZED );
	if ( aConsoleShown )
	{
		ImGui::Begin( "Developer Console" );
		static char buf[ 256 ] = "";
		if ( ImGui::InputText( "send", buf, 256, ImGuiInputTextFlags_EnterReturnsTrue ) )
			apConsole->Add( buf );
		ImGui::End(  );
	}
	aDrawnFrame = true;
}

void GuiSystem::ShowConsole(  )
{
	aConsoleShown = !aConsoleShown;
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

GuiSystem::GuiSystem(  ) : BaseSystem(  )
{
	aSystemType = GUI_C;
        AddUpdateFunction( [ & ](  ){ DrawGui(  ); } );
}

GuiSystem::~GuiSystem(  )
{

}
