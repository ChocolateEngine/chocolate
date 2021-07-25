#include "../../inc/core/graphics.h"

#include <cstdlib>
#include <ctime>

std::vector< model_data_t* > graphics_c::modelData;
std::vector< sprite_data_t* > graphics_c::spriteData;

void graphics_c::init_commands
	(  )
{
	msg_s msg;
	msg.type = GRAPHICS_C;

	msg.msg = GFIX_LOAD_SPRITE;
	msg.func = [ & ]( std::vector< std::any > args )
		{
			if ( args.size(  ) < 1 )
			{
				printf( "Invalid parameters for GFIX_LOAD_SPRITE\n" );
				return;
			}
			if ( args.size(  ) == 1 )
			{
				load_sprite( std::any_cast< const char* >( args[ 0 ] ) );
				return;
			}
			load_sprite( std::any_cast< const char* >( args[ 0 ] ), std::any_cast< sprite_t* >( args[ 1 ] ) );
		};
	engineCommands.push_back( msg );
	msg.msg = GFIX_LOAD_MODEL;
	msg.func = [ & ]( std::vector< std::any > args )
		{
			if ( args.size(  ) < 2 )
			{
				printf( "Invalid parameters for GFIX_LOAD_MODEL\n" );
				return;
			}
			load_model( std::any_cast< const char * >( args[ 0 ] ), std::any_cast< const std::string& >( args[ 1 ] ) );
		};
	engineCommands.push_back( msg );
}

void graphics_c::load_model
	( const std::string& modelPath, const std::string& texturePath, model_t* model )
{
	if ( model == NULL )
	{
		model = new model_t;
	}
        srand( ( unsigned int )time( 0 ) );
	float r = ( float )( rand(  ) / ( float )( RAND_MAX / 10.0f ) );
	model->modelData.posX = r;
	model->modelData.posY = r;
	model->modelData.posZ = r;
	renderer.init_model( model->modelData, modelPath, texturePath );
	
	models.push_back( model );
}

void graphics_c::load_sprite
	( const std::string& spritePath, sprite_t* sprite )
{
	if ( sprite == NULL )
	{
		sprite = new sprite_t;
	}
	srand( ( unsigned int )time( 0 ) );
	float r = ( float )( rand(  ) / ( float )( RAND_MAX / 1.0f ) );
        sprite->spriteData.posX = r;
	sprite->spriteData.posY = r;
	renderer.init_sprite( sprite->spriteData, spritePath );
	
	sprites.push_back( sprite );
}

void graphics_c::draw_frame
	(  )
{
	renderer.draw_frame(  );
}

void graphics_c::sync_renderer
	(  )
{
	renderer.msgs = msgs;
	renderer.console = console;
}

graphics_c::graphics_c
	(  )
{
	systemType = GRAPHICS_C;
	init_commands(  );
	add_func( [ & ](  ){ renderer.update(  ); } );
	add_func( [ & ](  ){ draw_frame(  ); } );
	
	renderer.models = &modelData;
	renderer.sprites = &spriteData;
	renderer.init_vulkan(  );

	//load_model( "materials/models/protogen_wip_5_plus_protodal.obj", "materials/textures/red_mat.png"  );
	load_model( "materials/models/protogen_wip_9.obj", "materials/textures/blue_mat.png" );
	//load_sprite( "materials/textures/hilde_sprite_upscale.png" );
	//load_sprite( "materials/textures/blue_mat.png" );
}

void graphics_c::init_subsystems
	(  )
{
	sync_renderer(  );
	renderer.send_messages(  );
}

graphics_c::~graphics_c
	(  )
{
	for ( const auto& model : models )
	{
		delete model;
	}
	for ( const auto& sprite : sprites )
	{
		delete sprite;
	}
}
