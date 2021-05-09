#include "../../inc/core/input.h"

void input_c::parse_input
	(  )
{
	for ( ; SDL_PollEvent( &event ) ; )
	{
		if ( event.type == SDL_QUIT )
		{
			msgs->add( { ENGINE_C, ENGI_EXIT, NULL } );
		}
		if ( event.type == SDL_KEYDOWN )
		{
			switch ( event.key.keysym.sym )
			{
				case SDLK_UP:
				{
					void* args[  ] = { ( void* )"materials/models/protogen_wip_5_plus_protodal.obj", ( void* )"materials/textures/blank_mat.png" };
					msgs->add( { GRAPHICS_C,
						     GFIX_LOAD_MODEL,
						     NULL,
						     args,
						     2
						} );
				}
			}
		}
	}
}
