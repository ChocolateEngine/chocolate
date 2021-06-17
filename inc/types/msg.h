#ifndef MSG_H
#define MSG_H

#define FLAGS_EXTERNAL_SYSTEM 1 << 0 

#include <functional>
#include <malloc.h>

typedef struct msg_s
{
	int flags;
	int type, msg;
	std::function< void( void**, int argsLen ) > func;
	int argsLen;
	void** args;
}msg_t;

#endif
