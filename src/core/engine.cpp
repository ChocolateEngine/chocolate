#include "../../inc/core/engine.h"

void engine_c::init_commands
	(  )
{
	msg_s msg;
	msg.type = ENGINE_C;
	
	msg.msg = ENGI_PING;
	msg.func = [ & ](  ){ printf( "Engine: Ping!\n" ); };
	engineCommands.push_back( msg );

	msg.msg = ENGI_EXIT;
	msg.func = [ & ](  ){ active = false; };
	engineCommands.push_back( msg );
}

void engine_c::init_systems
	(  )
{
	rend.msgs = msgs;
	rend.console = console;

	input.msgs = msgs;
	input.console = console;
}

void engine_c::update_systems
	(  )
{
	update(  );

	rend.update(  );
	rend.draw_frame(  );

	input.update(  );
	input.parse_input(  );
}

engine_c::engine_c
	(  )
{
	active = true;
	systemType = ENGINE_C;

	init_systems(  );
	init_commands(  );
}

engine_c::~engine_c
	(  )
{

}
