#pragma once

#include "iinput.h"
#include "button_inputs.h"
#include <string>


#if CH_CLIENT
using InputContextHandle_t = int;

// Properties for the current input
struct InputContext_t
{
	bool aCapturesMouse    = false;
	bool aCapturesKeyboard = false;
};


#define INPUT_CONVAR( name, desc ) \
	CONVAR_RANGE_INT( name, 0, IN_CVAR_JUST_RELEASED, IN_CVAR_JUST_PRESSED, CVARF( INPUT ), desc );


void                 Input_Init();
void                 Input_Update();

void                 Input_CalcMouseDelta();
glm::vec2            Input_GetMouseDelta();

// could use some stack system to determine who gets to control the mouse scale maybe
void                 Input_SetMouseDeltaScale( const glm::vec2& scale );
const glm::vec2&     Input_GetMouseDeltaScale();

// Button Handling
ButtonInput_t        Input_RegisterButton();
ButtonInput_t        Input_GetButtonStates();

void                 Input_BindKey( EButton key, const std::string& cmd );
void                 Input_BindKey( SDL_Scancode key, const std::string& cmd );

//bool Input_KeyPressed( ButtonInput_t key );
//bool Input_KeyReleased( ButtonInput_t key );
//bool Input_KeyJustPressed( ButtonInput_t key );
//bool Input_KeyJustReleased( ButtonInput_t key );

InputContextHandle_t Input_CreateContext( InputContext_t sContext );
void                 Input_FreeContext( InputContextHandle_t sContextHandle );

InputContext_t&      Input_GetContextData( InputContext_t sContext );

// Push a context to the top of the stack, this will be the currently enabled context
// Returns true if the context was successfully pushed to the top of the stack
bool                 Input_PushContext( InputContextHandle_t sContextHandle );
void                 Input_PopContext();

// Is the currently enabled context the enabled one?
bool                 Input_IsCurrentContext( InputContextHandle_t sContextHandle );

#endif

