#ifndef ENUMS_H
#define ENUMS_H

typedef enum
{
	ENGINE_C,
	RENDERER_C,
	GRAPHICS_C,
	AUDIO_C,
	GUI_C,
	INPUT_C,
}system_class_e;

typedef enum
{
	ENGI_PING,
	ENGI_EXIT
}engine_command_e;

typedef enum
{
	IMGUI_INITIALIZED,
}rend_command_e;

typedef enum
{
	GFIX_LOAD_SPRITE,
	GFIX_LOAD_MODEL,
}gfx_command_e;

typedef enum
{
	LOAD_IMGUI_DEMO,
	ASSIGN_WIN,
}gui_command_e;

#endif
