#ifndef SYSTEM_H
#define SYSTEM_H

#define EXTERNAL_SYSTEM 1 << 0

#include "console.h"
#include "msgs.h"

#include "../types/enums.h"
#include "../types/msg.h"
#include "../types/command.h"

#include <functional>

class system_c
{
	protected:

	int systemType;
	std::vector< msg_s > engineCommands;
	std::vector< command_s > userCommands;
	std::vector< std::function< void(  ) > > funcList;

	int flags = 0;
	
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

	msgs_c* msgs = NULL;
	console_c* console = NULL;
	
	void update
		(  );

	void add_flag
		( int flagsIn );
	void rm_flag
		( int flagsIn );

	system_c
		(  );
	virtual void send_messages
		(  );
	virtual void init_subsystems
		(  );
	virtual ~system_c
		(  );
};

#endif
