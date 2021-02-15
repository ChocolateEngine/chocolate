#ifndef MSGS_H
#define MSGS_H

#include <vector>

#include "../types/msg.h"
#include "../types/enums.h"

class msgs_c
{
	protected:

	public:

	std::vector< msg_s > queue;

	int fetch_msg
		( system_class_e type );
};

#endif
