#include "../../inc/core/system.h"

void system_c::read_msg
	(  )
{
	msg_s* msg;
	for ( ; ; )
	{
		if ( !( msg = msgs->fetch_msg( systemType ) ) )
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

void system_c::update
	(  )
{
	read_msg(  );
	read_console(  );
}

system_c::system_c
	(  )
{

}
