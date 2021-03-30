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

	int fetch_msg
		( system_class_e type );

	void add
		( msg_s msg );
	void remove
		( int index );
};

#endif
