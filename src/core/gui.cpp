#include "../../inc/core/gui.h"

#include "../../inc/imgui/imgui.h"
#include "../../inc/imgui/imgui_impl_vulkan.h"
#include "../../inc/imgui/imgui_impl_sdl.h"

void gui_c::init_commands
	(  )
{
	msg_s msg;
	msg.type = GUI_C;

	msg.msg = LOAD_IMGUI_DEMO;
	msg.func = [ & ]( std::vector< std::any > args ){ consoleShown = !consoleShown; };
	engineCommands.push_back( msg );

	msg.msg = ASSIGN_WIN;
	msg.func = [ & ]( std::vector< std::any > args )
		{
			if ( args.size(  ) < 1 )
			{
				printf( "Not enough arguments for ASSIGN_WIN\n" );
				return;
			}
			assign_win( std::any_cast< SDL_Window* >( args[ 0 ] ) );
			msgs->add( RENDERER_C, IMGUI_INITIALIZED );
	};
	engineCommands.push_back( msg );
}

void gui_c::draw_gui
	(  )
{
	if ( !win )
	{
		return;
	}
	ImGui_ImplVulkan_NewFrame(  );
	ImGui_ImplSDL2_NewFrame( win );
	ImGui::NewFrame(  );
	if ( consoleShown )
	{
		ImGui::Begin( "Developer Console" );
		static char buf[ 256 ] = "";
		if ( ImGui::InputText( "send", buf, 256, ImGuiInputTextFlags_EnterReturnsTrue ) )
		{
			console->add( buf );
		}
		ImGui::End(  );
	}
}

void gui_c::assign_win
	( SDL_Window* window )
{
	win = window;
	PublishedFunction function{  };
	auto pFunctionPointer = std::bind( &gui_c::ShowConsole, this, std::placeholders::_1 );
	msgs->aCmdManager.Add( "ShowConsole", Command< int >( pFunctionPointer ) );
	msgs->aCmdManager.Execute( "ShowConsole", 69 );
}

void gui_c::ShowConsole( int sArgs )
{
	consoleShown = !consoleShown;
	printf( "%d\n", sArgs );
}

gui_c::gui_c
	(  )
{
	systemType = GUI_C;
	add_func( [ & ](  ){ draw_gui(  ); } );

	init_commands(  );
}
	
