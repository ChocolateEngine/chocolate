#ifndef ENGINE_H
#define ENGINE_H

#include "system.h"
#include "graphics.h"
#include "input.h"
#include "audio.h"

#include <vector>

class engine_c : public system_c
{
	protected:

	std::vector< system_c* > systems;
	
	void ( *game_init )(  ) = NULL;

	template< typename T >
	void add_system
		( const T* s );
	
	void init_commands
		(  );
	
	public:

	bool active;

	void load_object
		( void ( *func ), const std::string& dlPath, const std::string& entry );

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
