/*
console.h ( Authored by p0lyh3dron )

Declares the sdfhuosdfhuiosdfhusdfhuisfhu
*/

#pragma once

#include <string>
#include <vector>
#include <functional>

#include "../types/msg.h"

class ConCommand
{
public:
	std::string str;
	std::function< void( std::vector< std::string > args ) > func;
};

class Console
{
	typedef std::string 			String;
	typedef std::vector< std::string >	StringList;
protected:
	int 				aCmdIndex = 0;
        StringList 			aQueue;
public:
	/* A.  */
	bool 	Empty(  );
	/* A.  */
	void 	Add( const String &srCmd );
	/* A.  */
	void 	DeleteCommand(  );
	/* A.  */
	void 	Clear(  );
	/* A.  */
	void 	MoveCommands(  );
	/* A.  */
	String 	FetchCmd(  );
	/* A.  */
		Console(  );
	/* A.  */
		~Console(  );
};
