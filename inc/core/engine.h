/*
engine.h ( Authored by p0lyh3dron )

Defines the engine structure, with
various systems and synchronizations
*/
#pragma once

#include "../shared/system.h"
#include "graphics.h"
#include "input.h"
#include "audio.h"
#include "gui.h"

#include <vector>

#define CRY_ABOUT_IT() \
	apCommandManager->Execute( Engine::Commands::EXIT );

class Engine : public BaseSystem
{
protected:
	typedef std::vector< BaseSystem* >      SystemList;
	typedef std::vector< void* > 	        DataList;

	SystemList	aSystems;
	DataList	aDlHandles;

	/* Entry for the game code.  */
	void 		( *game_init )( SystemList& ) = NULL;

	void		AddSystem(  ){  }
	/* Adds a system to aSystems.  */
	template< typename T, typename... TArgs >
        void 		AddSystem( const T *spSystem = NULL, TArgs... sSystems );

	/* Adds the game systems to aSystems.  */
	void 		AddGameSystems(  );
	/* Initializes all commands the system can respond to.  */
	void 		InitCommands(  );
	/* Initializes all console commands the system can respond to.  */
	void 		InitConsoleCommands(  );
public:
	enum class	Commands{ NONE = 0, PING, EXIT };
	
	bool 		aActive;
	/* Loads a shared object containing the game code.  */
	void 		LoadObject( const std::string& srDlPath,
				    const std::string& srEntry );

	/* The engine update cycle is stored here.  */
	void 		EngineMain(  );
	/* Initializes engine systems.  */
	void 	       	InitSystems(  );
	/* Updates all systems.  */
	void 		UpdateSystems(  );
	/* Initialize the engine, creating systems, etc.  */
	explicit 	Engine(  );
		        ~Engine(  );
};
