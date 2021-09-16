/*
input.cpp ( Authored by p0lyh3dron )

Defines the methods declared in
input.h
*/
#include "../../inc/core/input.h"
#include "../../inc/imgui/imgui.h"
#include "../../inc/imgui/imgui_impl_sdl.h"

#include "../../inc/core/engine.h"
#include "../../inc/core/gui.h"

#include <fstream>
#include <iostream>

InputSystem::InputSystem(  ) : BaseInputSystem(  )
{
	aSystemType = INPUT_C;

	MakeAliases(  );
	ParseBindings(  );
}

void InputSystem::MakeAliases(  )
{
	std::ifstream 	in( "resource/SDL_Alias_List.txt" );
	if ( !in.is_open(  ) )
		return;
	std::string 	alias;
        int 		scanCode;
	while ( in >> alias >> scanCode )
	{
	        KeyAlias 	key{  };
		key.aAlias 	= alias;
		key.aCode 	= SDL_SCANCODE_TO_KEYCODE( scanCode );
		aKeyAliases.push_back( key );

		std::cout << alias << " using scanCode " << scanCode << std::endl;
		in >> alias;	//	Hack to skip comment
	}
}

void InputSystem::ParseBindings(  )
{
	std::ifstream 	in( "cfg/bindings.cfg" );
	if ( !in.is_open(  ) )
		return;
	std::string 	bind;
	std::string	cmd;
        while ( in >> bind >> cmd )
	{
	        KeyBind binding{  };
		binding.aBind 	= bind;
		binding.aCmd 	= cmd;
		aKeyBinds.push_back( binding );

		std::cout << cmd << " bound to " << bind << std::endl;
	}
}

void InputSystem::Bind( const std::string& srKey, const std::string& srCmd )
{
	for ( const auto& alias : aKeyAliases )
		if ( alias.aAlias == srKey )
		{
			for ( int i = 0; i < aKeyBinds.size(  ); ++i )
				if ( aKeyBinds[ i ].aBind == srKey )
					aKeyBinds.erase( aKeyBinds.begin(  ) + i );
		        KeyBind bind{  };
			bind.aBind 	= srKey;
			bind.aCmd 	= srCmd;
			aKeyBinds.push_back( bind );

			std::cout << srCmd << " bound to " << srKey << std::endl;
			return;
		}
	std::cout << srKey << " is not a valid key alias" << std::endl;
}

void InputSystem::ParseInput(  )
{
	for ( ; SDL_PollEvent( &aEvent ) ; )
	{
		ImGui_ImplSDL2_ProcessEvent( &aEvent );

		switch (aEvent.type)
		{
			case SDL_QUIT:
			{
				apCommandManager->Execute( Engine::Commands::EXIT );
				break;
			}

			case SDL_KEYDOWN:
			{
				if ( aEvent.key.keysym.sym == SDLK_BACKQUOTE )
					apCommandManager->Execute( GuiSystem::Commands::SHOW_CONSOLE );
				else
					for ( const auto& alias : aKeyAliases )
						if ( aEvent.key.keysym.sym == alias.aCode )
							for ( const auto& bind : aKeyBinds )
								if ( bind.aBind == alias.aAlias )
									apConsole->Add( bind.aCmd );

				break;
			}

			case SDL_MOUSEMOTION:
			{
				aMousePos.x = aEvent.motion.x;
				aMousePos.y = aEvent.motion.y;
				aMouseDelta.x += aEvent.motion.xrel;
				aMouseDelta.y += aEvent.motion.yrel;
				break;
			}
		}

		for ( BaseSystem* sys: apSystemManager->GetSystemList() )
		{
			sys->HandleSDLEvent( &aEvent );
		}
	}
}


CON_COMMAND( bind )
{
	// uhhhhh, should probably have an option to have a void* for the data we need or something, like a class
	// Bind( sArgs[ 1 ], sArgs[ 2 ] );
}


void InputSystem::InitConsoleCommands(  )
{
	// doesn't work at the moment
	/*ConCommand bind( "bind", [ & ]( std::vector< std::string > sArgs )
	{
		if ( sArgs.size(  ) < 3 )
		{
			printf( "Insufficient arguments for bind\n" );
			return;
		}
		Bind( sArgs[ 1 ], sArgs[ 2 ] );
	} );*/

	BaseSystem::InitConsoleCommands(  );
}


void InputSystem::Update( float frameTime )
{
	ResetInputs(  );
	ParseInput(  );
}


void InputSystem::ResetInputs(  )
{
	aMouseDelta = {0, 0};
}

const glm::vec2& InputSystem::GetMouseDelta(  )
{
	return aMouseDelta;
}

const glm::vec2& InputSystem::GetMousePos(  )
{
	return aMousePos;
}


