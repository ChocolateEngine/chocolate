#ifndef MSG_H
#define MSG_H

#include <functional>

typedef struct
{
	int type, msg;
	std::function< void(  ) > func;
	
}msg_s;

#endif
