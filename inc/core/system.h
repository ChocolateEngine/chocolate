#ifndef SYSTEM_H
#define SYSTEM_H

#include "../shared/console.h"
#include "../shared/msgs.h"

#include "../types/enums.h"
#include "../types/msg.h"
#include "../types/command.h"

#include <functional>

class system_c
{
	protected:

	system_class_e systemType;
	std::vector< msg_s > engineCommands;
	std::vector< command_s > userCommands;
	std::vector< std::function< void(  ) > > funcList;

	void read_msg
		(  );
	void delete_msg
		( msg_s& msg );
	void read_console
		(  );
	void add_func
		( std::function< void(  ) > func );
	void exec_funcs
		(  );
	
	public:

	msgs_c* msgs;
	console_c* console;
	
	void update
		(  );

	system_c
		(  );
	virtual ~system_c
		(  );
};

#endif
