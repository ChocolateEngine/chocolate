#pragma once

#include "../shared/system.h"
#include "../glm/vec2.hpp"

#include <SDL2/SDL_scancode.h>


// Bit Shifts to allow the state to have both JustPressed and Pressed
enum
{
	KeyState_Invalid = 0,
	KeyState_Released = 1,
	KeyState_Pressed = 2,
	KeyState_JustReleased = 4,
	KeyState_JustPressed = 8,
};

typedef unsigned char KeyState;


class BaseInputSystem : public BaseSystem
{
public:

	virtual const glm::vec2& GetMouseDelta(  ) = 0;
	virtual const glm::vec2& GetMousePos(  ) = 0;

	virtual bool WindowHasFocus(  ) = 0;

	/* Add a Key to the key update list.  */
	virtual void RegisterKey( SDL_Scancode key ) = 0;

	/* Get the current KeyState value.  */
	virtual KeyState GetKeyState( SDL_Scancode key ) = 0;

	/* Convienence functions  */
	virtual bool IsKeyPressed( SDL_Scancode key ) = 0;
	virtual bool IsKeyJustPressed( SDL_Scancode key ) = 0;
	virtual bool IsKeyJustReleased( SDL_Scancode key ) = 0;
};

