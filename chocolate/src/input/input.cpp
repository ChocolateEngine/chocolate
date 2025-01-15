/*
input.cpp ( Authored by p0lyh3dron )

Defines the methods declared in
input.h
*/
#include "core/core.h"
#include "input.h"
#include "imgui/imgui.h"
#include "imgui/imgui_impl_sdl2.h"

#include <fstream>
#include <iostream>
#include <unordered_set>

#if CH_USE_MIMALLOC
  #include "mimalloc-new-delete.h"
#endif

InputSystem* input = new InputSystem;

LOG_CHANNEL_REGISTER( InputSystem, ELogColor_Default );


static ModuleInterface_t gInterfaces[] = {
	{ input, IINPUTSYSTEM_NAME, IINPUTSYSTEM_VER }
};

extern "C"
{
	DLL_EXPORT ModuleInterface_t* ch_get_interfaces( u8& srCount )
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


InputSysWindow* InputSystem::UpdateCurrentWindow( SDL_Event& sEvent )
{
	PROF_SCOPE();

	SDL_Window* window = nullptr;

	switch ( sEvent.type )
	{
		case SDL_WINDOWEVENT:
		{
			window = SDL_GetWindowFromID( sEvent.window.windowID );
			break;
		}

		case SDL_KEYUP:
		case SDL_KEYDOWN:
		{
			window = SDL_GetWindowFromID( sEvent.key.windowID );
			break;
		}
		
		case SDL_TEXTEDITING:
		{
			window = SDL_GetWindowFromID( sEvent.edit.windowID );
			break;
		}
		
		case SDL_TEXTEDITING_EXT:
		{
			window = SDL_GetWindowFromID( sEvent.editExt.windowID );
			break;
		}
		
		case SDL_TEXTINPUT:
		{
			window = SDL_GetWindowFromID( sEvent.text.windowID );
			break;
		}

		case SDL_MOUSEMOTION:
		{
			window = SDL_GetWindowFromID( sEvent.motion.windowID );
			break;
		}

		case SDL_MOUSEBUTTONUP:
		case SDL_MOUSEBUTTONDOWN:
		{
			window = SDL_GetWindowFromID( sEvent.button.windowID );
			break;
		}

		case SDL_MOUSEWHEEL:
		{
			window = SDL_GetWindowFromID( sEvent.wheel.windowID );
			break;
		}

		case SDL_DROPBEGIN:
		case SDL_DROPFILE:
		case SDL_DROPTEXT:
		case SDL_DROPCOMPLETE:
		{
			window = SDL_GetWindowFromID( sEvent.drop.windowID );
			break;
		}

		case SDL_USEREVENT:
		{
			window = SDL_GetWindowFromID( sEvent.user.windowID );
			break;
		}
	}

	if ( window == nullptr )
		return nullptr;

	auto it = aWindows.find( window );

	if ( it == aWindows.end() )
	{
		Log_ErrorF( "EVENT DOES NOT HAVE WINDOW CONTEXT\n" );
		return nullptr;
	}
	else
	{
		ImGui::SetCurrentContext( it->second.context );
	}

	return &it->second;
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

	ImGuiContext* origContext = ImGui::GetCurrentContext();

	std::unordered_set< SDL_Window* > windowsToDelete;

	for ( int i = 0; i < eventCount; i++ )
	{
		ImGui::SetCurrentContext( origContext );
		SDL_Event& event = aEvents[ i ];

		InputSysWindow* window = UpdateCurrentWindow( event );

		{
			PROF_SCOPE_NAMED( "ImGui_ImplSDL2_ProcessEvent" );
			ImGui_ImplSDL2_ProcessEvent( &event );
		}

		switch (event.type)
		{
			case SDL_MOUSEMOTION:
			{
				if ( !window )
					continue;

				window->mousePos.x = event.motion.x;
				window->mousePos.y = event.motion.y;
				window->mouseDelta.x += event.motion.xrel;
				window->mouseDelta.y += event.motion.yrel;
				break;
			}

			case SDL_WINDOWEVENT:
			{
				if ( !window )
					continue;

				switch ( event.window.event )
				{
					case SDL_WINDOWEVENT_CLOSE:
					{
						SDL_Window* sdlWindow = SDL_GetWindowFromID( event.window.windowID );
						windowsToDelete.emplace( sdlWindow );
						break;
					}

					case SDL_WINDOWEVENT_FOCUS_GAINED:
					{
						aFocusedWindow = window;
						break;
					}

					case SDL_WINDOWEVENT_FOCUS_LOST:
					{
						if ( aFocusedWindow == window )
							aFocusedWindow = nullptr;

						break;
					}
				}
				break;
			}

			case SDL_MOUSEBUTTONDOWN:
			{
				if ( !window )
					continue;

				switch ( event.button.button )
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
				if ( !window )
					continue;

				switch ( event.button.button )
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
				if ( !window )
					continue;

				window->mouseScroll.x += event.wheel.x;
				window->mouseScroll.y += event.wheel.y;

				if ( event.wheel.x > 0 )
					PressMouseButton( EButton_MouseWheelRight );
				else if ( event.wheel.x < 0 )
					PressMouseButton( EButton_MouseWheelLeft );

				if ( event.wheel.y > 0 )
					PressMouseButton( EButton_MouseWheelUp );
				else if ( event.wheel.y < 0 )
					PressMouseButton( EButton_MouseWheelDown );

				break;
			}
		}
	}

	ImGui::SetCurrentContext( origContext );

	for ( SDL_Window* sdlWindow : windowsToDelete )
	{
		RemoveWindow( sdlWindow );
	}
}


bool InputSystem::Init()
{
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

	for ( auto& [ sdlWindow, inputWindow ] : aWindows )
	{
		inputWindow.mouseDelta  = { 0, 0 };
		inputWindow.mouseScroll = { 0, 0 };
	}

	aKeyboardState = SDL_GetKeyboardState( NULL );

	ReleaseMouseButton( EButton_MouseWheelUp );
	ReleaseMouseButton( EButton_MouseWheelDown );
	ReleaseMouseButton( EButton_MouseWheelLeft );
	ReleaseMouseButton( EButton_MouseWheelRight );
}

const glm::ivec2& InputSystem::GetMouseDelta()
{
	if ( !aActiveWindow )
		return {};

	return aActiveWindow->mouseDelta;
}

const glm::ivec2& InputSystem::GetMousePos()
{
	if ( !aActiveWindow )
		return {};

	return aActiveWindow->mousePos;
}

glm::ivec2 InputSystem::GetMouseScroll()
{
	if ( !aActiveWindow )
		return {};

	return aActiveWindow->mouseScroll;
}

bool InputSystem::WindowHasFocus()
{
	if ( !aActiveWindow || !aFocusedWindow )
		return false;

	return aFocusedWindow = aActiveWindow;
}

bool InputSystem::WindowHasFocus( SDL_Window* window )
{
	auto it = aWindows.find( window );

	if ( it == aWindows.end() )
	{
		Log_ErrorF( "Could not find SDL Window\n" );
		return false;
	}

	return aFocusedWindow == &it->second;
}

void InputSystem::SetWindowFocused( SDL_Window* window )
{
	auto it = aWindows.find( window );

	if ( it == aWindows.end() )
	{
		Log_ErrorF( "Could not find SDL Window\n" );
		return;
	}

	aFocusedWindow = &it->second;

	SDL_RaiseWindow( it->second.window );
}

void InputSystem::UpdateKeyStates()
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
		Log_ErrorF( gLC_InputSystem, "Invalid Button ID: %d\n", key );
		return;
	}

	KeyState& state = aKeyStates[ key ];

	// Mouse Buttons
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

	bool pressed = aKeyboardState[ key ];

	// TODO: change this to a signed char enum instead of flags
	// -1 = JustReleased, 0 = Released, 1 = JustPressed, 2 = Pressed
	// That way we can easily check if it's true for pressed and false for released

	// Keyboard Buttons
	if ( state & KeyState_Pressed )
	{
		// If the key state is just pressed, remove the just pressed flag
		if ( state & KeyState_JustPressed )
			state &= ~KeyState_JustPressed;

		// If the key is no longer pressed, set it to released
		if ( !pressed )
		{
			// Set the key to released and just released
			// and remove the pressed and just pressed flags
			state |= KeyState_Released | KeyState_JustReleased;
			state &= ~(KeyState_Pressed | KeyState_JustPressed);
		}
	}
	// If the key is released
	else if ( state & KeyState_Released )
	{
		// If the key state is just released, remove the just released flag
		if ( state & KeyState_JustReleased )
			state &= ~KeyState_JustReleased;

		// If the key is now pressed, set it to pressed
		if ( pressed )
		{
			// Set the key to pressed and just pressed
			// and remove the released and just released flags
			state |= KeyState_Pressed | KeyState_JustPressed;
			state &= ~(KeyState_Released | KeyState_JustReleased);
		}
	}
	else
	{
		// No state set, set the key to released and just released
		state |= KeyState_Released | KeyState_JustReleased;
		state &= ~(KeyState_Pressed | KeyState_JustPressed);
	}
}

bool InputSystem::RegisterKey( EButton key )
{
	if ( key < 0 )
	{
		Log_ErrorF( gLC_InputSystem, "Invalid Button ID: %d\n", key );
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
		if ( sName == gButtonStr[ i ] )
			return (EButton)( (u32)EButton_BeforeMouse + i + 1 );
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
			Log_WarnF( gLC_InputSystem, "Invalid Key: \"%s\"\n", GetKeyName( key ) );
			return KeyState_Invalid;
		}
	}

	return state->second;
}


void InputSystem::AddWindow( SDL_Window* sWindow, void* sImGuiContext )
{
	aWindows[ sWindow ].context = (ImGuiContext*)sImGuiContext;

	// if there are no windows created yet, assume this one is
	if ( aWindows.size() == 1 )
	{
		aActiveWindow  = &aWindows[ sWindow ];
		aFocusedWindow = &aWindows[ sWindow ];
	}
	else
	{
		// clear this since the memory can change
		aActiveWindow  = nullptr;
		aFocusedWindow = nullptr;
	}
}


void InputSystem::RemoveWindow( SDL_Window* sWindow )
{
	// clear this since the memory can change
	aActiveWindow  = nullptr;
	aFocusedWindow = nullptr;

	aWindows.erase( sWindow );
}


bool InputSystem::SetCurrentWindow( SDL_Window* sWindow )
{
	auto it = aWindows.find( sWindow );

	if ( it == aWindows.end() )
	{
		Log_ErrorF( "Could not find SDL Window\n" );
		return false;
	}
	else
	{
		aActiveWindow = &it->second;
		ImGui::SetCurrentContext( it->second.context );
		return true;
	}
}


SDL_Window* InputSystem::GetCurrentWindow()
{
	if ( !aActiveWindow )
		return nullptr;

	return aActiveWindow->window;
}

