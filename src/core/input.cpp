/*
input.cpp ( Authored by p0lyh3dron )

Defines the methods declared in
input.h
*/
#include "../../inc/core/input.h"
#include "../../inc/imgui/imgui.h"
#include "../../inc/imgui/imgui_impl_sdl.h"

#include "../../inc/core/engine.h"
#include "../../inc/core/gui.h"

#include <fstream>
#include <iostream>

InputSystem::InputSystem(  ) : BaseInputSystem(  )
{
	aSystemType = INPUT_C;

	MakeAliases(  );
	ParseBindings(  );
}

void InputSystem::MakeAliases(  )
{
	std::ifstream 	in( "resource/SDL_Alias_List.txt" );
	if ( !in.is_open(  ) )
		return;
	std::string 	alias;
        int 		scanCode;
	while ( in >> alias >> scanCode )
	{
	        KeyAlias 	key{  };
		key.aAlias 	= alias;
		key.aCode 	= SDL_SCANCODE_TO_KEYCODE( scanCode );
		aKeyAliases.push_back( key );

		std::cout << alias << " using scanCode " << scanCode << std::endl;
		in >> alias;	//	Hack to skip comment
	}
}

void InputSystem::ParseBindings(  )
{
	std::ifstream 	in( "cfg/bindings.cfg" );
	if ( !in.is_open(  ) )
		return;
	std::string 	bind;
	std::string	cmd;
        while ( in >> bind >> cmd )
	{
	        KeyBind binding{  };
		binding.aBind 	= bind;
		binding.aCmd 	= cmd;
		aKeyBinds.push_back( binding );

		std::cout << cmd << " bound to " << bind << std::endl;
	}
}

void InputSystem::Bind( const std::string& srKey, const std::string& srCmd )
{
	for ( const auto& alias : aKeyAliases )
		if ( alias.aAlias == srKey )
		{
			for ( int i = 0; i < aKeyBinds.size(  ); ++i )
				if ( aKeyBinds[ i ].aBind == srKey )
					aKeyBinds.erase( aKeyBinds.begin(  ) + i );
		        KeyBind bind{  };
			bind.aBind 	= srKey;
			bind.aCmd 	= srCmd;
			aKeyBinds.push_back( bind );

			std::cout << srCmd << " bound to " << srKey << std::endl;
			return;
		}
	std::cout << srKey << " is not a valid key alias" << std::endl;
}

void InputSystem::ParseInput(  )
{
	for ( ; SDL_PollEvent( &aEvent ) ; )
	{
		ImGui_ImplSDL2_ProcessEvent( &aEvent );

		switch (aEvent.type)
		{
			case SDL_QUIT:
			{
				apCommandManager->Execute( Engine::Commands::EXIT );
				break;
			}

			case SDL_KEYDOWN:
			{
				if ( aEvent.key.keysym.sym == SDLK_BACKQUOTE )
					apCommandManager->Execute( GuiSystem::Commands::SHOW_CONSOLE );
				else
					for ( const auto& alias : aKeyAliases )
						if ( aEvent.key.keysym.sym == alias.aCode )
							for ( const auto& bind : aKeyBinds )
								if ( bind.aBind == alias.aAlias )
									apConsole->Add( bind.aCmd );

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

		for ( BaseSystem* sys: apSystemManager->GetSystemList() )
		{
			sys->HandleSDLEvent( &aEvent );
		}
	}
}


void InputSystem::Init(  )
{
	BaseSystem::Init(  );

	ResetInputs(  );
}

void InputSystem::InitConsoleCommands(  )
{
	// memory leak from using new, probably not important, will be here for the entire program runtime anyway
	// also slightly odd syntax, idk
	CON_COMMAND_LAMBDA( bind )
	{
		if ( sArgs.size(  ) < 3 )
		{
			apConsole->Print( "Insufficient arguments for bind\n" );
			return;
		}
		Bind( sArgs[ 1 ], sArgs[ 2 ] );
	});

	BaseSystem::InitConsoleCommands(  );
}


void InputSystem::Update( float frameTime )
{
	ResetInputs(  );
	ParseInput(  );
	UpdateKeyStates(  );
}


void InputSystem::ResetInputs(  )
{
	aMouseDelta = {0, 0};

	aKeyboardState = SDL_GetKeyboardState( NULL );
}

const glm::vec2& InputSystem::GetMouseDelta(  )
{
	return aMouseDelta;
}

const glm::vec2& InputSystem::GetMousePos(  )
{
	return aMousePos;
}

bool InputSystem::WindowHasFocus(  )
{
	return aHasFocus;
}

void InputSystem::UpdateKeyStates(  )
{
	for ( auto const& [key, val] : aKeyStates )
		UpdateKeyState( key );
}

void InputSystem::UpdateKeyState( SDL_Scancode key )
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

// old method
#if 0
	switch ( state )
	{
		case KeyState::Released:
			if ( pressed )
				state = KeyState::JustPressed;
			break;

		case KeyState::JustReleased:
			if ( pressed )
				state = KeyState::JustPressed;
			else
				state = KeyState::Released;
			break;

		case KeyState::JustPressed:
			if ( pressed )
				state = KeyState::Pressed;
			else
				state = KeyState::JustReleased;
			break;

		case KeyState::Pressed:
			if ( !pressed )
				state = KeyState::JustReleased;
			break;
	}
#endif
}

void InputSystem::RegisterKey( SDL_Scancode key )
{
	KeyStates::const_iterator state = aKeyStates.find( key );

	// Already registered
	if ( state != aKeyStates.end() )
		return;

	// aKeyStates[key] = aKeyboardState[key] ? KeyState::JustPressed : KeyState::Released;
	aKeyStates[key] = KeyState_Invalid;

	UpdateKeyState( key );
}

KeyState InputSystem::GetKeyState( SDL_Scancode key )
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
			apConsole->Print( "[Input System] Invalid Key: \"%s\"\n", SDL_GetKeyName( SDL_GetKeyFromScancode(key) ) );
			return KeyState_Invalid;
		}
	}

	return state->second;
}


