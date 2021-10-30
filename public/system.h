/*
system.h ( Authored by p0lyh3dron )

Defines the base system that all engine
systems inherit.
*/
#pragma once

#include "core/console.h"
#include "commandmanager.h"
#include "core/systemmanager.h"

#include "command.h"
#include "types/databuffer.hh"

#include <functional>

union SDL_Event;

class BaseSystem
{
protected:
	//typedef std::vector< Message > 				EngineCommands;
	typedef std::vector< ConCommand > 			ConsoleCommands;
	typedef std::vector< std::function< void(  ) > > 	FunctionList;

	FunctionList		aFreeQueue;

	/* Initializes all console commands the system can respond to.  */
	virtual void 	InitConsoleCommands(  );
	/* Initializes any member systems of the base system.  */
	virtual void    InitSubsystems(  );
public:
	/* Updates the system, performing all neccesary tasks.  */
	virtual void 		Update( float dt );
	/* Handle an SDL Event.  */
	virtual void 		HandleSDLEvent( SDL_Event* e );

	/* Constructor which will initialize the needed member variables.  */
	explicit	BaseSystem(  );
	/* Initialize system.  */
	virtual void 	Init(  );
	/* Destructs the system, freeing any used memory.  */
	virtual 	~BaseSystem(  ) = 0;
};
