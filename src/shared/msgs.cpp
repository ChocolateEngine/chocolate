#include "../../inc/shared/msgs.h"

msg_s* msgs_c::fetch_msg
	( system_class_e type )
{
	for ( int i = 0; i < queue.size(  ); ++i )
	{
		if ( queue[ i ].type == type )
		{
			lastMsgIndex = i;
			return &queue[ i ];
		}
	}
	lastMsgIndex = -1;
	return 0x0;
}

void msgs_c::add
	( int type, int cmd, int argsLen, void** args )
{
	msg_s msg;
	msg.type 	= type;
	msg.msg 	= cmd;
	msg.argsLen 	= argsLen;
	msg.args 	= ( void** )malloc( argsLen * sizeof( void* ) );
	for ( int i = 0; i < argsLen; ++i )
	{
		msg.args[ i ] = args[ i ];
	}
        queue.push_back( msg );
}

void msgs_c::remove
	( int index )
{
	queue.erase( queue.begin(  ) + index );
}
