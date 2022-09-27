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
#include "ch_iengine.h"


// NOTE: this should probably just be moved to core and have this dll just completely nuked
class Engine: public Ch_IEngine
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
	void Init( const std::vector< std::string >& srModulePaths ) override;

	/*
	*    Updates the engine.
	*
	*    @param float    The delta time.
	*/
	void Update( float sDT ) override
	{
		Con_Update();
		UpdateSystems( sDT );	
	}

	bool IsActive() override
	{
		return aActive;
	}

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
				Log_Warn( "No GUI found!" );
			}
		}
	};
	/*
	 *    Loads a library.
	 *
	 *	  @param const std::strnig &    The library name.
	 */
	BaseSystem*             LoadModule( const std::string& srDlPath );
	/*
	 *    Updates all systems.  
	 */
	void                    UpdateSystems( float sDT );
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
