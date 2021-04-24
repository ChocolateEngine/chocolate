#ifndef ENGINE_H
#define ENGINE_H

#include "system.h"
#include "renderer.h"
#include "input.h"
#include "audio.h"

class engine_c : public system_c
{
	protected:

	renderer_c rend;
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
