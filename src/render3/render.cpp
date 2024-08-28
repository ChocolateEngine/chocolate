#include "render.h"


LOG_CHANNEL_REGISTER( Render );
LOG_CHANNEL_REGISTER( Vulkan );
LOG_CHANNEL_REGISTER( Validation );


ResourceList< r_window_data_t > g_windows;
ChHandle_t                      g_main_window;

SDL_Window**                    g_windows_sdl;
ImGuiContext**                  g_windows_imgui_contexts;


CONVAR_EX_BOOL( r_msaa_enabled, "r.msaa.enabled", false, CVARF_ARCHIVE );
CONVAR_EX_INT( r_msaa_samples, "r.msaa.samples", 4, CVARF_ARCHIVE );

