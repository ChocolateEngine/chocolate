#include "render.h"


VkCommandPool   g_vk_command_pool_graphics;
VkCommandPool   g_vk_command_pool_transfer;

VkCommandBuffer g_vk_command_buffer_transfer;


void vk_command_pool_create_base( const char* name, u32 queue_family, VkCommandPool& pool )
{
	VkCommandPoolCreateInfo pool_info{ VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };
	pool_info.queueFamilyIndex = queue_family;
	pool_info.flags            = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

	vk_check( vkCreateCommandPool( g_vk_device, &pool_info, nullptr, &pool ), "Failed to create command pool" );

	vk_set_name( VK_OBJECT_TYPE_COMMAND_POOL, (u64)pool, name );
}


void vk_command_pool_create()
{
	vk_command_pool_create_base( "Graphics", g_vk_queue_family_graphics, g_vk_command_pool_graphics );
	vk_command_pool_create_base( "Transfer", g_vk_queue_family_transfer, g_vk_command_pool_transfer );

	// create one time command buffer for transfer queue
	VkCommandBufferAllocateInfo alloc_info{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
	alloc_info.commandPool        = g_vk_command_pool_transfer;
	alloc_info.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	alloc_info.commandBufferCount = 1;

	vk_check( vkAllocateCommandBuffers( g_vk_device, &alloc_info, &g_vk_command_buffer_transfer), "Failed to allocate command buffer!" );
}


void vk_command_pool_destroy()
{
	if ( g_vk_command_pool_graphics )
		vkDestroyCommandPool( g_vk_device, g_vk_command_pool_graphics, nullptr );

	if ( g_vk_command_pool_transfer )
		vkDestroyCommandPool( g_vk_device, g_vk_command_pool_transfer, nullptr );

	g_vk_command_pool_graphics = VK_NULL_HANDLE;
	g_vk_command_pool_transfer = VK_NULL_HANDLE;
}


void vk_command_pool_reset_graphics()
{
	vk_check( vkResetCommandPool( g_vk_device, g_vk_command_pool_graphics, 0 ), "Failed to reset graphics command pool!" );
}


VkCommandBuffer vk_cmd_transfer_begin()
{
	VkCommandBufferBeginInfo begin_info{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
	begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	vk_check( vkBeginCommandBuffer( g_vk_command_buffer_transfer, &begin_info ), "Failed to begin transfer command buffer!" );

	return g_vk_command_buffer_transfer;
}


void vk_cmd_transfer_end( VkCommandBuffer command_buffer )
{
	vk_check( vkEndCommandBuffer( command_buffer ), "Failed to end transfer command buffer!" );

	VkSubmitInfo submit_info{ VK_STRUCTURE_TYPE_SUBMIT_INFO };
	submit_info.commandBufferCount   = 1;
	submit_info.pCommandBuffers      = &command_buffer;

	vk_check( vkQueueSubmit( g_vk_queue_transfer, 1, &submit_info, VK_NULL_HANDLE ), "Failed to submit transfer command buffer!" );
	vk_check( vkQueueWaitIdle( g_vk_queue_transfer ), "Failed to wait for transfer queue!" );
	vk_check( vkResetCommandPool( g_vk_device, g_vk_command_pool_transfer, 0 ), "Failed to reset transfer command pool!" );
}


// IDEA: maybe later on for multi-threading, we can de-couple command buffers to windows
// and when we render a window, we just pick some secondary command buffers or something? idk yet
bool vk_command_buffers_create( r_window_data_t* window )
{
	VkCommandBufferAllocateInfo alloc_info{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
	alloc_info.commandPool        = g_vk_command_pool_graphics;
	alloc_info.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	alloc_info.commandBufferCount = window->swap_image_count;

	return !vk_check_e( vkAllocateCommandBuffers( g_vk_device, &alloc_info, window->command_buffers ), "Failed to allocate window graphics command buffers" );
}


void vk_command_buffers_destroy( r_window_data_t* window )
{
	if ( window->command_buffers )
	{
		vkFreeCommandBuffers( g_vk_device, g_vk_command_pool_graphics, window->swap_image_count, window->command_buffers );
	}
}

