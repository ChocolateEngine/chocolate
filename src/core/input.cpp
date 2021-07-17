#include "../../inc/core/input.h"
#include "../../inc/imgui/imgui.h"
#include "../../inc/imgui/imgui_impl_sdl.h"

#include <fstream>
#include <iostream>

void input_c::make_aliases
		(  )
{
	std::ifstream in( "resource/SDL_Alias_List.txt" );
	if ( !in.is_open(  ) )
	{
		return;
	}
	std::string alias;
	int scanCode;
	for ( ; in >> alias >> scanCode; )
	{
		key_alias_t key{  };
		key.alias 	= alias;
		key.code 	= SDL_SCANCODE_TO_KEYCODE( scanCode );
		keyAliases.push_back( key );

		std::cout << alias << " using scanCode " << scanCode << std::endl;
		in >> alias;	//	Hack to skip comment
	}
}

void input_c::parse_bindings
		(  )
{
	std::ifstream in( "cfg/bindings.cfg" );
	if ( !in.is_open(  ) )
	{
		return;
	}
	std::string bind, cmd;
	for ( ; in >> bind >> cmd; )
	{
		keybind_t binding{  };
		binding.bind 	= bind;
		binding.cmd 	= cmd;
		keyBinds.push_back( binding );

		std::cout << cmd << " bound to " << bind << std::endl;
	}
}

void input_c::bind
	( const std::string& key, const std::string& cmd )
{
	for ( const auto& alias : keyAliases )
	{
		if ( alias.alias == key )
		{
			for ( int i = 0; i < keyBinds.size(  ); ++i )
			{
				if ( keyBinds[ i ].bind == key )
				{
					keyBinds.erase( keyBinds.begin(  ) + i );
				}
			}
			keybind_t bind{  };
			bind.bind 	= key;
			bind.cmd 	= cmd;
			keyBinds.push_back( bind );

			std::cout << cmd << " bound to " << key << std::endl;
		}
	}
}

void input_c::parse_input
	(  )
{
	for ( ; SDL_PollEvent( &event ) ; )
	{
		ImGui_ImplSDL2_ProcessEvent( &event );
		if ( event.type == SDL_QUIT )
		{
			msgs->add( ENGINE_C, ENGI_EXIT );
		}
		if ( event.type == SDL_KEYDOWN )
		{
			if ( event.key.keysym.sym == SDLK_BACKQUOTE )
			{
				msgs->add( GUI_C, LOAD_IMGUI_DEMO );
			}
			else
			{
				for ( const auto& alias : keyAliases )
				{
					if ( event.key.keysym.sym == alias.code )
					{
						for ( const auto& bind : keyBinds )
						{
							if ( bind.bind == alias.alias )
							{
								console->add( bind.cmd );
							}
						}
					}
				}
			}
		}
	}
}

void input_c::init_console_commands
	(  )
{
	command_t cmd;

	cmd.str = "bind";
	cmd.func = [ & ]( std::vector< std::string > args )
		{
			bind( args[ 0 ], args[ 1 ] );
		};
	consoleCommands.push_back( cmd );
	printf( "added function bitch\n" );
}

input_c::input_c
	(  )
{
	systemType = INPUT_C;
	add_func( [ & ](  ){ parse_input(  ); } );
	make_aliases(  );
	parse_bindings(  );
	init_console_commands(  );
}
