/*
msg.h ( Authored by p0lyh3dron )

Defines data types used by the message bus
which is shared amongst all engine systems
*/

#pragma once

#define FLAGS_EXTERNAL_SYSTEM 1 << 0 

#include <functional>
#include <malloc.h>
#include <any>

class BaseCommand
{
public:
	/* Virtual deconstructor for the function.  */
	virtual 	~BaseCommand(  ){  }
};

template < class... Args >
class Command : public BaseCommand
{
private:
	typedef std::function< void( Args... ) > FuncType;
	FuncType	aFunction;
public:
		Command(  ){  }
  		Command( FuncType sFunction ) : aFunction( sFunction ){  }
	void 	operator(  )( Args... sArgs ){ if( aFunction ) aFunction( sArgs... ); }
};

struct PublishedFunction
{
	std::function< std::any( std::any( std::any, std::any ) ) >	aCallableFunction;
	std::string		     		aCallableString;
};

class Message
{
public:
	int flags;
	int type, msg;
	std::function< void( std::vector< std::any > args ) > func;
	std::vector< std::any > args;
};
