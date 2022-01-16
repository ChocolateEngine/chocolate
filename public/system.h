/*
system.h ( Authored by p0lyh3dron )

Defines the base system that all engine
systems inherit.
*/
#pragma once

#include "core/console.h"
#include "core/systemmanager.h"
#include "types/databuffer.hh"

#include <functional>

union SDL_Event;

class BaseSystem
{
protected:
public:
	/* Self explanatory.  */
	virtual void     Update( float sDT ) = 0;
	/* Initialize system.  */
	virtual void 	 Init()              = 0;
	/* Destructs the system, freeing any used memory.  */
	virtual 	~BaseSystem()        = 0;
};
