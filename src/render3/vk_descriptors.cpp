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
	aDescriptorPoolInfo.maxSets       = gDescriptorSetCount;

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

