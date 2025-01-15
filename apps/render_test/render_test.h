#pragma once

#include "iinput.h"
#include "igui.h"
#include "irender3.h"
#include "igraphics_data.h"

#include "types/transform.h"


extern IGuiSystem*    gui;
extern IRender3*      render;
extern IGraphicsData* graphics_data;
extern IInputSystem*  input;

extern SDL_Window*    g_window;
extern void*          g_window_native;
extern ch_handle_t    g_graphics_window;

extern glm::mat4      g_view;
extern glm::mat4      g_projection;


bool                  load_scene();
void                  handle_inputs( float frame_time );
void                  update_viewport();

