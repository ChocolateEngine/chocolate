#ifndef CONSOLE_H
#define CONSOLE_H

#include <string>
#include <vector>
#include <functional>

#include "../types/msg.h"

typedef struct command_s
{
	std::string str;
	std::function< void( std::vector< std::string > args ) > func;
}command_t;

class console_c
{
	protected:

	int cmdIndex = 0;
	std::vector< std::string > queue;

	public:

	bool empty
		(  );

	void add
		( const std::string& cmd );
	void delete_command
		(  );
	void clear
		(  );
	void move_commands
		(  );
        std::string fetch_cmd
		(  );

	console_c
		(  );
	~console_c
		(  );
};

#endif
