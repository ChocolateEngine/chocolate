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
	( int type, int cmd, int flags, const std::vector< std::any >& args  )
{
	msg_s msg;
	msg.type 	= type;
	msg.msg 	= cmd;
	msg.flags 	= flags;
	msg.args	= args;
        queue.push_back( msg );
}

void msgs_c::AddFunction( PublishedFunction &srPublishedFunction )
{
	aPublishedFunctions.push_back( srPublishedFunction );
}

std::any msgs_c::Call( const std::string& srFunctionName,
		       const std::vector< std::any > sArgs )
{
	
}

void msgs_c::remove
	( int index )
{
	queue.erase( queue.begin(  ) + index );
}
