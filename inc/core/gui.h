#pragma once

#include <SDL2/SDL.h>

#include "../../inc/shared/system.h"

class GuiSystem : public BaseSystem
{
protected:
	SDL_Window 	*apWindow 	= NULL;
	bool 		aConsoleShown 	= false;
	bool		aDrawnFrame	= false;

	/* Draw the gui.  */
	void 		DrawGui(  );
	/* Shows the console window.  */
	void		ShowConsole(  );
	/* Initializes all commands the system can respond to.  */
	void 		InitCommands(  );

public:
	enum class	Commands{ NONE = 0, SHOW_CONSOLE, ASSIGN_WINDOW };
	/* Gets the window pointer from the renderer.  */
	void 		AssignWindow( SDL_Window* spWindow );
	/* Constructor.  */
	explicit	GuiSystem(  );
	/* A.  */
		        ~GuiSystem(  );
};
