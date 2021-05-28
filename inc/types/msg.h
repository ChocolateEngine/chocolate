#ifndef MSG_H
#define MSG_H

#include <functional>
#include <malloc.h>

typedef struct msg_s
{
	int type, msg;
	std::function< void( void**, int argsLen ) > func;
	int argsLen;
	void** args;
}msg_t;

#endif
