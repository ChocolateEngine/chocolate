#pragma once

#include "../shared/system.h"

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

class InputSystem : public BaseSystem
{
	SYSTEM_OBJECT( InputSystem )
protected:
	typedef std::vector< KeyAlias > KeyAliases;
	typedef std::vector< KeyBind >	KeyBinds;
	
	SDL_Event 	aEvent;
	KeyAliases 	aKeyAliases;
	KeyBinds 	aKeyBinds;

	/* Parse the key aliases that can be bound to.  */
	void 		MakeAliases(  );
	/* Parses keybinds located in the config folder.  */
	void 		ParseBindings(  );
	/* Binds the console command to the key.  */
	void 		Bind( const std::string& srKey, const std::string& srCmd );
public:
	/* Parses SDL inputs and if there is a valid input, execute the console command.  */
	void 		ParseInput(  );
	/* Cosntructor.  */
	explicit        InputSystem(  );
};

