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


vk_buffer_t* vk_buffer_create( VkDeviceSize size, VkBufferUsageFlags usage, VmaMemoryUsage mem_usage )
{
	// create a buffer
	VkBufferCreateInfo buffer_info{ VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
	buffer_info.size        = size;
	buffer_info.usage       = usage;
	buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	VmaAllocationCreateInfo alloc_info{};
	alloc_info.usage       = mem_usage;
	alloc_info.flags       = VMA_ALLOCATION_CREATE_MAPPED_BIT;

	vk_buffer_t* vk_buffer = ch_calloc< vk_buffer_t >( 1 );

	// allocate the buffer
	vk_check( vmaCreateBuffer( g_vma, &buffer_info, &alloc_info, &vk_buffer->buffer, &vk_buffer->alloc, &vk_buffer->info ) );

	// add to the list of buffers
	g_vk_buffers.push_back( vk_buffer );

	return vk_buffer;
}


vk_buffer_t* vk_buffer_create( const char* name, VkDeviceSize size, VkBufferUsageFlags usage, VmaMemoryUsage mem_bits )
{
	vk_buffer_t* buffer = vk_buffer_create( size, usage, mem_bits );

	if ( !buffer )
		return nullptr;

	vk_set_name( VK_OBJECT_TYPE_BUFFER, (u64)buffer->buffer, name );
	vk_set_name( VK_OBJECT_TYPE_DEVICE_MEMORY, (u64)buffer->info.deviceMemory, name );

	return buffer;
}


void vk_buffer_destroy( vk_buffer_t* buffer )
{
	if ( !buffer )
		return;

	// remove it from the list
	g_vk_buffers.erase( buffer );

	vmaDestroyBuffer( g_vma, buffer->buffer, buffer->alloc );
	ch_free( buffer );
}


// not sure what file to put this in right now
gpu_mesh_buffers_t vk_mesh_upload( u32* indices, u32 index_count, gpu_vertex_t* vertices, u32 vertex_count )
{
	gpu_mesh_buffers_t mesh_buffers{};

	size_t             vertex_size = vertex_count * sizeof( gpu_vertex_t );
	size_t             index_size  = index_count * sizeof( u32 );

	// Create Vertex Buffer
	// Using VK_BUFFER_USAGE_TRANSFER_DST_BIT as memory will be copied to it
	mesh_buffers.vertex            = vk_buffer_create( "Vertex Buffer", vertex_size, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT, VMA_MEMORY_USAGE_GPU_ONLY );

	// Get Vertex Buffer Address
	VkBufferDeviceAddressInfo address_info{ VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO };
	address_info.buffer         = mesh_buffers.vertex->buffer;
	mesh_buffers.vertex_address = vkGetBufferDeviceAddress( g_vk_device, &address_info );

	// Create Index Buffer
	mesh_buffers.index          = vk_buffer_create( "Index Buffer", index_size, VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VMA_MEMORY_USAGE_GPU_ONLY );

	// Create a staging buffer to upload vertex data to
	// TODO: try CPU_TO_GPU memory usage?
	vk_buffer_t* staging        = vk_buffer_create( "Mesh Staging Buffer", vertex_size + index_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY );

	void*        staging_data = nullptr;
	vk_check( vmaMapMemory( g_vma, staging->alloc, &staging_data ) );

	// copy vertex buffer
	memcpy( staging_data, vertices, vertex_size );

	// copy index buffer
	memcpy( (char*)staging_data + vertex_size, indices, index_size );

	staging_data = nullptr;
	vmaUnmapMemory( g_vma, staging->alloc );

	// TODO: queue this, or do it in the background
	VkCommandBuffer c = vk_cmd_transfer_begin();

	VkBufferCopy    vertex_copy{};
	vertex_copy.dstOffset = 0;
	vertex_copy.srcOffset = 0;
	vertex_copy.size      = vertex_size;

	vkCmdCopyBuffer( c, staging->buffer, mesh_buffers.vertex->buffer, 1, &vertex_copy );

	VkBufferCopy index_copy{};
	index_copy.dstOffset = 0;
	index_copy.srcOffset = vertex_size;
	index_copy.size      = index_size;

	vkCmdCopyBuffer( c, staging->buffer, mesh_buffers.index->buffer, 1, &index_copy );

	vk_cmd_transfer_end( c );

	vk_buffer_destroy( staging );

	return mesh_buffers;
}


// not sure what file to put this in right now
gpu_mesh_buffers_t vk_mesh_upload( gpu_vertex_t* vertices, u32 vertex_count )
{
	gpu_mesh_buffers_t mesh_buffers{};

	size_t             vertex_size = vertex_count * sizeof( gpu_vertex_t );

	// Create Vertex Buffer
	// Using VK_BUFFER_USAGE_TRANSFER_DST_BIT as memory will be copied to it
	mesh_buffers.vertex            = vk_buffer_create( "Vertex Buffer", vertex_size, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT, VMA_MEMORY_USAGE_GPU_ONLY );

	// Get Vertex Buffer Address
	VkBufferDeviceAddressInfo address_info{ VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO };
	address_info.buffer         = mesh_buffers.vertex->buffer;
	mesh_buffers.vertex_address = vkGetBufferDeviceAddress( g_vk_device, &address_info );

	// Create a staging buffer to upload vertex data to
	// TODO: try CPU_TO_GPU memory usage?
	vk_buffer_t* staging        = vk_buffer_create( "Mesh Staging Buffer", vertex_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY );

	void*        staging_data = nullptr;
	vk_check( vmaMapMemory( g_vma, staging->alloc, &staging_data ) );

	// copy vertex buffer
	memcpy( staging_data, vertices, vertex_size );

	staging_data = nullptr;
	vmaUnmapMemory( g_vma, staging->alloc );

	// TODO: queue this, or do it in the background
	VkCommandBuffer c = vk_cmd_transfer_begin();

	VkBufferCopy    vertex_copy{};
	vertex_copy.dstOffset = 0;
	vertex_copy.srcOffset = 0;
	vertex_copy.size      = vertex_size;

	vkCmdCopyBuffer( c, staging->buffer, mesh_buffers.vertex->buffer, 1, &vertex_copy );

	vk_cmd_transfer_end( c );

	vk_buffer_destroy( staging );

	return mesh_buffers;
}


void vk_mesh_free( gpu_mesh_buffers_t& mesh_buffers )
{
	vk_buffer_destroy( mesh_buffers.index );
	vk_buffer_destroy( mesh_buffers.vertex );
}


#if 0

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

#endif

