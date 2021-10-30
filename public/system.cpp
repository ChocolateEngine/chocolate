#include "system.h"
#include "core/systemmanager.h"

#include <SDL2/SDL.h>


void BaseSystem::InitConsoleCommands(  )
{
}

void BaseSystem::InitSubsystems(  )
{
	
}

void BaseSystem::Update( float dt )
{
}

void BaseSystem::HandleSDLEvent( SDL_Event* e )
{
	
}

BaseSystem::BaseSystem(  )
{

}


void BaseSystem::Init(  )
{
	InitConsoleCommands(  );
	InitSubsystems(  );
}


BaseSystem::~BaseSystem(  )
{
}
