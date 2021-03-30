#include "../../inc/core/engine.h"

void engine_c::init_commands
	(  )
{
	msg_s msg;
	msg.type = ENGINE_C;
	
	msg.msg = ENGI_PING;
	msg.func = [ & ](  ){ printf( "Engine: Ping!\n" ); };
	engineCommands.push_back( msg );
}

void engine_c::init_systems
	(  )
{
	rend.msgs = msgs;
	rend.console = console;
}

void engine_c::update_systems
	(  )
{
	update(  );
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
