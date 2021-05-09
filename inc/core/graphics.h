#ifndef GRAPHICS_H
#define GRAPHICS_H

#include "system.h"
#include "renderer.h"

class graphics_c : public system_c
{
	protected:

	std::vector< sprite_t > sprites;
	std::vector< model_t > models;
	static std::vector< model_data_t > modelData;
	
	renderer_c renderer;

	void init_commands
		(  );

	void load_model
		( const std::string& modelPath, const std::string& texturePath );
	void load_sprite
		( const std::string& spritePath );
	
	public:

	void draw_frame
		(  );
	void sync_renderer
		(  );

	graphics_c
		(  );
	~graphics_c
		(  );
};

#endif
