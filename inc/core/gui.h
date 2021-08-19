#pragma once

#include <SDL2/SDL.h>

#include "../../inc/shared/system.h"

class GuiSystem : public BaseSystem
{
	SYSTEM_OBJECT( GuiSystem )
protected:
	SDL_Window 	*apWindow 	= NULL;
	bool 		aConsoleShown 	= false;

	/* Draw the gui.  */
	void 		DrawGui(  );
	/* Shows the console window.  */
	void		ShowConsole(  );
public:
	/* Gets the window pointer from the renderer.  */
	void 		AssignWindow( SDL_Window* spWindow );
	/* Constructor.  */
	explicit	GuiSystem(  );

};
