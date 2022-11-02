#pragma once

#include "system.h"
#include <glm/vec2.hpp>

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
	using Inputs = std::vector< SDL_Event >;
protected:
public:
	virtual const glm::ivec2& GetMouseDelta(  ) = 0;
	virtual const glm::ivec2& GetMousePos(  ) = 0;

	virtual bool WindowHasFocus(  ) = 0;

	/* Add a Key to the key update list.  */
	virtual void RegisterKey( SDL_Scancode key ) = 0;

	/* Get the current KeyState value.  */
	virtual KeyState GetKeyState( SDL_Scancode key ) = 0;

	/* Convienence functions  */
	virtual bool    KeyPressed( SDL_Scancode key ) = 0;
	virtual bool    KeyReleased( SDL_Scancode key ) = 0;
	virtual bool    KeyJustPressed( SDL_Scancode key ) = 0;
	virtual bool    KeyJustReleased( SDL_Scancode key ) = 0;
	/* Accessors.  */
	virtual Inputs *GetEvents() = 0;
};


#define IINPUTSYSTEM_NAME "InputSystem"
#define IINPUTSYSTEM_HASH typeid( BaseInputSystem ).hash_code()

