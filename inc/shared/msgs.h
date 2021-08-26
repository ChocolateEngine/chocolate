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

#include "../types/msg.h"
#include "../types/enums.h"

class CommandManager
{
	typedef std::shared_ptr< BaseCommand > 		BaseCommandPtr;
	typedef std::map< std::string, BaseCommandPtr > FMap;
public:
	template < class T >
		void Add( std::string sName, const T& srCmd )
		{
			//aFmap.insert( std::pair< std::string, BaseCommandPtr >( sName, BaseCommandPtr( new T ( srCmd ) ) ) );
		}
	template < class... Args >	
		void Execute( std::string sName, Args... sArgs )
		{
			typedef Command< Args... > 	CommandType;
			FMap::const_iterator it = aFmap.find( sName );
			if( it != aFmap.end(  ) )
			{
				CommandType *c = reinterpret_cast< CommandType* >( it->second.get(  ) );
				if( c )
				{
					( *c )( sArgs... );
				}
			}
		}
private:
	FMap aFmap;
};

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
	CommandManager 	aCmdManager;
        MessagesList 	aQueue;
        FunctionList 	aPublishedFunctions;
	/* A.  */
	Message 		*FetchMessage( int sType, int sFlags = 0 );
	/* A.  */
	void	 	Add( int sType, int sCmd, int sFlags = 0, const Arbitrary &srArgs = Arbitrary(  ) );
	/* A.  */
	void 		Remove( int sIndex );
};
