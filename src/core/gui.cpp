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
	{
		ImGui::Begin( "Developer Console" );
		static char buf[ 256 ] = "";

		if ( !wasConsoleOpen )
		{
			ImGui::SetKeyboardFocusHere(0);
		}
		ImGui::BeginChild("ConsoleOutput", ImVec2(256,256), true);
		ImGui::EndChild();
		if ( ImGui::InputText( "send", buf, 256, ImGuiInputTextFlags_EnterReturnsTrue ) )
		{
			apConsole->Add( buf );
			snprintf(buf, 256, "");
			ImGui::SetKeyboardFocusHere(0);
		}

		ImGui::End(  );
	}

	wasConsoleOpen = aConsoleShown;

	ImGuiWindowFlags devFlags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize;
	if ( !aConsoleShown )
		devFlags |= ImGuiWindowFlags_NoMove

        curtick = SDL_GetTicks();
	Uint32 delta = curtick - prevtick;;

	// NOTE: imgui framerate is a rough estimate, might wanna have our own fps counter?
	ImGui::Begin( "Dev Info", (bool*)0, devFlags );
        //ImGui::Text("%.1f FPS (%.3f ms/frame)", ImGui::GetIO().Framerate, 1000.0f / ImGui::GetIO().Framerate);
	ImGui::Text("%.1f FPS (%.3f ms/frame)", (float)1000 / delta, (float)delta);
	ImGui::End(  );
	prevtick = SDL_GetTicks();

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
