#ifndef ENUMS_H
#define ENUMS_H

typedef enum
{
	ENGINE_C,
	RENDERER_C,
	GRAPHICS_C,
}system_class_e;

typedef enum
{
	ENGI_PING,
	ENGI_EXIT
}engine_command_e;

typedef enum
{
	REND_UP,
	REND_RIGHT,
	REND_DOWN,
	REND_LEFT
}rend_command_e;

typedef enum
{
	GFIX_LOAD_SPRITE,
	GFIX_LOAD_MODEL,
}gfx_command_e;

#endif
