#include "../../inc/shared/msgs.h"

bool msgs_c::shares_flag
	( int x, int y, int in )
{
	int x1 = ( x & in );
	int y1 = ( y & in );
	if ( x1 == y1 )
	{
		return true;
	}
	return false;
}

msg_s* msgs_c::fetch_msg
	( int type, int flags )
{
	for ( int i = 0; i < queue.size(  ); ++i )
	{
		if ( queue[ i ].type == type && queue[ i ].flags == flags )
		{
			lastMsgIndex = i;
			return &queue[ i ];
		}
	}
	lastMsgIndex = -1;
	return 0x0;
}

void msgs_c::add
	( int type, int cmd, int flags, int argsLen, void** args )
{
	msg_s msg;
	msg.type 	= type;
	msg.msg 	= cmd;
	msg.flags 	= flags;
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
