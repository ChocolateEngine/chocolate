#include "../../inc/core/engine.h"

int main
	(  )
{
	engine_c engine;
	static msgs_c msgs;
	static console_c console;

	engine.msgs = &msgs;
	engine.console = &console;

	engine.init_systems(  );

	msgs.add( { ENGINE_C, ENGI_PING, NULL } );
	
	for ( ; engine.active; )
	{
		engine.update_systems(  );
	}
	return 0;
}
