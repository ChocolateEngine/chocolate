/*
msgs.h ( Authored by p0lyh3dron )

Defines the message bus, an object that
all systems rely on for communication.
*/

#pragma once

#include <vector>
#include <any>
#include <string>
#include <memory>
#include <map>
#include <unordered_map>
#include <typeindex>

#include "../types/msg.h"
#include "../types/enums.h"

class Messages
{
	typedef std::vector< Message > 			MessagesList;
	typedef std::vector< PublishedFunction >	FunctionList;
	typedef std::vector< std::any >			Arbitrary;
protected:
	/* A.  */
	bool 		SharesFlag( int sX, int sY, int sFlag );
public:
	int 		aLastMsgIndex;
        MessagesList 	aQueue;
        FunctionList 	aPublishedFunctions;
	/* A.  */
	Message         *FetchMessage( int sType, int sFlags = 0 );
	/* A.  */
	void	 	Add( int sType, int sCmd, int sFlags = 0, const Arbitrary &srArgs = Arbitrary(  ) );
	/* A.  */
	void 		Remove( int sIndex );
};
