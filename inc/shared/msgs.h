#ifndef MSGS_H
#define MSGS_H

#include <vector>

#include "../types/msg.h"
#include "../types/enums.h"

class msgs_c
{
	protected:

	public:

	int lastMsgIndex;

	std::vector< msg_s > queue;

	msg_s* fetch_msg
		( system_class_e type );

	void add
		( int type, int cmd, int argsLen = 0, void** args = NULL );
	void remove
		( int index );
};

#endif
