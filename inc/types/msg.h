#ifndef MSG_H
#define MSG_H

#define FLAGS_EXTERNAL_SYSTEM 1 << 0 

#include <functional>
#include <malloc.h>
#include <any>

typedef struct msg_s
{
	int flags;
	int type, msg;
	std::function< void( std::vector< std::any > args ) > func;
	std::vector< std::any > args;
}msg_t;

#endif
