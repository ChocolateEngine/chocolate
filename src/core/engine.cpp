#include "../../inc/core/engine.h"

void engine_c::init_commands
	(  )
{
	msg_s msg;
	msg.type = ENGINE_C;
	
	msg.msg = ENGI_PING;
	msg.func = [ & ]( void** args, int argsLen ){ printf( "Engine: Ping!\n" ); };
	engineCommands.push_back( msg );

	msg.msg = ENGI_EXIT;
	msg.func = [ & ]( void** args, int argsLen ){ active = false; };
	engineCommands.push_back( msg );
}

void engine_c::init_systems
	(  )
{
	graphics.msgs = msgs;
	graphics.console = console;

	input.msgs = msgs;
	input.console = console;

	audio.msgs = msgs;
	audio.console = console;
}

void engine_c::update_systems
	(  )
{
	update(  );

	graphics.update(  );
	graphics.draw_frame(  );

	input.update(  );
	input.parse_input(  );

	audio.update(  );
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
