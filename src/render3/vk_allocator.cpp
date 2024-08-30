// =============================================================================
// Basic allocator for Vulkan resources
// 
// Not sure if this will be here forever,
// I may move to Vulkan Memory Allocator again if I can get it working on the GTX 750 Ti
// It wasn't working on that card for some reason
// =============================================================================

#include "render.h"


// this sucks, double pointer
ChVector< vk_buffer_t* > g_vk_buffers;


u32 vk_get_memory_type( u32 type_filter, VkMemoryPropertyFlags properties )
{
	for ( u32 i = 0; i < g_vk_device_memory_properties.memoryTypeCount; ++i )
	{
		if ( ( type_filter & ( 1 << i ) ) && ( g_vk_device_memory_properties.memoryTypes[ i ].propertyFlags & properties ) == properties )
		{
			return i;
		}
	}

	return UINT32_MAX;
}


vk_buffer_t* vk_buffer_create( VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags mem_bits )
{
	// create a buffer
	VkBufferCreateInfo buffer_info{ VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
	buffer_info.size        = size;
	buffer_info.usage       = usage;
	buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	VkBuffer buffer;
	vk_check( vkCreateBuffer( g_vk_device, &buffer_info, nullptr, &buffer ), "Failed to create buffer" );

	// allocate memory for the buffer
	VkMemoryRequirements mem_require;
	vkGetBufferMemoryRequirements( g_vk_device, buffer, &mem_require );

	VkMemoryAllocateInfo alloc_info{ VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
	alloc_info.allocationSize  = mem_require.size;
	alloc_info.memoryTypeIndex = vk_get_memory_type( mem_require.memoryTypeBits, mem_bits );

	if ( alloc_info.memoryTypeIndex == UINT32_MAX )
	{
		Log_Error( "Failed to find suitable memory type for buffer" );
		return nullptr;
	}

	// TODO: check maxMemoryAllocationCount and maxMemoryAllocationSize
	// also improve this allocator, allocate in chunks
	VkDeviceMemory memory;
	vk_check( vkAllocateMemory( g_vk_device, &alloc_info, nullptr, &memory ), "Failed to allocate buffer memory" );

	// bind the buffer to the device memory
	vk_check( vkBindBufferMemory( g_vk_device, buffer, memory, 0 ), "Failed to bind buffer" );

	vk_buffer_t* vk_buffer = ch_malloc< vk_buffer_t >( 1 );
	vk_buffer->buffer      = buffer;
	vk_buffer->memory      = memory;
	vk_buffer->size        = size;

	// add to the list of buffers
	g_vk_buffers.push_back( vk_buffer );

	return vk_buffer;
}


vk_buffer_t* vk_buffer_create( const char* name, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags mem_bits )
{
	vk_buffer_t* buffer = vk_buffer_create( size, usage, mem_bits );

	if ( !buffer )
		return nullptr;

	vk_set_name( VK_OBJECT_TYPE_BUFFER, (u64)buffer->buffer, name );
	vk_set_name( VK_OBJECT_TYPE_DEVICE_MEMORY, (u64)buffer->memory, name );

	return buffer;
}


void vk_buffer_destroy( vk_buffer_t* buffer )
{
	if ( !buffer )
		return;

	// remove it from the list
	g_vk_buffers.erase( buffer );

	if ( buffer->buffer )
	{
		vkDestroyBuffer( g_vk_device, buffer->buffer, nullptr );
		buffer->buffer = VK_NULL_HANDLE;
	}

	if ( buffer->memory )
	{
		vkFreeMemory( g_vk_device, buffer->memory, nullptr );
		buffer->memory = VK_NULL_HANDLE;
	}

	ch_free( buffer );
}


// returns how many bytes written to the buffer
VkDeviceSize vk_buffer_write( vk_buffer_t* buffer, void* data, VkDeviceSize size, VkDeviceSize offset )
{
	// maybe try mapping buffer regions
	// or have vk_buffer_map and vk_buffer_unmap functions?
	// also look into vkCmdUpdateBuffer

	size              = std::min( size, buffer->size - offset );

	void* mapped_data = nullptr;

	if ( vk_check_e( vkMapMemory( g_vk_device, buffer->memory, offset, size, 0, &mapped_data ) ) )
	{
		Log_Error( "Failed to map buffer memory" );
		return 0;
	}

	memcpy( mapped_data, data, size );
	vkUnmapMemory( g_vk_device, buffer->memory );

	return size;
}


// returns how many bytes read from the buffer
VkDeviceSize vk_buffer_read( vk_buffer_t* buffer, void* data, VkDeviceSize size, VkDeviceSize offset )
{
	size = std::min( size, buffer->size - offset );

	void* mapped_data = nullptr;
	
	if ( vk_check_e( vkMapMemory( g_vk_device, buffer->memory, offset, size, 0, &mapped_data ) ) )
	{
		Log_Error( "Failed to map buffer memory" );
		return 0;
	}

	memcpy( data, mapped_data, size );
	vkUnmapMemory( g_vk_device, buffer->memory );

	return size;
}


// copies data from one buffer to another in regions
bool vk_buffer_copy( vk_buffer_t* src, vk_buffer_t* dst, VkDeviceSize size, VkDeviceSize offset )
{
	VkCommandBuffer c = vk_cmd_transfer_begin();

	if ( offset > src->size || offset > dst->size )
	{
		Log_Error( "Can't copy VkBuffer, offset is greater than buffer size" );
		return false;
	}

	VkBufferCopy region{};

	if ( size == 0 )
	{
		region.size = std::min( src->size, dst->size );
	}
	else
	{
		region.size = size;
	}

	region.srcOffset = offset;
	region.dstOffset = offset;

	vkCmdCopyBuffer( c, src->buffer, dst->buffer, 1, &region );

	vk_cmd_transfer_end( c );
	return true;
}


// copies data from one buffer to another in regions
bool vk_buffer_copy_region( vk_buffer_t* src, vk_buffer_t* dst, VkBufferCopy* regions, u32 region_count )
{
	VkCommandBuffer c = vk_cmd_transfer_begin();
	vkCmdCopyBuffer( c, src->buffer, dst->buffer, region_count, regions );
	vk_cmd_transfer_end( c );
	return true;
}

