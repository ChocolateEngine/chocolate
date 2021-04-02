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
	}
}
