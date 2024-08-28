#pragma once

// 
// New Multi-Threaded Vulkan Renderer for Chocolate Engine
// Can also run in headless mode for server applications (though will we actually use that feature?)
// 


#include "core/core.h"
#include "system.h"
// #include "igraphics_data.h"


struct SDL_Window;


struct texture_upload_info_t
{
};


struct renderable_t
{
};


struct view_t
{

};


struct IRender3 : public ISystem
{
	// Required to be called before Init(), blame vulkan
	// TODO: If this is not called, we will be in headless mode
	virtual void       set_main_surface( SDL_Window* window, void* native_window = nullptr ) = 0;

	// --------------------------------------------------------------------------------------------
	// Windows
	// EACH ONE SHOULD HAVE THEIR OWN IMGUI CONTEXT !!!!
	// --------------------------------------------------------------------------------------------

	// the native window would be the HWND on windows, also required on windows
	virtual ChHandle_t window_create( SDL_Window* window, void* native_window = nullptr )    = 0;
	virtual void       window_free( ChHandle_t window )                                      = 0;
	virtual glm::uvec2 window_surface_size( ChHandle_t window )                              = 0;

	// --------------------------------------------------------------------------------------------
	// Rendering
	// --------------------------------------------------------------------------------------------

	virtual void       new_frame()                                                           = 0;
	virtual void       reset( ChHandle_t window )                                            = 0;
	virtual void       present( ChHandle_t window )                                          = 0;

	// --------------------------------------------------------------------------------------------
	// Views
	// --------------------------------------------------------------------------------------------

	// --------------------------------------------------------------------------------------------
	// Textures
	// --------------------------------------------------------------------------------------------

	// Uploads a texture to the GPU
	//	virtual ChHandle_t* texture_upload( ChHandle_t* textures, texture_upload_info_t* upload_infos, u64 count = 1 ) = 0;
	//	virtual void        texture_free( ChHandle_t* textures, u64 count = 1 )                                        = 0;

	// --------------------------------------------------------------------------------------------
	// Materials
	// --------------------------------------------------------------------------------------------


	// --------------------------------------------------------------------------------------------
	// Models
	// --------------------------------------------------------------------------------------------


	// --------------------------------------------------------------------------------------------
	// Shader System
	// --------------------------------------------------------------------------------------------


	// --------------------------------------------------------------------------------------------
	// Debug Drawing System
	// --------------------------------------------------------------------------------------------

#if 0
	virtual void        draw_debug_enable( bool enabled )                                                          = 0;

	// Sets thickness of lines for future commands
	virtual void        draw_push_thickness( float thickness )                                                     = 0;
	virtual void        draw_pop_thickness()                                                                       = 0;

	virtual void        draw_line( const glm::vec3& start, const glm::vec3& end, const glm::vec3& color )          = 0;
	virtual void        draw_line( const glm::vec3& start, const glm::vec3& end, const glm::vec4& color )          = 0;

	virtual void        draw_axis( const glm::vec3 pos, const glm::vec3& ang, const glm::vec3& scale )             = 0;
	virtual void        draw_axis( const glm::mat4& transform, const glm::vec3& scale )                            = 0;

	virtual void        draw_bounding_box( const glm::vec3& min, const glm::vec3& max, const glm::vec4& color )    = 0;
	//virtual void       draw_box( const glm::vec3& pos, const glm::vec3& ang, const glm::vec3& scale, const glm::vec3& color )   = 0;
	//virtual void       draw_box( const glm::mat4& transform, const glm::vec3& color )                                           = 0;

	virtual void        draw_projection_view( const glm::mat4& proj_view, const glm::vec4& color )                 = 0;
	virtual void        draw_frustum( const frustum_t& frustum, const glm::vec4& color )                           = 0;
#endif

};


#define CH_RENDER3     "chocolate_render3"
#define CH_RENDER3_VER 1

