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


class InputSystem : public BaseInputSystem
{
protected:
	typedef std::vector< KeyAlias > KeyAliases;
	typedef std::vector< KeyBind >	KeyBinds;
	typedef std::unordered_map< SDL_Scancode, KeyState >	KeyStates;
	
	SDL_Event 	aEvent;
	KeyAliases 	aKeyAliases;
	KeyBinds 	aKeyBinds;
	KeyStates 	aKeyStates;
	const Uint8*      aKeyboardState;

	/* Parse the key aliases that can be bound to.  */
	void 		MakeAliases(  );
	/* Parses keybinds located in the config folder.  */
	void 		ParseBindings(  );
	/* Binds the console command to the key.  */
	void 		Bind( const std::string& srKey, const std::string& srCmd );
	/* Initializes all console commands the system can respond to.  */
	void 		InitConsoleCommands(  );
	/*   */
	void 		Init(  );
	/*   */
	void 		Update( float frameTime );
	/* Update all key states on this frame  */
	void 		UpdateKeyStates(  );
	/* Update all key states on this frame  */
	void 		UpdateKeyState( SDL_Scancode key );

	void        ResetInputs(  );

	glm::ivec2  aMouseDelta = {0, 0};
	glm::ivec2  aMousePos = {0, 0};
	bool        aHasFocus = false;

public:
	/* Parses SDL inputs and if there is a valid input, execute the console command.  */
	void 		ParseInput(  );

	const glm::ivec2& GetMouseDelta(  );
	const glm::ivec2& GetMousePos(  );

	/* Is the engine window in focus?.  */
	bool        WindowHasFocus(  );

	/* Add a Key to the key update list.  */
	void        RegisterKey( SDL_Scancode key );

	/* Get the current KeyState value.  */
	KeyState    GetKeyState( SDL_Scancode key );

	/* Convienence functions  */
	bool KeyPressed( SDL_Scancode key ) { return GetKeyState(key) & KeyState_Pressed; }
	bool KeyReleased( SDL_Scancode key ) { return GetKeyState(key) & KeyState_Released; }
	bool KeyJustPressed( SDL_Scancode key ) { return GetKeyState(key) & KeyState_JustPressed; }
	bool KeyJustReleased( SDL_Scancode key ) { return GetKeyState(key) & KeyState_JustReleased; }

	/* Constructor.  */
	explicit        InputSystem(  );
};

