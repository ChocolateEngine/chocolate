#include "core/platform.h"
#include "core/log.h"
#include "util.h"

#include "render_vk.h"


static uint32_t                                                     gDescriptorCount    = 128;
static uint32_t                                                     gDescriptorPoolSize = 128;
static VkDescriptorPool                                             gDescriptorPool;
static std::vector< VkDescriptorSet >                               gDescriptorSets;

static Handle                                                       gSamplerLayoutHandle;
static std::vector< Handle >                                        gSamplerSetsHandles;
static VkDescriptorSetLayout                                        gImageLayout;
static std::vector< VkDescriptorSet >                               gImageSets;
constexpr u32                                                       MAX_IMAGES = 1000;

static std::unordered_map< Handle, std::vector< VkDescriptorSet > > gUniformSets;

// static std::vector< BufferVK* >                                     gUniformBuffers;
// constexpr u32                                                       MAX_UNIFORM_BUFFERS = 1000;

static VkDescriptorSetLayout                                        gImageStorageLayout;
static std::vector< VkDescriptorSet >                               gImageStorage;
static std::vector< TextureVK* >                                    gImageStorageTextures;  // why
bool                                                                gImageStorageUpdate = true;
constexpr u32                                                       MAX_STORAGE_IMAGES  = 10;

static ResourceList< VkDescriptorSetLayout >                        gDescLayouts;
static ResourceList< VkDescriptorSet >                              gDescSets;

extern std::vector< TextureVK* >                                    gTextures;
extern ResourceList< BufferVK >                                     gBufferHandles;


VkDescriptorType VK_ToVKDescriptorType( EDescriptorType type )
{
	switch ( type )
	{
		default:
		case EDescriptorType_Max:
			Log_Error( gLC_Render, "Unknown Descriptor Type!" );
			return VK_DESCRIPTOR_TYPE_MAX_ENUM;

		case EDescriptorType_Sampler:
			return VK_DESCRIPTOR_TYPE_SAMPLER;

		case EDescriptorType_CombinedImageSampler:
			return VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;

		case EDescriptorType_SampledImage:
			return VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;

		case EDescriptorType_StorageImage:
			return VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;

		case EDescriptorType_UniformTexelBuffer:
			return VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER;

		case EDescriptorType_StorageTexelBuffer:
			return VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER;

		case EDescriptorType_UniformBuffer:
			return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;

		case EDescriptorType_StorageBuffer:
			return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;

		case EDescriptorType_UniformBufferDynamic:
			return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;

		case EDescriptorType_StorageBufferDynamic:
			return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC;

		case EDescriptorType_InputAttachment:
			return VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
	}
}


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


void VK_AllocSets()
{
	// Allocate Image Sets
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
		gSamplerSetsHandles.resize( VK_GetSwapImageCount() );
		VK_CheckResult( vkAllocateDescriptorSets( VK_GetDevice(), &a, gImageSets.data() ), "Failed to allocate image descriptor sets!" );
		
		for ( u32 i = 0; i < gImageSets.size(); i++ )
			gSamplerSetsHandles[ i ] = gDescSets.Add( gImageSets[ i ] );
	}

#if 0
	// ----------------------------------------------------------------------------
	// Allocate Uniform Buffer Descriptor Sets
	{
		uint32_t                                           counts[] = { MAX_UNIFORM_BUFFERS, MAX_UNIFORM_BUFFERS };
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
		VK_CheckResult( vkAllocateDescriptorSets( VK_GetDevice(), &a, gImageSets.data() ), "Failed to allocate uniform buffer descriptor sets!" );
	}
#endif

	// ----------------------------------------------------------------------------
	// Allocate Image Storage Descriptor Sets
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
			// if ( gTextures[ j ]->aRenderTarget )
			// 	continue;

			// must be a sampled image
			if ( !(gTextures[ j ]->aUsage & VK_IMAGE_USAGE_SAMPLED_BIT) )
				continue;

			VkDescriptorImageInfo img{};
			img.imageLayout        = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			img.imageView          = gTextures[ j ]->aImageView;
			img.sampler            = VK_GetSampler( gTextures[ j ]->aFilter );

			gTextures[ j ]->aIndex = index++;
			infos.push_back( img );
		}

		if ( infos.empty() )
			continue;

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

		for ( uint32_t j = 0; j < gImageStorageTextures.size(); j++ )
		{
			VkDescriptorImageInfo img{};
			img.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
			img.imageView   = gImageStorageTextures[ j ]->aImageView;
			img.sampler     = VK_GetSampler( gImageStorageTextures[ j ]->aFilter );
			infos.push_back( img );
		}

		if ( infos.empty() )
			continue;

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
		VkDescriptorBindingFlagsEXT  bindFlag = VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT_EXT | VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT_EXT;

		VkDescriptorSetLayoutBinding imageBinding{};
		imageBinding.descriptorCount                             = MAX_IMAGES;
		imageBinding.descriptorType                              = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		imageBinding.stageFlags                                  = VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT;
		imageBinding.binding                                     = 0;

		VkDescriptorSetLayoutBindingFlagsCreateInfoEXT extend{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO_EXT };
		extend.pNext         = nullptr;
		extend.bindingCount  = 1;
		extend.pBindingFlags = &bindFlag;

		VkDescriptorSetLayoutCreateInfo layoutInfo{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
		layoutInfo.pNext        = &extend;
		layoutInfo.bindingCount = 1;
		layoutInfo.pBindings    = &imageBinding;

		VK_CheckResult( vkCreateDescriptorSetLayout( VK_GetDevice(), &layoutInfo, NULL, &gImageLayout ), "Failed to create image descriptor set layout!" );

		gSamplerLayoutHandle = gDescLayouts.Add( gImageLayout );
	}

	// ----------------------------------------------------------------------------
	// Uniform Buffers Layout
#if 0
	{
		VkDescriptorBindingFlagsEXT  bindFlag = VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT_EXT | VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT_EXT;

		VkDescriptorSetLayoutBinding layoutBinding{};
		layoutBinding.descriptorCount                           = MAX_IMAGES;
		layoutBinding.descriptorType                            = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		layoutBinding.stageFlags                                = VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT;
		layoutBinding.binding                                   = 0;

		VkDescriptorSetLayoutBindingFlagsCreateInfoEXT extend{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO_EXT };
		extend.pNext         = nullptr;
		extend.bindingCount  = 1;
		extend.pBindingFlags = &bindFlag;

		VkDescriptorSetLayoutCreateInfo layoutInfo{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
		layoutInfo.pNext        = &extend;
		layoutInfo.bindingCount = 1;
		layoutInfo.pBindings    = &layoutBinding;

		VK_CheckResult( vkCreateDescriptorSetLayout( VK_GetDevice(), &layoutInfo, NULL, &gImageLayout ), "Failed to create image descriptor set layout!" );
	}
#endif

	// ----------------------------------------------------------------------------
	// Texture Storage Layout
	{
		VkDescriptorBindingFlagsEXT  bindFlag = VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT_EXT | VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT_EXT;

		VkDescriptorSetLayoutBinding samplerLayoutBinding{};
		samplerLayoutBinding.binding                             = 0;
		samplerLayoutBinding.descriptorCount                     = MAX_STORAGE_IMAGES;
		samplerLayoutBinding.descriptorType                      = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
		samplerLayoutBinding.pImmutableSamplers                  = nullptr;
		samplerLayoutBinding.stageFlags                          = VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT;

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

	VK_AllocSets();
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


Handle VK_CreateVariableDescLayout( const CreateVariableDescLayout_t& srCreate )
{
	// Create the layouts
	VkDescriptorBindingFlagsEXT  bindFlag = VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT_EXT | VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT_EXT;

	VkDescriptorSetLayoutBinding layoutBinding{};
	layoutBinding.descriptorCount = srCreate.aCount;
	layoutBinding.descriptorType  = VK_ToVKDescriptorType( srCreate.aType );
	layoutBinding.stageFlags      = VK_ToVkShaderStage( srCreate.aStages );
	layoutBinding.binding         = srCreate.aBinding;

	VkDescriptorSetLayoutBindingFlagsCreateInfoEXT extend{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO_EXT };
	extend.pNext         = nullptr;
	extend.bindingCount  = 1;
	extend.pBindingFlags = &bindFlag;

	VkDescriptorSetLayoutCreateInfo layoutInfo{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
	layoutInfo.pNext        = &extend;
	layoutInfo.bindingCount = 1;
	layoutInfo.pBindings    = &layoutBinding;

	VkDescriptorSetLayout layout;

	VK_CheckResult( vkCreateDescriptorSetLayout( VK_GetDevice(), &layoutInfo, NULL, &layout ), "Failed to create image descriptor set layout!" );

#ifdef _DEBUG
	// add a debug label onto it
	if ( pfnSetDebugUtilsObjectName && srCreate.apName )
	{
		const VkDebugUtilsObjectNameInfoEXT nameInfo = {
			VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,  // sType
			NULL,                                                // pNext
			VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT,                // objectType
			(uint64_t)layout,                                    // objectHandle
			srCreate.apName,                                     // pObjectName
		};

		pfnSetDebugUtilsObjectName( VK_GetDevice(), &nameInfo );
	}
#endif

	return gDescLayouts.Add( layout );
}


bool VK_AllocateVariableDescLayout( const AllocVariableDescLayout_t& srCreate, Handle* handles )
{
	VkDescriptorSetLayout layout = VK_GetDescLayout( srCreate.aLayout );
	if ( layout == VK_NULL_HANDLE )
		return false;

	// ----------------------------------------------------------------------------
	// Allocate the Layouts
	uint32_t* counts = new uint32_t[ srCreate.aSetCount ];

	for ( u32 i = 0; i < srCreate.aSetCount; i++ )
		counts[ i ] = srCreate.aCount;

	VkDescriptorSetVariableDescriptorCountAllocateInfo dc{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_VARIABLE_DESCRIPTOR_COUNT_ALLOCATE_INFO };
	dc.descriptorSetCount = srCreate.aSetCount;
	dc.pDescriptorCounts  = counts;

	std::vector< VkDescriptorSetLayout > layouts( srCreate.aSetCount, layout );

	VkDescriptorSetAllocateInfo          a{};
	a.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	a.pNext              = &dc;
	a.descriptorPool     = VK_GetDescPool();
	a.descriptorSetCount = srCreate.aSetCount;
	a.pSetLayouts        = layouts.data();

	VkDescriptorSet* descSets = new VkDescriptorSet[ srCreate.aSetCount ];

	VK_CheckResult( vkAllocateDescriptorSets( VK_GetDevice(), &a, descSets ), "Failed to allocate uniform buffer descriptor sets!" );

	for ( u32 i = 0; i < srCreate.aSetCount; i++ )
	{
		handles[ i ] = gDescSets.Add( descSets[ i ] );

#ifdef _DEBUG
		// add a debug label onto it
		if ( pfnSetDebugUtilsObjectName && srCreate.apName )
		{
			const VkDebugUtilsObjectNameInfoEXT nameInfo = {
				VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,  // sType
				NULL,                                                // pNext
				VK_OBJECT_TYPE_DESCRIPTOR_SET,                       // objectType
				(uint64_t)descSets[ i ],                             // objectHandle
				srCreate.apName,                                     // pObjectName
			};

			pfnSetDebugUtilsObjectName( VK_GetDevice(), &nameInfo );
		}
#endif
	}

	delete[] descSets;
	delete[] counts;

	return true;
}


void VK_FreeVariableDescLayout( Handle sLayout )
{
}


void VK_UpdateVariableDescSet( const UpdateVariableDescSet_t& srUpdate )
{
	VK_WaitForGraphicsQueue();

	for ( uint32_t i = 0; i < srUpdate.aDescSets.size(); ++i )
	{
		if ( srUpdate.aBuffers.empty() )
			continue;

		VkDescriptorType                      type    = VK_ToVKDescriptorType( srUpdate.aType );
		VkDescriptorSet                       descSet = VK_GetDescSet( srUpdate.aDescSets[ i ] );

		std::vector< VkDescriptorBufferInfo > buffers( srUpdate.aBuffers.size() );
		std::vector< VkDescriptorImageInfo >  images( srUpdate.aImages.size() );

		for ( uint32_t j = 0; j < srUpdate.aBuffers.size(); j++ )
		{
			BufferVK* buffer = nullptr;
			if ( !gBufferHandles.Get( srUpdate.aBuffers[ j ], &buffer ) )
			{
				Log_ErrorF( gLC_Render, "Failed to find buffer %u\n", j );
				continue;
			}

			buffers[ j ].buffer = buffer->aBuffer;
			buffers[ j ].offset = 0;
			buffers[ j ].range  = buffer->aSize;
		}

		for ( uint32_t j = 0; j < srUpdate.aImages.size(); j++ )
		{
			TextureVK* tex = VK_GetTexture( srUpdate.aImages[ j ] );
			if ( !tex )
			{
				Log_ErrorF( gLC_Render, "Failed to find texture %u\n", j );
				continue;
			}

			images[ j ].imageLayout = VK_IMAGE_LAYOUT_GENERAL;
			images[ j ].imageView   = tex->aImageView;
			images[ j ].sampler     = VK_GetSampler( tex->aFilter );
		}

		VkWriteDescriptorSet w{ VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
		w.pNext            = nullptr;
		w.dstSet           = descSet;
		w.dstBinding       = 0;
		w.dstArrayElement  = 0;
		w.descriptorCount  = static_cast< uint32_t >( buffers.size() + images.size() );
		w.descriptorType   = VK_ToVKDescriptorType( srUpdate.aType );
		w.pImageInfo       = images.data();
		w.pBufferInfo      = buffers.data();
		w.pTexelBufferView = nullptr;

		vkUpdateDescriptorSets( VK_GetDevice(), 1, &w, 0, nullptr );
	}
}


VkDescriptorPool VK_GetDescPool()
{
	return gDescriptorPool;
}


VkDescriptorSetLayout VK_GetImageLayout()
{
	return gImageLayout;
}


Handle VK_GetSamplerLayoutHandle()
{
	return gSamplerLayoutHandle;
}

const std::vector< Handle >& VK_GetSamplerSetsHandles()
{
	return gSamplerSetsHandles;
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


VkDescriptorSetLayout VK_GetDescLayout( Handle sHandle )
{
	VkDescriptorSetLayout layout = VK_NULL_HANDLE;
	if ( !gDescLayouts.Get( sHandle, &layout ) )
		Log_Error( gLC_Render, "VK_GetDescLayout(): Failed to get descriptor set layout!\n" );

	return layout;
}


VkDescriptorSet VK_GetDescSet( Handle sHandle )
{
	VkDescriptorSet descSet = VK_NULL_HANDLE;
	if ( !gDescSets.Get( sHandle, &descSet ) )
		Log_Error( gLC_Render, "VK_GetDescLayout(): Failed to get descriptor set!\n" );

	return descSet;
}

