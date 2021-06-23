#include "../../inc/core/engine.h"

#include <dlfcn.h>

template< typename T >
void engine_c::add_system
	( const T* s )
{
	systems.push_back( ( system_c* )s );
}

void engine_c::add_game_systems
	(  )
{
	std::vector< system_c* > gameSystems = game_init(  );
	for ( const auto& sys : gameSystems )
	{
		sys->add_flag( EXTERNAL_SYSTEM );
		sys->msgs = msgs;
		sys->console = console;
		systems.push_back( sys );
	}
}

void engine_c::init_commands
	(  )
{
	msg_s msg;
	msg.type = ENGINE_C;
	
	msg.msg = ENGI_PING;
	msg.func = [ & ]( std::vector< std::any > args ){ printf( "Engine: Ping!\n" ); };
	engineCommands.push_back( msg );

	msg.msg = ENGI_EXIT;
	msg.func = [ & ]( std::vector< std::any > args ){ active = false; };
	engineCommands.push_back( msg );
}

void engine_c::load_object
	( const std::string& dlPath, const std::string& entry )
{
	void* handle = NULL;

	handle = dlopen( dlPath.c_str(  ), RTLD_LAZY );
	if ( !handle )
	{
		fprintf( stderr, "Error: %s\n", dlerror(  ) );
		throw std::runtime_error( "Unable to load shared librarys!" );
	}

	*( void** )( &game_init ) = dlsym( handle, entry.c_str(  ) );
	if ( !game_init )
	{
		printf( "%s\n", dlerror() );
		dlclose( handle );
		throw std::runtime_error( "Unable to link library's entry point!" );
	}
	dlHandles.push_back( handle );	//	ew
}

void engine_c::engine_main
	(  )
{
	static msgs_c msgs;
	static console_c console;

	this->msgs = &msgs;
	this->console = &console;

	load_object( "bin/client.so", "game_init" );	//	fix please or else i will yell at myself later for this being shit
	init_systems(  );
	add_game_systems(  );

	this->msgs->add( ENGINE_C, ENGI_PING );
	//this->msgs->add( GRAPHICS_C, GFIX_LOAD_SPRITE, 0, { "materials/textures/hilde_sprite_upscale.png" } );
	this->console->add( "ent_create" );
	
	for ( ; active; )
	{
		update_systems(  );
	}
}

void engine_c::init_systems
	(  )
{
	add_system( new graphics_c );
	add_system( new input_c );
	add_system( new audio_c );
	add_system( new gui_c );
	
	for ( const auto& sys : systems )
	{
		sys->msgs = msgs;
		sys->console = console;
		sys->init_subsystems(  );
	}
}

void engine_c::update_systems
	(  )
{
	update(  );

	for ( const auto& sys : systems )
	{
		sys->update(  );
	}
}

engine_c::engine_c
	(  )
{
	active = true;
	systemType = ENGINE_C;

	init_commands(  );
}

engine_c::~engine_c
	(  )
{
	for ( const auto& sys : systems )
	{
		sys->~system_c(  );
	}
	for ( const auto& handle : dlHandles )
	{
		dlclose( handle );
	}
}
