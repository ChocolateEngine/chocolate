#ifndef ENGINE_H
#define ENGINE_H

#include "system.h"

class engine_c : public system_c
{
	protected:

	public:

	bool active;

	void init_systems
		(  );

	engine_c
		(  );
	~engine_c
		(  );
};

#endif
