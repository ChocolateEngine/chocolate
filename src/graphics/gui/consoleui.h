#pragma once

#include <SDL2/SDL.h>

#include "system.h"
#include "igui.h"

class GuiSystem : public BaseGuiSystem
{
protected:
	SDL_Window 	*apWindow 	= NULL;
	bool 		aConsoleShown 	= true;
	bool		aDrawnFrame	= false;
	bool		aCursorLocked	= false;

	std::vector< std::string > aDebugMessages;

	/* Draw the gui.  */
	void 		DrawGui( float sDT );
	/* Draw the console, move to it's own class later  */
	void 		DrawConsole( bool wasConsoleOpen );

public:
	/* Set to VGUI Style 😎 */
	void 		StyleImGui();

	/* Set to VGUI Style 😎 */
	void 		StyleImGui();

	/* Per-Frame Update  */
	void 		Update( float dt ) override;
	/* Gets the window pointer from the renderer.  */
	void 		AssignWindow( SDL_Window* spWindow ) override;
	/* Shows the console window.  */
	void		ShowConsole(  ) override;
	/* Is the console shown.  */
	bool		IsConsoleShown(  ) override;
	/* Debug Text on the side of the screen  */
	void		DebugMessage( const char* format, ... ) override;
	void		DebugMessage( size_t index, const char* format, ... ) override;
	void		InsertDebugMessage( size_t index, const char* format, ... ) override;
	/*
	 *    Starts a new ImGui frame.
	 */
	void 		StartFrame() override;

	void            Init();

	/* Constructor.  */
	explicit	GuiSystem(  );
	/* A.  */
		        ~GuiSystem(  );
};
