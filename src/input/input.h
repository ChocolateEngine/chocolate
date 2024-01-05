#pragma once

#include "system.h"
#include "iinput.h"

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


class InputSystem : public IInputSystem
{
protected:
	typedef std::vector< KeyAlias > KeyAliases;
	typedef std::vector< KeyBind >	KeyBinds;
	
	KeyAliases 	aKeyAliases;
	KeyBinds 	aKeyBinds;

	std::unordered_map< EButton, KeyState > aKeyStates;

	const Uint8*                            aKeyboardState;
	std::vector< SDL_Event >                aEvents;

	/* Parse the key aliases that can be bound to.  */
	void 		MakeAliases();
	/* Parses keybinds located in the config folder.  */
	void 		ParseBindings();
	/* Binds the console command to the key.  */
	void 		Bind( const std::string& srKey, const std::string& srCmd );
	/* Initializes all console commands the system can respond to.  */
	void 		InitConsoleCommands();
	/*   */
	bool 		Init();
	/*   */
	void 		Update( float frameTime );
	/* Update all key states on this frame  */
	void 		UpdateKeyStates();
	/* Update all key states on this frame  */
	void 		UpdateKeyState( EButton key );

	void        ResetInputs();

	void        PressMouseButton( EButton sButton );
	void        ReleaseMouseButton( EButton sButton );

	glm::ivec2  aMouseDelta = {0, 0};
	glm::ivec2  aMousePos = {0, 0};
	bool        aHasFocus = true;

	glm::ivec2  aScroll;

public:
	/* Parses SDL inputs and if there is a valid input, execute the console command.  */
	void 		ParseInput(  );

	const glm::ivec2&         GetMouseDelta() override;
	const glm::ivec2&         GetMousePos() override;
	virtual glm::ivec2        GetMouseScroll() override;

	/* Is the engine window in focus?.  */
	bool        WindowHasFocus();

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

	/* Accessors.  */
	std::vector< SDL_Event > *GetEvents() { return &aEvents; }

	/* Constructor.  */
	explicit        InputSystem();
};

