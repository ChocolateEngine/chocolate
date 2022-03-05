/*
 *	engine.h ( Authored by p0lyh3dron )
 *
 *	Defines the engine structure, with
 *	various systems and synchronizations
 */
#pragma once

#include <vector>
#include <functional>

#include "core/core.h"

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
	DataList	   aDlHandles;
	SystemList     aOrderedSystems;

	BaseGuiSystem *apGuiSystem = nullptr;
public:	
	bool 		aActive;
	/*
	 *    Initializes the engine.
	 */
	std::function< void() >           Init = [ & ](){
		/*
		 *    Create some engine specific console commands.
		 */
		CON_COMMAND_LAMBDA( exit ) { aActive = false; });
		CON_COMMAND_LAMBDA( quit ) { aActive = false; });
		/*
		 *    Grab the systems from libraries.
		 */
		InitSystems();
		/*
		 *    Register the convars that are declared.
		 */
		console->RegisterConVars();
		/*
		 *    Get systems that the engine needs to specifically
		 *    keep track of.
		 */
		apGuiSystem = GET_SYSTEM( BaseGuiSystem );
	};
	/*
	 *    Updates the engine.
	 *
	 *    @param float    The delta time.
	 */
	std::function< void( float ) >    Update = [ & ]( float sDT ) {
		console->Update();
		UpdateSystems( sDT );	
	};
	/*
	 *    Starts a new frame.
	 */
	std::function< void() >           StartFrame = [ & ]() {
		if ( apGuiSystem ) {
			apGuiSystem->StartFrame();
		}
		else {
			apGuiSystem = GET_SYSTEM( BaseGuiSystem );
			if ( !apGuiSystem ) {
				LogWarn( "No GUI found!" );
			}
		}
	};
	/*
	 *    Loads a library.
	 *
	 *	  @param const std::strnig &    The library name.
	 */
	void                    LoadModule( const std::string& srDlPath );
	/*
	 *    Initializes the systems.
	 */
	void DLL_EXPORT	       	InitSystems();
	/*
	 *    Updates all systems.  
	 */
	void DLL_EXPORT		    UpdateSystems( float sDT );
	/*
	 *    Set the engine to active on construction.
	 */
	explicit 	            Engine() : aActive( true ){};
	/*
	 *    Free memory.
	 */
	~Engine() {
		for ( const auto& pSys : systems->GetSystemList() )
			delete pSys;

		for ( const auto& handle : aDlHandles )
			SDL_UnloadObject( ( Module )handle );
	};

	std::string aGamePath;
	std::string aExePath;
};
