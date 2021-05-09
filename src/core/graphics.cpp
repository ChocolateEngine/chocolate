#include "../../inc/core/graphics.h"

std::vector< model_data_t > graphics_c::modelData;

void graphics_c::init_commands
	(  )
{
	msg_s msg;
	msg.type = GRAPHICS_C;

	msg.msg = GFIX_LOAD_MODEL;
	msg.func = [ & ]( void** args, int argsLen )
		{
			if ( argsLen < 2 )
			{
				printf( "Invalid parameters for GFIX_LOAD_MODEL\n" );
				return;
			}
			load_model( ( char* )args[ 0 ], ( char* )args[ 1 ] );
		};
	engineCommands.push_back( msg );
}

void graphics_c::load_model
	( const std::string& modelPath, const std::string& texturePath )
{
	model_t model{  };
	renderer.init_model( model.modelData, modelPath, texturePath );
	models.push_back( model );
}

void graphics_c::load_sprite
	( const std::string& spritePath )
{
	
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
	
	renderer.models = &modelData;
	renderer.init_vulkan(  );
	
	//load_model( "materials/models/protogen_wip_5_plus_protodal.obj", "materials/textures/blank_mat.png"  );
	//load_model( "materials/models/protogen_wip_4.obj", "materials/textures/blank_mat.png" );
}

graphics_c::~graphics_c
	(  )
{
	
}
