#pragma once

// 
// New Multi-Threaded Vulkan Renderer for Chocolate Engine
// Can also run in headless mode for server applications (though will we actually use that feature?)
// 


#include "core/core.h"
#include "igraphics_data.h"


CH_HANDLE_GEN_32( r_window_h );
CH_HANDLE_GEN_32( r_texture_h );
CH_HANDLE_GEN_32( r_mesh_h );
CH_HANDLE_GEN_32( r_mesh_render_h );


enum e_render_reset_flags
{
	e_render_reset_flags_none   = 0,
	e_render_reset_flags_resize = ( 1 << 0 ),  // A Window Resize Occurred
	e_render_reset_flags_msaa   = ( 1 << 1 ),  // MSAA was toggled, Recreate RenderPass and shaders (is this needed with dynamic rendering?)
};


// Function callback for app code for when renderer gets reset
typedef void ( *fn_render_on_reset_t )( r_window_h window, e_render_reset_flags flags );


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


struct texture_load_info_t
{
};


struct texture_data_t
{
	GraphicsFmt format;
	glm::vec3   size;
	u32         mip_levels;
	u32         array_layers;
	u32         samples;
	u32         data_size;
	u8*         data;
};


struct IRender3 : public ISystem
{
	// Required to be called before Init(), blame vulkan
	// TODO: If this is not called, we will be in headless mode
	virtual void        set_main_surface( SDL_Window* window, void* native_window = nullptr )      = 0;

	// get gpu name
	virtual const char* get_device_name()                                                          = 0;

	// --------------------------------------------------------------------------------------------
	// Windows
	// Each window should have their own ImGui Context
	// And should be set to that whenever calling any function that includes a window handle
	// --------------------------------------------------------------------------------------------

	// the native window would be the HWND on windows, also required on windows
	virtual r_window_h  window_create( SDL_Window* window, void* native_window = nullptr )         = 0;
	virtual void        window_free( r_window_h window )                                           = 0;
	virtual void        window_set_reset_callback( r_window_h window, fn_render_on_reset_t func )  = 0;
	virtual glm::uvec2  window_surface_size( r_window_h window )                                   = 0;

	// --------------------------------------------------------------------------------------------
	// Rendering
	// --------------------------------------------------------------------------------------------

	virtual void        new_frame( r_window_h window_handle )                                      = 0;
	virtual void        reset( r_window_h window )                                                 = 0;
	virtual void        present( r_window_h window )                                               = 0;

	// notes for rendering ideas
	//
	// - make a set_render_target function that takes in a render target and viewport (or should i make a set_viewport function?)
	// - passing in nullptr or CH_INVALID_HANDLE as the render target in set_render_target would use the backbuffer of the active window
	// - and then a separate render/draw command for renderables
	// - then you could easily add in other stuff for rendering later, like debug drawing, particles, screen-space effects, or taking screenshots
	// - shadowmapping would be handled internally
	// - or maybe you could have some command builder thing? idk
	// - this won't render stuff in these call, but just prepare the rendering, so the renderer can multi-thread it
	// - maybe have a start_render and end_render function? that sounds very immediate mode lol, but that's not a bad thing here since it's not true immediate mode
	// - this simple fake immediate mode rendering would also work nicely for split-screen rendering
	//
	// - other things to think about:
	//   - occlusion culling can be done internally
	//   - skybox and viewmodels?
	//     they use a depth hack, skybox is rendered at the end, and viewmodel at the start? maybe even an fov hack for viewmodels
	//
	//   - reflections? cubemaps? screen-space reflections?
	//   - AO?
	//   - tonemapping? bloom? color correction?
	//   - what about water rendering? like when you're underwater, or looking through the surface of the water
	//
	//   - what about in the physics dll for debug rendering? how do you know what views to draw that to?
	//     actually we might, because any debug draw commands go to the currently active physics environment, and we would know what views have the physics objects visible in the app code
	//
	//   - how would debug drawing work? you want to be able to do debug drawing to specific views, but also to the main window
	// 
	//   - what about material selction under cursor?
	//   - what about renderables with vis testing disabled?
	//   - what about renderables with shadow casting or receiving disabled?
	// 
	//   - what if you have a third person player model, and you don't want that rendered in the main view, but want a shadow casted on the world from it?
	//     maybe have a cmd_set_shadow_renderables() function?
	// 

	/*
	* // idea for basic render loop
	* 
	* // start of frame
	* render->new_frame();
	* 
	* 
	* // we have a camera in this scene rendering another view on a monitor/material, maybe like gman on a tv or something
	* // this also applies for screenshots, or just rendering to a custom texture
	* monitor_c = render->cmd_render_start();
	* render->cmd_set_render_target( monitor_c, monitor_target );
	* render->cmd_set_viewport( monitor_c, monitor_viewport );
	* 
	* render->cmd_clear( monitor_c, clear_color );
	* 
	* // all this stuff would be culled already for this viewport somehow
	* // or we could just do culling internally, 
	* // since the method of occlusion culling im gonna try using can be done entirely in the renderer
	* render->cmd_draw_renderables( monitor_c, renderables, renderable_count );
	* render->cmd_draw_lights( monitor_c, lights, light_count );
	* render->cmd_draw_particles( monitor_c, particles, particle_count );
	* 
	* render->cmd_render_finish( monitor_c );
	* 
	* // now draw the main scene
	* c = render->cmd_render_start( backbuffer, main_viewport );
	* 
	* render->cmd_clear( c, clear_color ); // if we have a skybox, we don't need to clear
	* 
	* // viewmodel rendering?
	* render->cmd_set_viewport( c, viewmodel_view );
	* render->cmd_enable_vis( c, false );
	* render->cmd_enable_shadow_casting( c, false );
	* render->cmd_draw_renderables( c, &viewmodel, 1 );
	* render->cmd_enable_vis( c, true );
	* render->cmd_enable_shadow_casting( c, true );
	* 
	* // do we need to render lights, particles, or other stuff here?
	* render->cmd_set_viewport( c, main_view );
	* 
	* render->cmd_draw_renderables( c, renderables, renderable_count );
	* render->cmd_draw_debug( c );
	* render->cmd_draw_lights( c, lights, light_count );
	* render->cmd_draw_particles( c, particles, particle_count );
	* render->cmd_apply_screen_space_effects( c ); // ?? i have no idea how screenspace stuff works right now lol
	* render->cmd_draw_imgui( c, ImGui::GetDrawData() );
	* 
	* render->cmd_render_finish( c );
	* 
	* // maybe have a callback for when this is done so the game code can do something with it? 
	* render->take_screenshot( backbuffer, "screenshot.png" );
	* 
	* 
	* // end of frame
	* render->cmd_draw_target_to_window( target, window, params );  // only needed if using a custom rendertarget, and not backbuffer
	* render->present( window );
	*
	* // other cmd functions
	* render->cmd_enable_vis( c, false );
	* render->cmd_draw_renderables( c, renderables, renderable_count );
	* render->cmd_enable_vis( c, true );
	* 
	* render->cmd_enable_shadow_casting( c, false );
	* render->cmd_draw_renderables( c, renderables, renderable_count );
	* render->cmd_enable_shadow_casting( c, true );
	* 
	* render->cmd_enable_shadow_receiving( c, bool );
	* 
	*/

	// --------------------------------------------------------------------------------------------
	// Views
	// --------------------------------------------------------------------------------------------


	// --------------------------------------------------------------------------------------------
	// Textures
	// --------------------------------------------------------------------------------------------

	// Loads a texture from disk and uploads it to the GPU
//	virtual ch_texture_h* texture_load( const char** paths, const texture_load_info_t* create_infos, size_t count = 1 )                = 0;
//	virtual ch_texture_h* texture_load( const char** paths, s64* pathLens, const texture_load_info_t* create_infos, size_t count = 1 ) = 0;
//	//virtual ch_texture_h     texture_load( ch_string* paths, const texture_load_info_t* create_infos, size_t count = 1 )   = 0;
//	virtual ch_texture_h  texture_load( const char* path, s64 pathLen, const texture_load_info_t& create_info )                        = 0;
//
//	virtual void          texture_free( ch_texture_h* texture, size_t count = 1 )                                                      = 0;
	//	virtual texture_data_t       texture_get_data( ch_texture_h texture )                                                                      = 0;

	// --------------------------------------------------------------------------------------------
	// Materials
	// --------------------------------------------------------------------------------------------


	// --------------------------------------------------------------------------------------------
	// Models
	// --------------------------------------------------------------------------------------------


	// --------------------------------------------------------------------------------------------
	// Renderables
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

	// --------------------------------------------------------------------------------------------
	// Test Rendering 
	// --------------------------------------------------------------------------------------------

	virtual bool            test_init()                                                                               = 0;
	virtual void            test_shutdown()                                                                           = 0;

	virtual void            test_update( float frame_time, r_window_h window, glm::mat4 view, glm::mat4 projection )  = 0;

	virtual r_mesh_h        mesh_upload( ch_model_h handle )                                                          = 0;
	virtual r_mesh_render_h mesh_render_create( r_mesh_h handle )                                                     = 0;
	virtual void            mesh_render_set_matrix( r_mesh_render_h handle, const glm::mat4& matrix )                 = 0;
	//	virtual void        test_mesh_free( ch_handle_t mesh )                                                        = 0;
};


#define CH_RENDER3     "chocolate_render3"
#define CH_RENDER3_VER 6

