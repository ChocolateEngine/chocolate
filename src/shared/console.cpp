#include "../../inc/shared/console.h"

bool console_c::empty
	(  )
{
        return queue.empty(  ) ? true : false;
}

void console_c::add
	( const std::string& cmd )
{
	queue.push_back( cmd );
}

void console_c::delete_command
	(  )
{
	queue.erase( queue.begin(  ) + cmdIndex );
}

std::string console_c::fetch_cmd
	(  )
{
	if ( empty(  ) )
	{
		cmdIndex = 0;
		return "";
	}
	return queue[ cmdIndex ];
}

console_c::console_c
	(  )
{

}

console_c::~console_c
	(  )
{
	
}
