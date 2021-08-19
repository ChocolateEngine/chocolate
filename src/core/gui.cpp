#include "../../inc/core/gui.h"

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
	if ( aConsoleShown )
	{
		ImGui::Begin( "Developer Console" );
		static char buf[ 256 ] = "";
		if ( ImGui::InputText( "send", buf, 256, ImGuiInputTextFlags_EnterReturnsTrue ) )
			apConsole->add( buf );
		ImGui::End(  );
	}
}

void GuiSystem::ShowConsole(  )
{
	aConsoleShown = !aConsoleShown;
}

void GuiSystem::InitCommands(  )
{
	auto showConsole 	= std::bind( &GuiSystem::ShowConsole, this );
	apMsgs->aCmdManager.Add( "_show_console", Command<  >( showConsole ) );
	auto assignWin 		= std::bind( &GuiSystem::AssignWindow, this, std::placeholders::_1 );
	apMsgs->aCmdManager.Add( "_assign_win", Command< SDL_Window* >( assignWin ) );
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
	
