/*
input.cpp ( Authored by p0lyh3dron )

Defines the methods declared in
input.h
*/
#include "input.h"
#include "imgui/imgui.h"
#include "imgui/imgui_impl_sdl.h"

#include "igui.h"

#include "util.h"

#include <fstream>
#include <iostream>

extern "C"
{
	void ParseInput(  )
	{
		static BaseGuiSystem* gui = GET_SYSTEM( BaseGuiSystem );

		aEvents.clear(  );

		for ( ; SDL_PollEvent( &aEvent ) ; )
		{
			aEvents.push_back( aEvent );
			ImGui_ImplSDL2_ProcessEvent( &aEvent );

			switch (aEvent.type)
			{
			case SDL_QUIT:
			{
				// tell engine to quit somehow
				break;
			}

			case SDL_MOUSEMOTION:
			{
				aMousePos.x = aEvent.motion.x;
				aMousePos.y = aEvent.motion.y;
				aMouseDelta.x += aEvent.motion.xrel;
				aMouseDelta.y += aEvent.motion.yrel;
				break;
			}

			case SDL_WINDOWEVENT:
			{
				switch (aEvent.window.event)
				{
				case SDL_WINDOWEVENT_FOCUS_GAINED:
				{
					aHasFocus = true;
					break;
				}

				case SDL_WINDOWEVENT_FOCUS_LOST:
				{
					aHasFocus = false;
					break;
				}
				}
				break;
			}
			
			}

			for ( BaseSystem* sys: systems->GetSystemList() )
			{
				sys->HandleSDLEvent( &aEvent );
			}
		}
	}


	void InitInput(  )
	{
		ResetInputs(  );
	}


	void InputUpdate( float frameTime )
	{
		ResetInputs(  );
		ParseInput(  );
		UpdateKeyStates(  );
	}

	void ResetInputs(  )
	{
		aMouseDelta = {0, 0};

		aKeyboardState = SDL_GetKeyboardState( NULL );
	}

	const glm::ivec2& GetMouseDelta(  )
	{
		return aMouseDelta;
	}

	const glm::ivec2& GetMousePos(  )
	{
		return aMousePos;
	}

	bool WindowHasFocus(  )
	{
		return aHasFocus;
	}

	void UpdateKeyStates(  )
	{
		for ( auto const& [key, val] : aKeyStates )
			UpdateKeyState( key );
	}

	void UpdateKeyState( SDL_Scancode key )
	{
		bool pressed = aKeyboardState[key];
		KeyState& state = aKeyStates[key];

		if ( state & KeyState_Pressed )
		{
			if ( state & KeyState_JustPressed )
				state &= ~KeyState_JustPressed;

			if ( !pressed )
			{
				state |= KeyState_Released | KeyState_JustReleased;
				state &= ~(KeyState_Pressed | KeyState_JustPressed);
			}
		}
		else if ( state & KeyState_Released )
		{
			if ( state & KeyState_JustReleased )
				state &= ~KeyState_JustReleased;

			if ( pressed )
			{
				state |= KeyState_Pressed | KeyState_JustPressed;
				state &= ~(KeyState_Released | KeyState_JustReleased);
			}
		}
		else
		{
			state |= KeyState_Released | KeyState_JustReleased;
			state &= ~(KeyState_Pressed | KeyState_JustPressed);
		}
	}

	void RegisterKey( SDL_Scancode key )
	{
		KeyStates::const_iterator state = aKeyStates.find( key );

		// Already registered
		if ( state != aKeyStates.end() )
			return;

		// aKeyStates[key] = aKeyboardState[key] ? KeyState::JustPressed : KeyState::Released;
		aKeyStates[key] = KeyState_Invalid;

		UpdateKeyState( key );
	}

	KeyState GetKeyState( SDL_Scancode key )
	{
		KeyStates::const_iterator state = aKeyStates.find( key );

		if ( state == aKeyStates.end() )
		{
			// Try to register this key
			RegisterKey( key );

			state = aKeyStates.find( key );

			if ( state == aKeyStates.end() )
			{
				// would be odd if this got hit
				Print( "[Input System] Invalid Key: \"%s\"\n", SDL_GetKeyName( SDL_GetKeyFromScancode(key) ) );
				return KeyState_Invalid;
			}
		}

		return state->second;
	}

        std::vector< SDL_Event >* GetEvents(  )
	{
		return &aEvents;
	}
}
