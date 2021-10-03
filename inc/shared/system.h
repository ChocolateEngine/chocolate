/*
system.h ( Authored by p0lyh3dron )

Defines the base system that all engine
systems inherit.
*/
#pragma once

#define EXTERNAL_SYSTEM 1 << 0

#define SYSTEM_OBJECT( sClassName )		\
protected:					\
	void 	InitCommands(  ); 		\
	void	InitConsoleCommands(  ); 	\
public:						\
	void	InitSubsystems(  );		\
	        ~sClassName(  );		\
private:					\

#include "console.h"
#include "msgs.h"
#include "commandmanager.h"
#include "systemmanager.h"

#include "../types/enums.h"
#include "../types/msg.h"
#include "../types/command.h"
#include "../types/databuffer.hh"

#include <functional>

union SDL_Event;

class BaseSystem
{
protected:
	typedef std::vector< Message > 				EngineCommands;
	typedef std::vector< ConCommand > 			ConsoleCommands;
	typedef std::vector< std::function< void(  ) > > 	FunctionList;

	int 			aFlags = 0;
	int 			aSystemType;
	EngineCommands 		aEngineCommands;
	ConsoleCommands 	aConsoleCommands;
	FunctionList 		aFunctionList;
	FunctionList		aFreeQueue;

	/* Reads all messages that can be interpreted by the system.  */
	void 		ReadMessage(  );
	/* Adds a function that will be run on every frame.  */
	void 		AddUpdateFunction( std::function< void(  ) > sFunction );
	/* Adds a function to be ran on the destructor to free memory.  */
	void		AddFreeFunction( std::function< void(  ) > sFunction );
	/* Executes a list of functions.  */
	void 		ExecuteFunctions( FunctionList& srFunctionList );

	/* Initializes all commands the system can respond to.  */
	virtual void 	InitCommands(  );
	/* Initializes all console commands the system can respond to.  */
	virtual void 	InitConsoleCommands(  );
	/* Initializes any member systems of the base system.  */
	virtual void    InitSubsystems(  );
public:
	Messages 	        		*apMsgs 		= NULL;
	Console 				*apConsole 		= NULL;
	CommandManager				*apCommandManager 	= NULL;
	SystemManager				*apSystemManager 	= NULL;
	/* Updates the system, performing all neccesary tasks.  */
	virtual void 		Update( float dt );
	/* Adds a flag to the aFlags, e.g update twice  */
	void 		AddFlag( int sFlags );
	/* Removes a flag from aFlags e.g no longer needs to update twice.  */
	void 		RemoveFlag( int sFlags );
	/* Handle an SDL Event.  */
	virtual void 		HandleSDLEvent( SDL_Event* e );

	/* Constructor which will initialize the needed member variables.  */
	explicit	BaseSystem(  );
	/* Initialize system.  */
	virtual void 	Init(  );
	/* Send messages required for later stages of initialization.  */
	virtual void	SendMessages(  );
	/* Destructs the system, freeing any used memory.  */
	virtual 	~BaseSystem(  ) = 0;
};
