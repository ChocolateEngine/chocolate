#ifndef ENGINE_H
#define ENGINE_H

#include "system.h"
#include "graphics.h"
#include "input.h"
#include "audio.h"

class engine_c : public system_c
{
	protected:

	graphics_c graphics;
	input_c input;
	audio_c audio;
	
	void init_commands
		(  );
	
	public:

	bool active;

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
