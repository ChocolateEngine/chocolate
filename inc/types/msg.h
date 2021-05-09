#ifndef MSG_H
#define MSG_H

#include <functional>
#include <malloc.h>

typedef struct
{
	void** args;
	int argsLen;

	void allocate
		( void** in, int numArgs )
		{
			argsLen = 0;
			args 	= NULL;
			for ( int i = 0; i < numArgs; ++ i )
			{
				args[ i ] = malloc( sizeof( void* ) );
				args[ i ] = in[ i ];
				argsLen++;
			}
		}
	void destruct
		(  )
		{
			
		}
}args_t;

typedef struct
{
	int type, msg;
	std::function< void( void**, int argsLen ) > func;
	void** args;
	int argsLen;
	
}msg_s;

#endif
