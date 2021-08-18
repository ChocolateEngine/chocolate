#ifndef GUI_H
#define GUI_H

#include <SDL2/SDL.h>

#include "../../inc/shared/system.h"

class gui_c : public system_c
{
	protected:

	SDL_Window* win = NULL;
	bool consoleShown = false;

	void init_commands
		(  );
	void draw_gui
		(  );
	void		ShowConsole( int sArgs );
	
	public:

	void assign_win
		( SDL_Window* window );

	gui_c
		(  );

};

#endif
