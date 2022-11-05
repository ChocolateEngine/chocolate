#pragma once

#include "system.h"

struct ImFont;
struct ImFontConfig;


class BaseGuiSystem : public BaseSystem
{
public:
	/* Compile an ImGui font.  */
	// virtual ImFont* 	BuildFont( const char* spPath, float sSizePixels = 15.f, const ImFontConfig* spFontConfig = nullptr , const ImWchar* spGlyphRanges = nullptr ) = 0;
	virtual ImFont* 	BuildFont( const char* spPath, float sSizePixels = 15.f, const ImFontConfig* spFontConfig = nullptr ) = 0;

	/* Shows the console window.  */
	virtual void		ShowConsole() = 0;
	
	/* Is the console shown.  */
	virtual bool		IsConsoleShown() = 0;
	
	/* Debug Text on the side of the screen  */
	virtual void		DebugMessage( const char* format, ... ) = 0;
	virtual void		DebugMessage( size_t index, const char* format, ... ) = 0;
	virtual void		InsertDebugMessage( size_t index, const char* format, ... ) = 0;
	
	/*
	 *    Starts a new ImGui frame.
	 */	
	virtual void 		StartFrame() = 0;
};


#define IGUI_NAME "GUI"
#define IGUI_HASH 1

