/*
input.cpp ( Authored by p0lyh3dron )

Defines the methods declared in
input.h
*/
#include "core/core.h"
#include "input.h"
#include "imgui/imgui.h"
#include "imgui/imgui_impl_sdl.h"

#include "igui.h"

#include <fstream>
#include <iostream>

#if CH_USE_MIMALLOC
  #include "mimalloc-new-delete.h"
#endif

InputSystem *input = new InputSystem;

LOG_REGISTER_CHANNEL( InputSystem, LogColor::Default );

CONVAR( in_show_mouse_events, 0 );

BaseGuiSystem*           gui           = nullptr;

static ModuleInterface_t gInterfaces[] = {
	{ input, IINPUTSYSTEM_NAME, IINPUTSYSTEM_HASH }
};

extern "C"
{
	DLL_EXPORT ModuleInterface_t* cframework_GetInterfaces( size_t& srCount )
	{
		srCount = 1;
		return gInterfaces;
	}
}

InputSystem::InputSystem(  ) : BaseInputSystem(  )
{
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

void InputSystem::ParseInput()
{
	PROF_SCOPE();

	{
		PROF_SCOPE_NAMED( "SDL_PumpEvents" );
		SDL_PumpEvents();
	}

	int mouseEventCount = 0;
	int eventCount      = 0;

	{
		PROF_SCOPE_NAMED( "SDL_PeepEvents" );
		// get total event count first
		eventCount = SDL_PeepEvents( nullptr, 0, SDL_PEEKEVENT, SDL_FIRSTEVENT, SDL_LASTEVENT );

		// resize event vector with events found
		aEvents.resize( eventCount );

		// fill event vector with events found
		SDL_PeepEvents( aEvents.data(), eventCount, SDL_GETEVENT, SDL_FIRSTEVENT, SDL_LASTEVENT );
	}

	for ( int i = 0; i < eventCount; i++ )
	{
		SDL_Event& aEvent = aEvents[i];

		{
			PROF_SCOPE_NAMED( "ImGui_ImplSDL2_ProcessEvent" );
			ImGui_ImplSDL2_ProcessEvent( &aEvent );
		}

		switch (aEvent.type)
		{
			case SDL_QUIT:
			{
				// lazy way to tell engine to quit
				Con_RunCommand( "quit" );
				break;
			}

			case SDL_MOUSEMOTION:
			{
				aMousePos.x = aEvent.motion.x;
				aMousePos.y = aEvent.motion.y;
				aMouseDelta.x += aEvent.motion.xrel;
				aMouseDelta.y += aEvent.motion.yrel;

				if ( in_show_mouse_events )
					gui->DebugMessage( 12 + mouseEventCount++, "Mouse Motion X: %d Y:%d", aEvent.motion.xrel, aEvent.motion.yrel );

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
	}
}


bool InputSystem::Init()
{
	gui = ( BaseGuiSystem* )Mod_GetInterface( IGUI_NAME, IGUI_HASH );
	
	if ( gui == nullptr )
	{
		Log_Error( gInputSystemChannel, "Failed to load GUI Interface\n" );
		return false;
	}

	ResetInputs();
	return true;
}

void InputSystem::Update( float frameTime )
{
	PROF_SCOPE_NAMED( "Input System Update" );

	ResetInputs(  );
	ParseInput(  );
	UpdateKeyStates(  );
}


void InputSystem::ResetInputs(  )
{
	PROF_SCOPE();

	aMouseDelta = {0, 0};

	aKeyboardState = SDL_GetKeyboardState( NULL );
}

const glm::ivec2& InputSystem::GetMouseDelta(  )
{
	return aMouseDelta;
}

const glm::ivec2& InputSystem::GetMousePos(  )
{
	return aMousePos;
}

bool InputSystem::WindowHasFocus(  )
{
	return aHasFocus;
}

void InputSystem::UpdateKeyStates(  )
{
	PROF_SCOPE();

	for ( auto const& [key, val] : aKeyStates )
		UpdateKeyState( key );
}

void InputSystem::UpdateKeyState( SDL_Scancode key )
{
	PROF_SCOPE();

	bool      pressed = aKeyboardState[ key ];
	KeyState& state   = aKeyStates[ key ];

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

void InputSystem::RegisterKey( SDL_Scancode key )
{
	auto state = aKeyStates.find( key );

	// Already registered
	if ( state != aKeyStates.end() )
		return;

	// aKeyStates[key] = aKeyboardState[key] ? KeyState::JustPressed : KeyState::Released;
	aKeyStates[ key ] = KeyState_Invalid;

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
			Log_WarnF( gInputSystemChannel, "Invalid Key: \"%s\"\n", SDL_GetKeyName( SDL_GetKeyFromScancode(key) ) );
			return KeyState_Invalid;
		}
	}

	return state->second;
}


