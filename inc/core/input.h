#ifndef INPUT_H
#define INPUT_H

#include "../shared/system.h"

#include <SDL2/SDL.h>

typedef struct key_alias_s
{
	std::string alias;
	int code;
}key_alias_t;

typedef struct keybind_s
{
	std::string bind, cmd;
}keybind_t;

class input_c : public system_c
{
	protected:

	SDL_Event event;
	std::vector< key_alias_t > keyAliases;
	std::vector< keybind_t > keyBinds;

	void make_aliases
		(  );
	void parse_bindings
		(  );
	void bind
		( const std::string& key, const std::string& cmd );
	
	public:

	void parse_input
		(  );

	void init_console_commands
		(  );
	
	input_c
		(  );
};

#endif
