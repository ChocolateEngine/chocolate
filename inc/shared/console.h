#ifndef CONSOLE_H
#define CONSOLE_H

#include <string>
#include <vector>

#include "../types/msg.h"

typedef struct
{
	std::string str;
	msg_t msg;
}command_t;

class console_c
{
	protected:

	std::vector< command_t > commands;
	std::vector< std::string > queue;

	void init_commands
		(  );

	public:

	void add
		( std::string& cmd );
	void update
		(  );

	console_c
		(  );
	~console_c
		(  );
};

#endif
