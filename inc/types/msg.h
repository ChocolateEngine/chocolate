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
