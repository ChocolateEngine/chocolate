#ifndef ENGINE_H
#define ENGINE_H

#include "system.h"
#include "renderer.h"

class engine_c : public system_c
{
	protected:

	renderer_c rend;
	
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
