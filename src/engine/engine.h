/*
engine.h ( Authored by p0lyh3dron )

Defines the engine structure, with
various systems and synchronizations
*/
#pragma once

#include "system.h"
#include "graphics/igraphics.h"
#include "input.h"
#include "iaudio.h"
#include "igui.h"

#include <vector>

#define CRY_ABOUT_IT() \
	apCommandManager->Execute( Engine::Commands::EXIT );

class Engine
{
protected:
	typedef std::vector< BaseSystem* >      SystemList;
	typedef std::vector< void* > 	        DataList;

	DataList	aDlHandles;

	/* Entry for the game code.  */
	void 		( *game_init )( SystemList& ) = NULL;

	/* Adds the game systems to aSystems.  */
	void 		AddGameSystems(  );
public:
	enum class	Commands{ NONE = 0, PING, EXIT };
	
	bool 		aActive;
	/* Loads a shared object containing the game code.  */
	void 		LoadObject( const std::string& srDlPath,
				    const std::string& srEntry );

	void            Update( float sDT );

	void        LoadModule( const std::string& srDlPath );

	/* The engine update cycle is stored here.  */
	void 		EngineMain(  );
	/* Initializes engine systems.  */
	void 	       	InitSystems(  );
	/* Updates all systems.  */
	void 		UpdateSystems(  );
	/* Initialize the engine, creating systems, etc.  */
	explicit 	Engine(  );
		        ~Engine(  );

	std::string aGamePath;
	std::string aExePath;
};
