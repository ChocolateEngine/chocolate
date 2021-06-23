#include "../../inc/core/input.h"
#include "../../inc/imgui/imgui.h"
#include "../../inc/imgui/imgui_impl_sdl.h"

void input_c::parse_input
	(  )
{
	for ( ; SDL_PollEvent( &event ) ; )
	{
		ImGui_ImplSDL2_ProcessEvent( &event );
		if ( event.type == SDL_QUIT )
		{
			msgs->add( ENGINE_C, ENGI_EXIT );
		}
		if ( event.type == SDL_KEYDOWN )
		{
			switch ( event.key.keysym.sym )
			{
				case SDLK_UP:
				{
					/*void* args[  ] = { ( void* )"materials/models/protogen_wip_5_plus_protodal.obj", ( void* )"materials/textures/blue_mat.png" };
					msgs->add( GRAPHICS_C,
						   GFIX_LOAD_MODEL,
						   0,
						   2,
						   args );*/
					msgs->add( 0, 0, FLAGS_EXTERNAL_SYSTEM );
					break;
				}
				case SDLK_BACKQUOTE:
				{
					msgs->add( GUI_C, LOAD_IMGUI_DEMO );
				}
			}
		}
	}
}

input_c::input_c
	(  )
{
	add_func( [ & ](  ){ parse_input(  ); } );
}
