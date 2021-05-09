#ifndef MSG_H
#define MSG_H

#include <functional>

typedef struct
{
	int type, msg;
	std::function< void( void**, int argsLen ) > func;
	void** args;
	int argsLen;
	
}msg_s;

#endif
