#ifndef SYSTEM_H
#define SYSTEM_H

#include "console.h"
#include "msgs.h"

#include "../types/enums.h"
#include "../types/msg.h"
#include "../types/command.h"

class system_c
{
	protected:

	system_class_e systemType;
	std::vector< msg_s > engineCommands;
	std::vector< command_s > userCommands;

	void read_msg
		(  );
	void read_console
		(  );
	
	public:

	msgs_c* msgs;
	console_c* console;
	
	void update
		(  );

	system_c
		(  );
};

#endif
