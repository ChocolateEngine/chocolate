#pragma once

#include <SDL2/SDL.h>

#include "system.h"
#include "igui.h"

extern "C"
{
/* Draw the gui.  */
	void 		DrawGui(  );
/* Draw the console, move to it's own class later  */
	void 		DrawConsole( bool wasConsoleOpen );
/* Shows the console window.  */
	void		ShowConsole(  );
/* Is the console shown.  */
	bool		IsConsoleShown(  );
/* Debug Text on the side of the screen  */
	void		DebugMessage( size_t index, const char* format, ... );
	void		InsertDebugMessage( size_t index, const char* format, ... );
}
