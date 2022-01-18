/*
 *	engine.h ( Authored by p0lyh3dron )
 *
 *	Defines the engine structure, with
 *	various systems and synchronizations
 */
#pragma once

#include <vector>
#include <functional>

#include "core/platform.h"

#include "system.h"
#include "graphics/igraphics.h"
#include "iinput.h"
#include "iaudio.h"
#include "igui.h"

class Engine
{
	using SystemList = std::vector< BaseSystem* >;
	using DataList   = std::vector< void* >;
protected:
	DataList	aDlHandles;
	SystemList      aOrderedSystems;
public:	
	bool 		aActive;
	std::function< void() >           Init = [ & ](){
		CON_COMMAND_LAMBDA( exit ) { aActive = false; });
		CON_COMMAND_LAMBDA( quit ) { aActive = false; });

		InitSystems();

		console->RegisterConVars(  );
	};
	std::function< void( float ) >    Update = [ & ]( float sDT ) {
		console->Update();
		UpdateSystems( sDT );	
	};
	void            LoadModule( const std::string& srDlPath );
	/* Initializes engine systems.  */
	void DLL_EXPORT	       	InitSystems(  );
	/* Updates all systems.  */
	void DLL_EXPORT		UpdateSystems( float sDT );
	/* Initialize the engine, creating systems, etc.  */
	explicit 	Engine(  ) : aActive( true ){};
			~Engine(  ){
				for ( const auto& pSys : systems->GetSystemList() )
					delete pSys;

				for ( const auto& handle : aDlHandles )
					SDL_UnloadObject( (Module)handle );
			};

	std::string aGamePath;
	std::string aExePath;
};
