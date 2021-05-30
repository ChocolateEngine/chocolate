#include "../../inc/core/engine.h"

#include <dlfcn.h>

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

void engine_c::load_object
	( const std::string& dlPath, const std::string& entry )
{
	void* handle;

	handle = dlopen( dlPath.c_str(  ), RTLD_LAZY );
	if ( !handle )
	{
		throw std::runtime_error( "Unable to load shared librarys!" );
	}

	*( void** )( &game_update ) = dlsym( handle, entry.c_str(  ) );
	if ( !game_update )
	{
		dlclose( handle );
		throw std::runtime_error( "Unable to link library's entry point!" );
	}

	dlclose( handle );
}

void engine_c::engine_main
	(  )
{
	static msgs_c msgs;
	static console_c console;

	this->msgs = &msgs;
	this->console = &console;

	init_systems(  );

	this->msgs->add( ENGINE_C, ENGI_PING );
	
	for ( ; active; )
	{
		update_systems(  );
	}
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
