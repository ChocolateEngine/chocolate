/*
system.h ( Authored by p0lyh3dron )

Defines the base system that all engine
systems inherit.
*/
#pragma once

class ISystem
{
protected:
public:
	/* Self explanatory.  */
	virtual void     Update( float sDT ) = 0;
	/* Initialize system.  */
	virtual bool 	 Init()              = 0;
	/* Destructs the system, freeing any used memory.  */
	virtual 	~ISystem()            = default;
};
