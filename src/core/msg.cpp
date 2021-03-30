#include "../../inc/core/msgs.h"

int msgs_c::fetch_msg
	( system_class_e type )
{
	for ( int i = 0; i < queue.size(  ); ++i )
	{
		if ( queue[ i ].type == type )
		{
			lastMsgIndex = i;
			return queue[ i ].msg;
		}
	}
	lastMsgIndex = -1;
	return -1;
}

void msgs_c::add
	( msg_s msg )
{
        queue.push_back( msg );
}

void msgs_c::remove
	( int index )
{
	queue.erase( queue.begin(  ) + index );
}
