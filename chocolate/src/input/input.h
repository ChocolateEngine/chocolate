#pragma once

#include "iinput.h"

#include "imgui/imgui.h"
#include "imgui/imgui_internal.h"

#include <SDL2/SDL.h>

class KeyAlias
{
public:
	std::string 	aAlias;
	int 		aCode;
};

class KeyBind
{
public:
	std::string 	aBind;
	std::string	aCmd;
};


struct InputSysWindow
{
	SDL_Window*   window;
	ImGuiContext* context;
	glm::ivec2    mouseDelta{};
	glm::ivec2    mousePos{};
	glm::ivec2    mouseScroll{};
};


class InputSystem : public IInputSystem
{
protected:
	typedef std::vector< KeyAlias >                   KeyAliases;
	typedef std::vector< KeyBind >                    KeyBinds;

	KeyAliases                                        aKeyAliases;
	KeyBinds                                          aKeyBinds;

	std::unordered_map< EButton, KeyState >           aKeyStates;

	const Uint8*                                      aKeyboardState;
	std::vector< SDL_Event >                          aEvents;

	std::unordered_map< SDL_Window*, InputSysWindow > aWindows;

	// TODO: these really should not be a pointer to a value in the hash map above
	InputSysWindow*                                   aActiveWindow  = nullptr;
	InputSysWindow*                                   aFocusedWindow = nullptr;

	/* Parse the key aliases that can be bound to.  */
	void 		MakeAliases();
	/* Parses keybinds located in the config folder.  */
	void 		ParseBindings();
	/* Binds the console command to the key.  */
	void 		Bind( const std::string& srKey, const std::string& srCmd );

	bool 		Init();

	void 		Update( float frameTime );
	/* Update all key states on this frame  */
	void 		UpdateKeyStates();
	/* Update all key states on this frame  */
	void 		UpdateKeyState( EButton key );

	void        ResetInputs();

	void        PressMouseButton( EButton sButton );
	void        ReleaseMouseButton( EButton sButton );

public:
	/* Parses SDL inputs and if there is a valid input, execute the console command.  */
	void 		ParseInput(  );

	InputSysWindow*           UpdateCurrentWindow( SDL_Event& sEvent );


	const glm::ivec2&         GetMouseDelta() override;
	const glm::ivec2&         GetMousePos() override;
	virtual glm::ivec2        GetMouseScroll() override;

	/* Is the engine window in focus?.  */
	bool                      WindowHasFocus() override;
	bool                      WindowHasFocus( SDL_Window* window ) override;
	void                      SetWindowFocused( SDL_Window* window ) override;

	/* Add a Key to the key update list.  */
	bool        RegisterKey( EButton key ) override;

	/* Get the display name for this key  */
	const char* GetKeyName( EButton key ) override;
	const char* GetKeyDisplayName( EButton key ) override;
	EButton     GetKeyFromName( std::string_view sName ) override;

	/* Get the current KeyState value.  */
	KeyState    GetKeyState( EButton key ) override;

	/* Convienence functions  */
	bool KeyPressed( EButton key ) { return GetKeyState(key) & KeyState_Pressed; }
	bool KeyReleased( EButton key ) { return GetKeyState(key) & KeyState_Released; }
	bool KeyJustPressed( EButton key ) { return GetKeyState(key) & KeyState_JustPressed; }
	bool KeyJustReleased( EButton key ) { return GetKeyState(key) & KeyState_JustReleased; }

	virtual void              AddWindow( SDL_Window* sWindow, void* sImGuiContext ) override;
	virtual void              RemoveWindow( SDL_Window* sWindow ) override;
	virtual bool              SetCurrentWindow( SDL_Window* window ) override;
	virtual SDL_Window*       GetCurrentWindow() override;

	/* Accessors.  */
	std::vector< SDL_Event > *GetEvents() { return &aEvents; }

	/* Constructor.  */
	explicit        InputSystem();
};

