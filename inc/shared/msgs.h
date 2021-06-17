#ifndef MSGS_H
#define MSGS_H

#include <vector>

#include "../types/msg.h"
#include "../types/enums.h"

class msgs_c
{
	protected:

	bool shares_flag
		( int x, int y, int in );

	public:

	int lastMsgIndex;

	std::vector< msg_s > queue;

	msg_s* fetch_msg
		( int type, int flags = 0 );

	void add
		( int type, int cmd, int flags = 0, int argsLen = 0, void** args = NULL );
	void remove
		( int index );
};

#endif
