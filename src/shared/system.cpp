#include "../../inc/shared/system.h"

void system_c::read_msg
	(  )
{
	if ( !msgs )
	{
		return;
	}
	msg_s* msg;
	for ( ; ; )
	{
		if ( !( msg = msgs->fetch_msg( systemType, flags ) ) )
		{
			return;
		}
		for ( auto cmd : engineCommands )
		{	
			if ( cmd.msg == msg->msg )
			{
				cmd.func( msg->args );
				delete_msg( *msg );
			}
		}
		msgs->remove( msgs->lastMsgIndex );
	}
}

void system_c::delete_msg
	( msg_s& msg )
{
	
}

void system_c::read_console
	(  )
{
	std::string command;
	if ( !console || consoleCommands.empty(  ) )
	{
		return;
	}
	for ( ; ; )
	{
		if ( console->empty(  ) )
		{
			return;
		}
		command = console->fetch_cmd(  );
		if ( command == "" )
		{
			return;
		}
		for ( const auto& cmd : consoleCommands )
		{
			if ( cmd.str == command )
			{
				std::vector< std::string > args;
				int start, end = 0;
				for ( ; ( start = command.find_first_not_of( ' ', end ) ) != std::string::npos ; )
				{
					end = command.find( ' ', start );
					args.push_back( command.substr( start, end - start ) );
				}
				cmd.func( args );
				console->delete_command(  );
			}
		}
	}
}

void system_c::add_func
	( std::function< void(  ) > func )
{
	funcList.push_back( func );
}

void system_c::exec_funcs
	(  )
{
	for ( const auto& func : funcList )
	{
		func(  );
	}
}

void system_c::init_commands
	(  )
{
	
}

void system_c::init_console_commands
	(  )
{
	
}

void system_c::update
	(  )
{
	read_msg(  );
	read_console(  );
	exec_funcs(  );
}

void system_c::add_flag
	( int flagsIn )
{
	flags |= flagsIn;
}
	
void system_c::rm_flag
	( int flagsIn )
{
	
}

system_c::system_c
	(  )
{
	init_commands(  );
	init_console_commands(  );
}

void system_c::send_messages
	(  )
{
	
}

void system_c::init_subsystems
	(  )
{
	
}

system_c::~system_c
	(  )
{
	
}
