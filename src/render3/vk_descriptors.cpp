#include "render.h"


//u32         VK_DESCRIPTOR_TYPE_COUNT = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT + 1;
u32 VK_DESCRIPTOR_TYPE_COUNT = 11;

struct DescriptorPoolTypeStats_t
{
	u32 aAllocated = 0;
	u32 aUsed      = 0;
};


struct DescriptorPoolStats_t
{
	u32                       aSetsAllocated = 0;
	u32                       aSetsUsed      = 0;

	DescriptorPoolTypeStats_t aTypes[ 11 ];
};


enum EDescriptorPoolSize
{
	EDescriptorPoolSize_Storage               = 32767,
	EDescriptorPoolSize_Uniform               = 128,
	EDescriptorPoolSize_CombinedImageSamplers = 4096,
	EDescriptorPoolSize_SampledImages         = 32767,
};


static u32                   gDescriptorSetCount = 512;

static VkDescriptorPool      gVkDescriptorPool;
static DescriptorPoolStats_t gDescriptorPoolStats;


VkDescriptorPool             g_vk_imgui_desc_pool        = VK_NULL_HANDLE;
VkDescriptorSetLayout        g_vk_imgui_desc_layout      = VK_NULL_HANDLE;

VkDescriptorPool             g_vk_desc_pool              = VK_NULL_HANDLE;
VkDescriptorSetLayout        g_vk_desc_draw_image_layout = VK_NULL_HANDLE;


void                         vk_descriptor_pool_create()
{
	VkDescriptorPoolSize aPoolSizes[] = {
		{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, EDescriptorPoolSize_CombinedImageSamplers },
		{ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, EDescriptorPoolSize_SampledImages },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, EDescriptorPoolSize_Uniform },
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, EDescriptorPoolSize_Storage },
	};

	VkDescriptorPoolCreateInfo aDescriptorPoolInfo{ VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO };
	aDescriptorPoolInfo.poolSizeCount = CH_ARR_SIZE( aPoolSizes );
	aDescriptorPoolInfo.pPoolSizes    = aPoolSizes;
	aDescriptorPoolInfo.maxSets       = gDescriptorSetCount;  // this is incorrect lol, this should be the total number of sets that can be allocated from this pool, all types combined

	// Allows you to update descriptors after they have been bound in a command buffer
	aDescriptorPoolInfo.flags         = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT | VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT;

	vk_check( vkCreateDescriptorPool( g_vk_device, &aDescriptorPoolInfo, nullptr, &gVkDescriptorPool ), "Failed to create descriptor pool!" );

	gDescriptorPoolStats.aSetsAllocated                                                 = gDescriptorSetCount;
	gDescriptorPoolStats.aTypes[ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER ].aAllocated = EDescriptorPoolSize_CombinedImageSamplers;
	gDescriptorPoolStats.aTypes[ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE ].aAllocated          = EDescriptorPoolSize_SampledImages;
	gDescriptorPoolStats.aTypes[ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER ].aAllocated         = EDescriptorPoolSize_Uniform;
	gDescriptorPoolStats.aTypes[ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER ].aAllocated         = EDescriptorPoolSize_Storage;

	vk_set_name( VK_OBJECT_TYPE_DESCRIPTOR_POOL, (u64)gVkDescriptorPool, "Global Descriptor Pool" );
}


VkDescriptorPool vk_descriptor_pool_create( const char* name, u32 max_sets, vk_desc_pool_size_ratio_t* pool_sizes, u32 pool_size_count )
{
	VkDescriptorPoolSize* pool_sizes_vk = ch_malloc< VkDescriptorPoolSize >( pool_size_count );

	for ( u32 i = 0; i < pool_size_count; i++ )
	{
		pool_sizes_vk[ i ] = { pool_sizes[ i ].type, u32( pool_sizes[ i ].ratio * max_sets ) };
	}

	VkDescriptorPoolCreateInfo pool_info = { .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO };
	pool_info.flags                      = 0;
	pool_info.maxSets                    = max_sets;
	pool_info.poolSizeCount              = pool_size_count;
	pool_info.pPoolSizes                 = pool_sizes_vk;

	VkDescriptorPool pool;
	if ( vk_check_e( vkCreateDescriptorPool( g_vk_device, &pool_info, nullptr, &pool ), "Failed to create descriptor pool" ) )
	{
		ch_free( pool_sizes_vk );
		return VK_NULL_HANDLE;
	}

	vk_set_name( VK_OBJECT_TYPE_DESCRIPTOR_POOL, (u64)pool, name );

	ch_free( pool_sizes_vk );

	return pool;
}


bool vk_descriptor_init()
{
	// create compute shader pool
	{
		vk_desc_pool_size_ratio_t ratios[ 1 ] = {
			{
			  .type  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			  .ratio = 1,  // 100% of the pool
			},
		};

		g_vk_desc_pool = vk_descriptor_pool_create( "Global Pool", 10, ratios, 1 );

		if ( g_vk_desc_pool == VK_NULL_HANDLE )
		{
			Log_Error( gLC_Render, "Failed to create global descriptor pool\n" );
			return false;
		}

		// now descriptor set layout for compute draw
		VkDescriptorSetLayoutBinding bindings[ 1 ] = {
			{
			  .binding         = 0,
			  .descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
			  .descriptorCount = 1,
			  .stageFlags      = VK_SHADER_STAGE_COMPUTE_BIT,
			},
		};

		VkDescriptorSetLayoutCreateInfo layout_info{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
		layout_info.bindingCount = 1;
		layout_info.pBindings    = bindings;

		if ( vk_check_e( vkCreateDescriptorSetLayout( g_vk_device, &layout_info, nullptr, &g_vk_desc_draw_image_layout ), "Failed to create descriptor set layout" ) )
			return false;
	}

	// create imgui pool
	{
		VkDescriptorPoolSize       pool_sizes[] = { { VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
			                                        { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
			                                        { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
			                                        { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
			                                        { VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
			                                        { VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
			                                        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
			                                        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
			                                        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
			                                        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
			                                        { VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 } };

		VkDescriptorPoolCreateInfo pool_info{ VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO };
		pool_info.flags         = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
		pool_info.maxSets       = 1000;
		pool_info.poolSizeCount = ARR_SIZE( pool_sizes );
		pool_info.pPoolSizes    = pool_sizes;

		if ( vk_check_e( vkCreateDescriptorPool( g_vk_device, &pool_info, nullptr, &g_vk_imgui_desc_pool ), "Failed to create ImGui Descriptor Pool" ) )
			return false;

		vk_set_name( VK_OBJECT_TYPE_DESCRIPTOR_POOL, (u64)g_vk_imgui_desc_pool, "Dear ImGui" );
	}

	return true;
}


void vk_descriptor_destroy()
{
	if ( g_vk_desc_pool )
	{
		vkDestroyDescriptorPool( g_vk_device, g_vk_desc_pool, nullptr );
		g_vk_desc_pool = VK_NULL_HANDLE;
	}

	if ( g_vk_desc_draw_image_layout )
	{
		vkDestroyDescriptorSetLayout( g_vk_device, g_vk_desc_draw_image_layout, nullptr );
		g_vk_desc_draw_image_layout = VK_NULL_HANDLE;
	}

	if ( g_vk_imgui_desc_pool )
	{
		vkDestroyDescriptorPool( g_vk_device, g_vk_imgui_desc_pool, nullptr );
		g_vk_imgui_desc_pool = VK_NULL_HANDLE;
	}
}


void vk_descriptor_update_window( r_window_data_t* window )
{
	VkDescriptorImageInfo image_info{};
	image_info.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
	image_info.imageView   = window->draw_image.view;

	VkWriteDescriptorSet draw_image_write{ VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
	draw_image_write.dstBinding      = 0;
	draw_image_write.dstSet          = window->desc_draw_image;
	draw_image_write.descriptorCount = 1;
	draw_image_write.descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
	draw_image_write.pImageInfo      = &image_info;

	vkUpdateDescriptorSets( g_vk_device, 1, &draw_image_write, 0, nullptr );
}


bool vk_descriptor_allocate_window( r_window_data_t* window )
{
	// allocate descriptor sets
	VkDescriptorSetAllocateInfo alloc_info{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
	alloc_info.descriptorPool     = g_vk_desc_pool;
	alloc_info.descriptorSetCount = 1;
	alloc_info.pSetLayouts        = &g_vk_desc_draw_image_layout;

	if ( vk_check_e( vkAllocateDescriptorSets( g_vk_device, &alloc_info, &window->desc_draw_image ), "Failed to allocate descriptor set" ) )
		return false;

	vk_descriptor_update_window( window );

	return true;
}

