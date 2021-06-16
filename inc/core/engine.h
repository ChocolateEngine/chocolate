#ifndef ENGINE_H
#define ENGINE_H

#include "../shared/system.h"
#include "graphics.h"
#include "input.h"
#include "audio.h"

#include <vector>

class engine_c : public system_c
{
	protected:

	std::vector< system_c* > systems;
	
	std::vector< system_c* > ( *game_init )(  ) = NULL;

	template< typename T >
	void add_system
		( const T* s );
	void add_game_systems
		(  );
	
	void init_commands
		(  );
	
	public:

	bool active;

	void load_object
		( const std::string& dlPath, const std::string& entry );

	void engine_main
		(  );

	void init_systems
		(  );
	void update_systems
		(  );

	engine_c
		(  );
	~engine_c
		(  );
};

#endif
