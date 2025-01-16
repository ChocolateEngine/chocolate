#define VMA_IMPLEMENTATION 1

#include "render.h"

LOG_CHANNEL_REGISTER( Render, ELogColor_Cyan );
LOG_CHANNEL_REGISTER( Vulkan );
LOG_CHANNEL_REGISTER( Validation );


ch_handle_list_32< r_window_h, r_window_data_t, 1 > g_windows;
r_window_h                                          g_main_window;

SDL_Window**                                        g_windows_sdl;
ImGuiContext**                                      g_windows_imgui_contexts;

delete_queue_t                                      g_vk_delete_queue;

handle_list_t< vk_image_t >                         g_vk_images;
