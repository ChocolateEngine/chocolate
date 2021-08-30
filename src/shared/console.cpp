#include "../../inc/shared/console.h"

bool Console::Empty(  )
{
	if ( aQueue.empty() )
		aCmdIndex = -1;

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
	aCmdIndex++;
	if ( Empty(  ) || aCmdIndex >= aQueue.size(  ) )
	{
		aCmdIndex = 0;
		return "";
	}
	return aQueue[ aCmdIndex ];
}

Console::Console(  )
{

}

Console::~Console(  )
{
	
}
