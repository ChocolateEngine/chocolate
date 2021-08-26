#include "../../inc/shared/msgs.h"

bool SharesFlag( int sX, int sY, int sFlag )
{
	int x1 = ( sX & sFlag );
	int y1 = ( sY & sFlag );
	if ( x1 == y1 )
	{
		return true;
	}
	return false;
}

Message *Messages::FetchMessage( int sType, int sFlags )
{
	for ( int i = 0; i < aQueue.size(  ); ++i )
	{
		if ( aQueue[ i ].type == sType && aQueue[ i ].flags == sFlags )
		{
			aLastMsgIndex = i;
			return &aQueue[ i ];
		}
	}
	aLastMsgIndex = -1;
	return 0x0;
}

void Messages::Add( int sType, int sCmd, int sFlags, const Arbitrary &srArgs )
{
	Message msg;
	msg.type 	= sType;
	msg.msg 	= sCmd;
	msg.flags 	= sFlags;
	msg.args	= srArgs;
        aQueue.push_back( msg );
}

void Messages::Remove( int sIndex )
{
	aQueue.erase( aQueue.begin(  ) + sIndex );
}
