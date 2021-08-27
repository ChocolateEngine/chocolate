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
		if ( aEvent.type == SDL_QUIT )
			apCommandManager->Execute( Engine::Commands::EXIT );
		if ( aEvent.type == SDL_KEYDOWN )
		{
			if ( aEvent.key.keysym.sym == SDLK_BACKQUOTE )
				apCommandManager->Execute( GuiSystem::Commands::SHOW_CONSOLE );
			else
				for ( const auto& alias : aKeyAliases )
					if ( aEvent.key.keysym.sym == alias.aCode )
						for ( const auto& bind : aKeyBinds )
							if ( bind.aBind == alias.aAlias )
								apConsole->Add( bind.aCmd );
		}
	}
}

void InputSystem::InitConsoleCommands(  )
{
	ConCommand cmd;

	cmd.str = "bind";
	cmd.func = [ & ]( std::vector< std::string > sArgs )
		{
			if ( sArgs.size(  ) < 3 )
			{
				printf( "Insufficient arguments for bind\n" );
				return;
			}
			Bind( sArgs[ 1 ], sArgs[ 2 ] );
		};
	aConsoleCommands.push_back( cmd );
}

InputSystem::InputSystem(  ) : BaseSystem(  )
{
	aSystemType = INPUT_C;
        AddUpdateFunction( [ & ](  ){ ParseInput(  ); } );
	
        MakeAliases(  );
        ParseBindings(  );
}
