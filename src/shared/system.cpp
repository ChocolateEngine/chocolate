#include "../../inc/shared/system.h"
#include "../../inc/shared/systemmanager.h"

#include <SDL2/SDL.h>


SystemManager* systems = nullptr;


void BaseSystem::ReadMessage(  )
{
	if ( !apMsgs )
		return;
	Message 	*pMsg;
	while ( true )
	{
		/* No more messages for this system.  */
		if ( !( pMsg = apMsgs->FetchMessage( aSystemType, aFlags ) ) )
			return;
		for ( auto&& cmd : aEngineCommands )
			/* If the message is used by the system.  */
			if ( cmd.msg == pMsg->msg )
				cmd.func( pMsg->args );
		/* Get rid of the spent message.  */
		apMsgs->Remove( apMsgs->aLastMsgIndex );
	}
}

void BaseSystem::AddUpdateFunction( std::function< void(  ) > sFunction )
{
	aFunctionList.push_back( sFunction );
}

void BaseSystem::AddFreeFunction( std::function< void(  ) > sFunction )
{
	aFreeQueue.push_back( sFunction );
}

void BaseSystem::ExecuteFunctions( FunctionList& srFunctionList )
{
	for ( const auto& func : srFunctionList )
		func(  );
}

void BaseSystem::InitCommands(  )
{
	
}

void BaseSystem::InitConsoleCommands(  )
{
	// really should move this to when loading a dll, before even calling this
	// maybe some DLLInit function
	apConsole->RegisterConVars(  );
}

void BaseSystem::InitSubsystems(  )
{
	
}

void BaseSystem::Update( float dt )
{
	ReadMessage(  );
	ExecuteFunctions( aFunctionList );
}

void BaseSystem::AddFlag( int sFlags )
{
	aFlags |= sFlags;
}
	
void BaseSystem::RemoveFlag( int sFlags )
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
	InitCommands(  );
	InitConsoleCommands(  );
	InitSubsystems(  );
}

void BaseSystem::SendMessages(  )
{
	
}

BaseSystem::~BaseSystem(  )
{
	/* Free all memory allocated by the system.  */
	ExecuteFunctions( aFreeQueue );
}
