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
			aFmap.insert( std::pair< std::string, BaseCommandPtr >( sName, BaseCommandPtr( new T ( srCmd ) ) ) );
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

class msgs_c
{
	protected:

	bool shares_flag
		( int x, int y, int in );

	public:

	int lastMsgIndex;

	CommandManager aCmdManager;

	std::vector< msg_s > queue;
	std::vector< PublishedFunction > aPublishedFunctions;

	msg_s* fetch_msg
		( int type, int flags = 0 );

	void add
		( int type, int cmd, int flags = 0, const std::vector< std::any >& args = std::vector< std::any >(  ) );
	/* Function that adds a published function to the list to be called.  */
	void		AddFunction( PublishedFunction& srPublishedFunction );
	/* Calls a published function and returns the arbitrary type.  */
	std::any	Call( const std::string& srFunctionName,
			      const std::vector< std::any > sArgs =
			      std::vector< std::any >(  ) );
	void remove
		( int index );
};
