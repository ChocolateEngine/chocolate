#include "core/platform.h"
#include "core/log.h"
#include "util.h"

#include "render_vk.h"


static uint32_t                       gDescriptorCount = 128;
static uint32_t                       gDescriptorPoolSize = 128;
static VkDescriptorPool               gDescriptorPool;
static std::vector< VkDescriptorSet > gDescriptorSets;

VkDescriptorSetLayout                 gImageLayout;
std::vector< VkDescriptorSet >        gImageSets;
constexpr u32                         MAX_IMAGES = 1000;

VkDescriptorSetLayout                 gImageStorageLayout;
std::vector< VkDescriptorSet >        gImageStorage;
std::vector< TextureVK* >             gImageStorageTextures;  // why
bool                                  gImageStorageUpdate = true;
constexpr u32                         MAX_STORAGE_IMAGES = 10;

extern std::vector< TextureVK* >      gTextures;


void VK_CreateDescriptorPool()
{
	VkDescriptorPoolSize aPoolSizes[] = {
		// { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, gDescriptorPoolSize },
		{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, gDescriptorPoolSize }
	};

	VkDescriptorPoolCreateInfo aDescriptorPoolInfo = {};
	aDescriptorPoolInfo.sType                      = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	aDescriptorPoolInfo.poolSizeCount              = ARR_SIZE( aPoolSizes );
	aDescriptorPoolInfo.pPoolSizes                 = aPoolSizes;
	aDescriptorPoolInfo.maxSets                    = gDescriptorCount;

	VK_CheckResult( vkCreateDescriptorPool( VK_GetDevice(), &aDescriptorPoolInfo, nullptr, &gDescriptorPool ), "Failed to create descriptor pool!" );
}


// TODO: Rethink this
#if 0
void VK_CreateDescriptorSetLayouts()
{
	VkDescriptorSetLayoutBinding aLayoutBindings[] = {
		// { 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, nullptr },
		{ 0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_VERTEX_BIT, nullptr },
	};

	VkDescriptorSetLayoutCreateInfo bufferLayout = {};
	bufferLayout.sType                           = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	bufferLayout.pNext                           = nullptr;
	bufferLayout.flags                           = 0;
	bufferLayout.bindingCount                    = 1;
	bufferLayout.pBindings                       = &aLayoutBindings[ 0 ];

	VK_CheckResult( vkCreateDescriptorSetLayout( VK_GetDevice(), &bufferLayout, nullptr, &gBufferLayout ), "Failed to create descriptor set layout!" );

	VkDescriptorSetLayoutCreateInfo imageLayout = {};
	imageLayout.sType                           = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	imageLayout.pNext                           = nullptr;
	imageLayout.flags                           = 0;
	imageLayout.bindingCount                    = 1;
	imageLayout.pBindings                       = &aLayoutBindings[ 1 ];

	VK_CheckResult( vkCreateDescriptorSetLayout( VK_GetDevice(), &imageLayout, nullptr, &gImageLayout ), "Failed to create descriptor set layout!" );
}
#endif


void VK_AllocImageSets()
{
	uint32_t                                           counts[] = { MAX_IMAGES, MAX_IMAGES };
	VkDescriptorSetVariableDescriptorCountAllocateInfo dc{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_VARIABLE_DESCRIPTOR_COUNT_ALLOCATE_INFO };
	dc.descriptorSetCount = VK_GetSwapImageCount();
	dc.pDescriptorCounts  = counts;

	std::vector< VkDescriptorSetLayout > layouts( VK_GetSwapImageCount(), gImageLayout );

	VkDescriptorSetAllocateInfo          a{};
	a.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	a.pNext              = &dc;
	a.descriptorPool     = VK_GetDescPool();
	a.descriptorSetCount = VK_GetSwapImageCount();
	a.pSetLayouts        = layouts.data();

	gImageSets.resize( VK_GetSwapImageCount() );
	VK_CheckResult( vkAllocateDescriptorSets( VK_GetDevice(), &a, gImageSets.data() ), "Failed to allocate descriptor sets!" );
}


void VK_AllocImageStorageSets()
{
	uint32_t                                           counts[] = { MAX_STORAGE_IMAGES, MAX_STORAGE_IMAGES };
	VkDescriptorSetVariableDescriptorCountAllocateInfo dc{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_VARIABLE_DESCRIPTOR_COUNT_ALLOCATE_INFO };
	dc.descriptorSetCount = VK_GetSwapImageCount();
	dc.pDescriptorCounts  = counts;

	std::vector< VkDescriptorSetLayout > layouts( VK_GetSwapImageCount(), gImageStorageLayout );

	VkDescriptorSetAllocateInfo          a{};
	a.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	a.pNext              = &dc;
	a.descriptorPool     = VK_GetDescPool();
	a.descriptorSetCount = VK_GetSwapImageCount();
	a.pSetLayouts        = layouts.data();

	gImageStorage.resize( VK_GetSwapImageCount() );
	VK_CheckResult( vkAllocateDescriptorSets( VK_GetDevice(), &a, gImageStorage.data() ), "Failed to allocate descriptor sets!" );
}


void VK_UpdateImageSets()
{
	if ( !gTextures.size() )
		return;

	// hmm, this doesn't crash on Nvidia, though idk how AMD would react
	// also would this be >= or just >, lol
	if ( gTextures.size() >= MAX_IMAGES )
	{
		Log_FatalF( gLC_Render, "Over Max Images allocated (at %zu, max is %d)", gTextures.size(), MAX_IMAGES );
	}

	for ( uint32_t i = 0; i < gImageSets.size(); ++i )
	{
		std::vector< VkDescriptorImageInfo > infos;
		size_t                               index = 0;

		for ( uint32_t j = 0; j < gTextures.size(); j++ )
		{
			// skip render targets (for now at least)
			if ( gTextures[ j ]->aRenderTarget )
				continue;

			VkDescriptorImageInfo img{};
			img.imageLayout        = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			img.imageView          = gTextures[ j ]->aImageView;
			img.sampler            = VK_GetSampler();

			gTextures[ j ]->aIndex = index++;
			infos.push_back( img );
		}

		VkWriteDescriptorSet w{};
		w.sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		w.dstBinding      = 0;
		w.dstArrayElement = 0;
		w.descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		w.descriptorCount = infos.size();
		w.pBufferInfo     = 0;
		w.dstSet          = gImageSets[ i ];
		w.pImageInfo      = infos.data();

		vkUpdateDescriptorSets( VK_GetDevice(), 1, &w, 0, nullptr );
	}
}


void VK_UpdateImageStorage()
{
	if ( !gImageStorageUpdate || gImageStorageTextures.empty() )
		return;

	for ( uint32_t i = 0; i < gImageStorage.size(); ++i )
	{
		std::vector< VkDescriptorImageInfo > infos;
		size_t                               index = 0;

		for ( uint32_t j = 0; j < gImageStorageTextures.size(); j++ )
		{
			VkDescriptorImageInfo img{};
			img.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
			img.imageView   = gImageStorageTextures[ j ]->aImageView;
			img.sampler     = VK_GetSampler();
			infos.push_back( img );
		}

		VkWriteDescriptorSet w{};
		w.sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		w.dstBinding      = 0;
		w.dstArrayElement = 0;
		w.descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
		w.descriptorCount = infos.size();
		w.pBufferInfo     = 0;
		w.dstSet          = gImageStorage[ i ];
		w.pImageInfo      = infos.data();

		vkUpdateDescriptorSets( VK_GetDevice(), 1, &w, 0, nullptr );
	}

	gImageStorageUpdate = false;
}


void VK_AddImageStorage( TextureVK* spTexture )
{
	if ( !spTexture )
		return;

	size_t index = vec_index( gImageStorageTextures, spTexture );

	if ( index != SIZE_MAX )
		return;

	gImageStorageTextures.push_back( spTexture );
	gImageStorageUpdate = true;
}


void VK_RemoveImageStorage( TextureVK* spTexture )
{
	if ( !spTexture )
		return;

	size_t index = vec_index( gImageStorageTextures, spTexture );

	if ( index == SIZE_MAX )
		return;

	vec_remove_index( gImageStorageTextures, index );
	gImageStorageUpdate = true;
}


void VK_CreateDescriptorSetLayouts()
{
	// Texture Samplers
	{
		VkDescriptorSetLayoutBinding imageBinding{};
		imageBinding.descriptorCount                             = MAX_IMAGES;
		imageBinding.descriptorType                              = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		imageBinding.stageFlags                                  = VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT;
		imageBinding.binding                                     = 0;

		VkDescriptorBindingFlagsEXT                    bindFlag  = VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT_EXT | VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT_EXT;

		VkDescriptorSetLayoutBindingFlagsCreateInfoEXT extend{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO_EXT };
		extend.pNext         = nullptr;
		extend.bindingCount  = 1;
		extend.pBindingFlags = &bindFlag;

		VkDescriptorSetLayoutCreateInfo layoutInfo{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
		layoutInfo.pNext        = &extend;
		layoutInfo.bindingCount = 1;
		layoutInfo.pBindings    = &imageBinding;

		VK_CheckResult( vkCreateDescriptorSetLayout( VK_GetDevice(), &layoutInfo, NULL, &gImageLayout ), "Failed to create image descriptor set layout!" );
	}

	// ----------------------------------------------------------------------------
	// Texture Storage Layout
	{
		VkDescriptorSetLayoutBinding samplerLayoutBinding{};
		samplerLayoutBinding.binding                             = 0;
		samplerLayoutBinding.descriptorCount                     = MAX_STORAGE_IMAGES;
		samplerLayoutBinding.descriptorType                      = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
		samplerLayoutBinding.pImmutableSamplers                  = nullptr;
		samplerLayoutBinding.stageFlags                          = VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT;

		VkDescriptorBindingFlagsEXT                    bindFlag  = VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT_EXT | VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT_EXT;

		VkDescriptorSetLayoutBindingFlagsCreateInfoEXT extend{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO_EXT };
		extend.pNext         = nullptr;
		extend.bindingCount  = 1;
		extend.pBindingFlags = &bindFlag;

		VkDescriptorSetLayoutCreateInfo samplerLayoutInfo{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
		samplerLayoutInfo.pNext = &extend;
		samplerLayoutInfo.bindingCount = 1;
		samplerLayoutInfo.pBindings    = &samplerLayoutBinding;

		VK_CheckResult( vkCreateDescriptorSetLayout( VK_GetDevice(), &samplerLayoutInfo, NULL, &gImageStorageLayout ), "Failed to create image descriptor set layout!" );
	}

	VK_AllocImageSets();
	VK_AllocImageStorageSets();
}


void VK_CreateDescSets()
{
	VK_CreateDescriptorPool();
	VK_CreateDescriptorSetLayouts();
}


void VK_DestroyDescSets()
{
	if ( gImageLayout )
		vkDestroyDescriptorSetLayout( VK_GetDevice(), gImageLayout, nullptr );

	if ( gImageStorageLayout )
		vkDestroyDescriptorSetLayout( VK_GetDevice(), gImageStorageLayout, nullptr );

	if ( gDescriptorPool )
		vkDestroyDescriptorPool( VK_GetDevice(), gDescriptorPool, nullptr );
}


VkDescriptorPool VK_GetDescPool()
{
	return gDescriptorPool;
}


VkDescriptorSetLayout VK_GetImageLayout()
{
	return gImageLayout;
}


VkDescriptorSetLayout VK_GetImageStorageLayout()
{
	return gImageStorageLayout;
}


const std::vector< VkDescriptorSet >& VK_GetImageSets()
{
	return gImageSets;
}


const std::vector< VkDescriptorSet >& VK_GetImageStorage()
{
	return gImageStorage;
}


VkDescriptorSet VK_GetImageSet( size_t sIndex )
{
	if ( gImageSets.size() >= sIndex )
	{
		printf( "VK_GetImageSet() - sIndex out of range!\n" );
		return nullptr;
	}

	return gImageSets[ sIndex ];
}

