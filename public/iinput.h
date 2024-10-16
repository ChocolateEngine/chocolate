#pragma once

#include "system.h"
#include <glm/vec2.hpp>

#include <SDL2/SDL_scancode.h>


// Bit Shifts to allow the state to have both JustPressed and Pressed
enum
{
	KeyState_Invalid      = 0,
	KeyState_Released     = 1,
	KeyState_Pressed      = 2,
	KeyState_JustReleased = 4,
	KeyState_JustPressed  = 8,
};

enum : s8
{
	KeyState2_JustReleased = -1,
	KeyState2_Released     = 0,
	KeyState2_Pressed      = 1,
	KeyState2_JustPressed  = 2,
};

typedef unsigned char KeyState;


enum EButton
{
	// Add mouse buttons to the SDL scancode list
	EButton_BeforeMouse = SDL_SCANCODE_ENDCALL,

	EButton_MouseLeft,
	EButton_MouseRight,
	EButton_MouseMiddle,
	EButton_MouseX1,
	EButton_MouseX2,

	// a bit strange how i have these as "buttons", but oh well
	EButton_MouseWheelUp,
	EButton_MouseWheelDown,
	EButton_MouseWheelLeft,
	EButton_MouseWheelRight,

	EButton_Count,
};


class IInputSystem : public ISystem
{
	using Inputs = std::vector< SDL_Event >;
protected:
   public:
	virtual const glm::ivec2& GetMouseDelta()                                       = 0;
	virtual const glm::ivec2& GetMousePos()                                         = 0;
	virtual glm::ivec2        GetMouseScroll()                                      = 0;

	virtual bool              WindowHasFocus()                                      = 0;
	virtual bool              WindowHasFocus( SDL_Window* window )                  = 0;
	virtual void              SetWindowFocused( SDL_Window* window )                = 0;

	/* Add a Key to the key update list.  */
	virtual bool              RegisterKey( EButton key )                            = 0;

	/* Get the display name for this key  */
	virtual const char*       GetKeyName( EButton key )                             = 0;
	virtual const char*       GetKeyDisplayName( EButton key )                      = 0;
	virtual EButton           GetKeyFromName( std::string_view sName )              = 0;

	/* Get the current KeyState value.  */
	virtual KeyState          GetKeyState( EButton key )                            = 0;

	/* Convienence functions  */
	virtual bool              KeyPressed( EButton key )                             = 0;
	virtual bool              KeyReleased( EButton key )                            = 0;
	virtual bool              KeyJustPressed( EButton key )                         = 0;
	virtual bool              KeyJustReleased( EButton key )                        = 0;

	virtual void              AddWindow( SDL_Window* sWindow, void* sImGuiContext ) = 0;
	virtual void              RemoveWindow( SDL_Window* sWindow )                   = 0;
	virtual bool              SetCurrentWindow( SDL_Window* window )                = 0;
	virtual SDL_Window*       GetCurrentWindow()                                    = 0;


	/* Accessors.  */
	virtual Inputs *GetEvents() = 0;
};


#define IINPUTSYSTEM_NAME "InputSystem"
#define IINPUTSYSTEM_VER 2

