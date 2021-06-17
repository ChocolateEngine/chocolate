#ifndef GRAPHICS_H
#define GRAPHICS_H

#include "../shared/system.h"
#include "renderer/renderer.h"

typedef struct
{
	uint32_t posX, posY, resX, resY;
	sprite_data_t spriteData;
}sprite_t;

typedef struct
{
	uint32_t posX, posY, posZ;
	model_data_t modelData;
}model_t;

class graphics_c : public system_c
{
	protected:

	std::vector< sprite_t* > sprites;
	std::vector< model_t* > models;
	static std::vector< model_data_t > modelData;
	static std::vector< sprite_data_t > spriteData;
	
	renderer_c renderer;

	void init_commands
		(  );

	void load_model
		( const std::string& modelPath, const std::string& texturePath, model_t* model = NULL );
	void load_sprite
		( const std::string& spritePath, sprite_t* sprite = NULL );
	
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
