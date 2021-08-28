#pragma once

#include <SDL2/SDL.h>

#include "../../inc/shared/system.h"
#include "../../inc/shared/basegui.h"

class GuiSystem : public BaseGuiSystem
{
protected:
	SDL_Window 	*apWindow 	= NULL;
	bool 		aConsoleShown 	= true;
	bool		aDrawnFrame	= false;
	bool		aCursorLocked	= false;

	/* Draw the gui.  */
	void 		DrawGui(  );
	/* Initializes all commands the system can respond to.  */
	void 		InitCommands(  );

public:
	enum class	Commands{ NONE = 0, SHOW_CONSOLE, ASSIGN_WINDOW };
	/* Per-Frame Update  */
	void 		Update( float dt );
	/* Gets the window pointer from the renderer.  */
	void 		AssignWindow( SDL_Window* spWindow );
	/* Shows the console window.  */
	void		ShowConsole(  );
	/* Is the console shown.  */
	bool		IsConsoleShown(  );
	/* Constructor.  */
	explicit	GuiSystem(  );
	/* A.  */
		        ~GuiSystem(  );
};
