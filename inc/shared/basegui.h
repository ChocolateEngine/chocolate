#pragma once

#include <SDL2/SDL.h>

#include "../../inc/shared/system.h"


class BaseGuiSystem : public BaseSystem
{
public:
	/* Gets the window pointer from the renderer.  */
	virtual void 		AssignWindow( SDL_Window* spWindow ) = 0;
	/* Shows the console window.  */
	virtual void		ShowConsole(  ) = 0;
	/* Is the console shown.  */
	virtual bool		IsConsoleShown(  ) = 0;
	/* Debug Text on the side of the screen  */
	virtual void		DebugMessage( size_t index, const char* format, ... ) = 0;
	virtual void		InsertDebugMessage( size_t index, const char* format, ... ) = 0;
};

