#define VMA_IMPLEMENTATION 1

#include "render.h"

LOG_CHANNEL_REGISTER( Render, ELogColor_Cyan );
LOG_CHANNEL_REGISTER( Vulkan );
LOG_CHANNEL_REGISTER( Validation );


ResourceList< r_window_data_t > g_windows;
ch_handle_t                      g_main_window;

SDL_Window**                    g_windows_sdl;
ImGuiContext**                  g_windows_imgui_contexts;

delete_queue_t                  g_vk_delete_queue;

handle_list_t< vk_image_t >     g_vk_images;


CONVAR_BOOL_NAME( r_msaa_enabled, "r.msaa.enabled", false, CVARF_ARCHIVE );
CONVAR_INT_NAME( r_msaa_samples, "r.msaa.samples", 4, CVARF_ARCHIVE );

