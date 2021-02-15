#ifndef SYSTEM_H
#define SYSTEM_H

#include "console.h"
#include "msgs.h"

#include "../types/enums.h"

class system_c
{
	protected:

	system_class_e systemType;

	void read_msg
		(  );
	void read_console
		(  );
	
	public:

	msgs_c* msgs;
	console_c* console;
	
	void update
		(  );
};

#endif
