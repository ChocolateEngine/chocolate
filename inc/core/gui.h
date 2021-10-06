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

	std::vector< std::string > aDebugMessages;

	/* Draw the gui.  */
	void 		DrawGui(  );
	/* Draw the console, move to it's own class later  */
	void 		DrawConsole( bool wasConsoleOpen );
	/* Initializes all commands the system can respond to.  */
	void 		InitCommands(  );

public:
	enum class	Commands{ NONE = 0, SHOW_CONSOLE, ASSIGN_WINDOW };

	/* Per-Frame Update  */
	void 		Update( float dt ) override;
	/* Gets the window pointer from the renderer.  */
	void 		AssignWindow( SDL_Window* spWindow ) override;
	/* Shows the console window.  */
	void		ShowConsole(  ) override;
	/* Is the console shown.  */
	bool		IsConsoleShown(  ) override;
	/* Debug Text on the side of the screen  */
	void		DebugMessage( size_t index, const char* format, ... ) override;
	void		InsertDebugMessage( size_t index, const char* format, ... ) override;

	/* Constructor.  */
	explicit	GuiSystem(  );
	/* A.  */
		        ~GuiSystem(  );
};
