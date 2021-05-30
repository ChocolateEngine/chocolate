#ifndef ENGINE_H
#define ENGINE_H

#include "system.h"
#include "graphics.h"
#include "input.h"
#include "audio.h"
#include "../shared/entity.h"

#include <vector>

class engine_c : public system_c
{
	protected:

	graphics_c graphics;
	input_c input;
	audio_c audio;

	std::vector< entity_c* > entities;
	
	void ( *game_init )(  ) = NULL;
	void ( *game_update )(  ) = NULL;
	
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
