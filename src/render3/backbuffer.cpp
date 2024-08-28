#include "render.h"


static bool vk_create_backbuffer_depth( r_window_data_t* window )
{
	return false;
}


bool vk_create_backbuffer( r_window_data_t* window )
{
	// create a new depth texture
	// the color texture will be swap image view 0, no VkImage creation needed
	// 
	// if msaa is enabled, we need a new color texture with msaa samples on it
	// then the resolve texture will be a swap image view instead, no VkImage creation needed
	// 
	// we also need 2 framebuffers, one for depth, one for color
	// 

	if ( r_msaa_enabled )
	{
		window->backbuffer.images = ch_malloc< backbuffer_image_t >( window->swap_image_count );
	}

	return false;
}

