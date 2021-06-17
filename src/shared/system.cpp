#include "../../inc/shared/system.h"

void system_c::read_msg
	(  )
{
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
				cmd.func( msg->args, msg->argsLen );
				delete_msg( *msg );
			}
		}
		msgs->remove( msgs->lastMsgIndex );
	}
}

void system_c::delete_msg
	( msg_s& msg )
{
	free( msg.args );
}

void system_c::read_console
	(  )
{

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

}

system_c::~system_c
	(  )
{
	
}
