#ifndef MSG_H
#define MSG_H

#include <functional>
#include <malloc.h>

typedef struct
{
	int type, msg;
	std::function< void( void**, int argsLen ) > func;
	int argsLen;
	void** args;
}msg_s;

#endif
