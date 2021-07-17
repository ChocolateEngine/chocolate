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

void console_c::clear
	(  )
{
	queue.clear(  );
}

std::string console_c::fetch_cmd
	(  )
{
	if ( empty(  ) || cmdIndex >= queue.size(  ) )
	{
		cmdIndex = 0;
		return "";
	}
	cmdIndex++;
	return queue[ cmdIndex - 1 ];
}

console_c::console_c
	(  )
{

}

console_c::~console_c
	(  )
{
	
}
