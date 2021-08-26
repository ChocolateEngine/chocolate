#include "../../inc/shared/console.h"

bool Console::Empty(  )
{
        return aQueue.empty(  ) ? true : false;
}

void Console::Add( const String &srCmd )
{
	aQueue.push_back( srCmd );
}

void Console::DeleteCommand(  )
{
	aQueue.erase( aQueue.begin(  ) + aCmdIndex );
}

void Console::Clear(  )
{
	aQueue.clear(  );
}

std::string Console::FetchCmd(  )
{
	if ( Empty(  ) || aCmdIndex >= aQueue.size(  ) )
	{
		aCmdIndex = 0;
		return "";
	}
	aCmdIndex++;
	return aQueue[ aCmdIndex - 1 ];
}

Console::Console(  )
{

}

Console::~Console(  )
{
	
}
