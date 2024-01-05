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

InputSystem* input = new InputSystem;
IGuiSystem*  gui   = nullptr;

LOG_REGISTER_CHANNEL( InputSystem, LogColor::Default );

CONVAR( in_show_mouse_events, 0 );


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

InputSystem::InputSystem(  ) : IInputSystem(  )
{
	MakeAliases(  );
	ParseBindings(  );

	// Register Mouse Buttons
	RegisterKey( EButton_MouseLeft );
	RegisterKey( EButton_MouseRight );
	RegisterKey( EButton_MouseMiddle );
	RegisterKey( EButton_MouseX1 );
	RegisterKey( EButton_MouseX2 );

	RegisterKey( EButton_MouseWheelUp );
	RegisterKey( EButton_MouseWheelDown );
	RegisterKey( EButton_MouseWheelLeft );
	RegisterKey( EButton_MouseWheelRight );
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


void InputSystem::PressMouseButton( EButton sButton )
{
	KeyState& state = aKeyStates[ sButton ];
	state |= KeyState_Pressed | KeyState_JustPressed;
	state &= ~(KeyState_Released | KeyState_JustReleased);
}


void InputSystem::ReleaseMouseButton( EButton sButton )
{
	KeyState& state = aKeyStates[ sButton ];
	state |= KeyState_Released | KeyState_JustReleased;
	state &= ~(KeyState_Pressed | KeyState_JustPressed);
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

			case SDL_MOUSEBUTTONDOWN:
			{
				switch ( aEvent.button.button )
				{
					case SDL_BUTTON_LEFT:
						PressMouseButton( EButton_MouseLeft );
						break;

					case SDL_BUTTON_RIGHT:
						PressMouseButton( EButton_MouseRight );
						break;

					case SDL_BUTTON_MIDDLE:
						PressMouseButton( EButton_MouseMiddle );
						break;

					case SDL_BUTTON_X1:
						PressMouseButton( EButton_MouseX1 );
						break;

					case SDL_BUTTON_X2:
						PressMouseButton( EButton_MouseX2 );
						break;
				}
				break;
			}

			case SDL_MOUSEBUTTONUP:
			{
				switch ( aEvent.button.button )
				{
					case SDL_BUTTON_LEFT:
						ReleaseMouseButton( EButton_MouseLeft );
						break;

					case SDL_BUTTON_RIGHT:
						ReleaseMouseButton( EButton_MouseRight );
						break;

					case SDL_BUTTON_MIDDLE:
						ReleaseMouseButton( EButton_MouseMiddle );
						break;

					case SDL_BUTTON_X1:
						ReleaseMouseButton( EButton_MouseX1 );
						break;

					case SDL_BUTTON_X2:
						ReleaseMouseButton( EButton_MouseX2 );
						break;
				}
				break;
			}

			case SDL_MOUSEWHEEL:
			{
				aScroll.x += aEvent.wheel.x;
				aScroll.y += aEvent.wheel.y;

				if ( aEvent.wheel.x > 0 )
					PressMouseButton( EButton_MouseWheelRight );
				else if ( aEvent.wheel.x < 0 )
					PressMouseButton( EButton_MouseWheelLeft );

				if ( aEvent.wheel.y > 0 )
					PressMouseButton( EButton_MouseWheelUp );
				else if ( aEvent.wheel.y < 0 )
					PressMouseButton( EButton_MouseWheelDown );

				break;
			}
		}
	}
}


bool InputSystem::Init()
{
	gui = ( IGuiSystem* )Mod_GetInterface( IGUI_NAME, IGUI_HASH );
	
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

	ResetInputs();
	ParseInput();
	UpdateKeyStates();
}


void InputSystem::ResetInputs()
{
	PROF_SCOPE();

	aMouseDelta = {0, 0};

	aKeyboardState = SDL_GetKeyboardState( NULL );

	aScroll = {0, 0};
		
	// ReleaseMouseButton( EButton_MouseLeft );
	// ReleaseMouseButton( EButton_MouseRight );
	// ReleaseMouseButton( EButton_MouseMiddle );
	// ReleaseMouseButton( EButton_MouseX1 );
	// ReleaseMouseButton( EButton_MouseX2 );
	
	ReleaseMouseButton( EButton_MouseWheelUp );
	ReleaseMouseButton( EButton_MouseWheelDown );
	ReleaseMouseButton( EButton_MouseWheelLeft );
	ReleaseMouseButton( EButton_MouseWheelRight );
}

const glm::ivec2& InputSystem::GetMouseDelta()
{
	return aMouseDelta;
}

const glm::ivec2& InputSystem::GetMousePos()
{
	return aMousePos;
}

glm::ivec2 InputSystem::GetMouseScroll()
{
	return aScroll;
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

void InputSystem::UpdateKeyState( EButton key )
{
	PROF_SCOPE();

	if ( key < 0 )
	{
		Log_ErrorF( gInputSystemChannel, "Invalid Button ID: %d\n", key );
		return;
	}

	bool pressed = false;
	KeyState& state   = aKeyStates[ key ];

	if ( key > EButton_BeforeMouse )
	{
		if ( state & KeyState_Pressed )
		{
			if ( state & KeyState_JustPressed )
				state &= ~KeyState_JustPressed;
		}
		else if ( state & KeyState_Released )
		{
			if ( state & KeyState_JustReleased )
				state &= ~KeyState_JustReleased;
		}
		else
		{
			state |= KeyState_Released | KeyState_JustReleased;
			state &= ~(KeyState_Pressed | KeyState_JustPressed);
		}

		return;
	}
	else
	{
		pressed = aKeyboardState[ key ];
	}

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

bool InputSystem::RegisterKey( EButton key )
{
	if ( key < 0 )
	{
		Log_ErrorF( gInputSystemChannel, "Invalid Button ID: %d\n", key );
		return false;
	}

	auto state = aKeyStates.find( key );

	// Already registered
	if ( state != aKeyStates.end() )
		return true;

	// aKeyStates[key] = aKeyboardState[key] ? KeyState::JustPressed : KeyState::Released;
	aKeyStates[ key ] = KeyState_Invalid;

	UpdateKeyState( key );
	return true;
}


static std::string_view gButtonStr[] = {

	"MOUSE_L",
	"MOUSE_R",
	"MOUSE_M",
	"MOUSE_X1",
	"MOUSE_X2",

	"MOUSE_WHEEL_UP",
	"MOUSE_WHEEL_DOWN",
	"MOUSE_WHEEL_LEFT",
	"MOUSE_WHEEL_RIGHT",
};


static_assert( CH_ARR_SIZE( gButtonStr ) == EButton_Count - ( EButton_BeforeMouse + 1 ) );


EButton InputSystem::GetKeyFromName( std::string_view sName )
{
	for ( u32 i = 0; i < CH_ARR_SIZE( gButtonStr ); i++ )
	{
		if (sName == gButtonStr[i])
			return (EButton)((u32)EButton_BeforeMouse + i);
	}

	return (EButton)SDL_GetScancodeFromName( sName.data() );
}


const char* InputSystem::GetKeyName( EButton key )
{
	if ( key > EButton_BeforeMouse )
	{
		if ( key < EButton_Count )
			return gButtonStr[ key - EButton_BeforeMouse - 1 ].data();
		else
			return SDL_GetScancodeName( SDL_SCANCODE_UNKNOWN );
			// return "INVALID";
		
		// switch (key)
		// {
		// 	case EButton_MouseLeft:
		// 		return "MOUSE_L";
		// 	case EButton_MouseRight:
		// 		return "MOUSE_R";
		// 	case EButton_MouseMiddle:
		// 		return "MOUSE_M";
		// 	case EButton_MouseX1:
		// 		return "MOUSE_X1";
		// 	case EButton_MouseX2:
		// 		return "MOUSE_X2";
		// 
		// 	case EButton_MouseWheelUp:
		// 		return "MOUSE_WHEEL_UP";
		// 	case EButton_MouseWheelDown:
		// 		return "MOUSE_WHEEL_DOWN";
		// 	case EButton_MouseWheelLeft:
		// 		return "MOUSE_WHEEL_LEFT";
		// 	case EButton_MouseWheelRight:
		// 		return "MOUSE_WHEEL_RIGHT";
		// }
	}
	else
	{
		return SDL_GetScancodeName( (SDL_Scancode)key );
	}
}


const char* InputSystem::GetKeyDisplayName( EButton key )
{
	if ( key > EButton_BeforeMouse )
	{
		switch (key)
		{
			case EButton_MouseLeft:
				return "Left Mouse";
			case EButton_MouseRight:
				return "Right Mouse";
			case EButton_MouseMiddle:
				return "Middle Mouse";
			case EButton_MouseX1:
				return "Mouse X1";
			case EButton_MouseX2:
				return "Mouse X2";

			case EButton_MouseWheelUp:
				return "Mouse Wheel Up";
			case EButton_MouseWheelDown:
				return "Mouse Wheel Down";
			case EButton_MouseWheelLeft:
				return "Mouse Wheel Left";
			case EButton_MouseWheelRight:
				return "Mouse Wheel Right";
		}
	}
	else
	{
		return SDL_GetKeyName( SDL_GetKeyFromScancode( (SDL_Scancode)key ) );
	}
}


KeyState InputSystem::GetKeyState( EButton key )
{
	auto state = aKeyStates.find( key );

	if ( state == aKeyStates.end() )
	{
		// Try to register this key
		if ( !RegisterKey( key ) )
			return KeyState_Invalid;

		state = aKeyStates.find( key );

		if ( state == aKeyStates.end() )
		{
			// would be odd if this got hit
			Log_WarnF( gInputSystemChannel, "Invalid Key: \"%s\"\n", GetKeyName( key ) );
			return KeyState_Invalid;
		}
	}

	return state->second;
}


