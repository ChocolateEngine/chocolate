#pragma once

#include <SDL2/SDL.h>

#include <vector>
#include <string>

#include "igui.h"

extern double gRealTime;

class GuiSystem : public IGuiSystem
{
   public:
	bool                       aConsoleShown    = true;
	bool                       aConVarListShown = false;

	std::vector< std::string > aDebugMessages;

	/* Draw the console */
	void                       DrawConsole( bool wasConsoleOpen, bool isChild = false ) override;

	/* ConVar List */
	void                       DrawConVarList( bool wasOpen );
	void                       InitConVarList();

	/* Set to VGUI Style 😎 */
	void    StyleImGui() override;

	/* Compile an ImGui font.  */
	ImFont* BuildFont( const char* spPath, float sSizePixels = 15.f, const ImFontConfig* spFontConfig = nullptr ) override;

	/* Per-Frame Update  */
	void    Update( float dt ) override;

	/* Shows the console window.  */
	void    ShowConsole() override;

	/* Is the console shown.  */
	bool    IsConsoleShown() override;

	// Shows the console window.
	void    ShowConVarList() override;

	// Is the console shown.
	bool    IsConVarListShown() override { return aConVarListShown; };

	/* Debug Text on the side of the screen  */
	void    DebugMessage( const char* format, ... ) override;
	void    DebugMessage( size_t index, const char* format, ... ) override;
	void    InsertDebugMessage( size_t index, const char* format, ... ) override;

	// Starts a new ImGui frame.
	void    StartFrame() override;

	void    InitConsole();
	bool    Init() override;
	void    Shutdown() override;

	/* Constructor.  */
	explicit GuiSystem();
	/* A.  */
	~GuiSystem();
};
	
extern GuiSystem* gui;
