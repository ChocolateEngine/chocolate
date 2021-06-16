#ifndef INPUT_H
#define INPUT_H

#include "system.h"

#include <SDL2/SDL.h>

class input_c : public system_c
{
	protected:

	SDL_Event event;
	
	public:

	void parse_input
		(  );

	input_c
		(  );
};

#endif
