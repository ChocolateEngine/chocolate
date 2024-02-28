#include "core/platform.h"
#include "core/log.h"
#include "util.h"

#include "render_vk.h"


// TODO: allocate a new pool when we fill up the current pool


struct DescriptorPoolTypeStats_t
{
	u32 aAllocated = 0;
	u32 aUsed      = 0;
};


struct DescriptorPoolStats_t
{
	u32                       aSetsAllocated = 0;
	u32                       aSetsUsed      = 0;

	DescriptorPoolTypeStats_t aTypes[ EDescriptorType_Max ];
};


enum EDescriptorPoolSize
{
	EDescriptorPoolSize_Storage               = 32767,
	EDescriptorPoolSize_Uniform               = 128,
	EDescriptorPoolSize_CombinedImageSamplers = 4096,
	EDescriptorPoolSize_SampledImages         = 32767,
};


static u32                                                          gDescriptorSetCount = 512;

static VkDescriptorPool                                             gVkDescriptorPool;
// static std::vector< VkDescriptorSet >                               gDescriptorSets;

static DescriptorPoolStats_t                                        gDescriptorPoolStats;

// static std::unordered_map< Handle, std::vector< VkDescriptorSet > > gUniformSets;

// static std::vector< BufferVK* >                                     gUniformBuffers;
// constexpr u32                                                       MAX_UNIFORM_BUFFERS = 1000;

static ResourceList< VkDescriptorSetLayout >                        gDescLayouts;
static ResourceList< VkDescriptorSet >                              gDescSets;

extern ResourceList< TextureVK* >                                   gTextureHandles;
extern ResourceList< BufferVK >                                     gBufferHandles;

static std::vector< ChHandle_t >                                    gImageSets;
static u32                                                          gImageBinding = 0;

extern Render_OnTextureIndexUpdate*                                 gpOnTextureIndexUpdateFunc;


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


static const char* gDescriptorTypeStr[] = {
	"Sampler",
	"CombinedImageSampler",
	"SampledImage",
	"StorageImage",
	"UniformTexelBuffer",
	"StorageTexelBuffer",
	"UniformBuffer",
	"StorageBuffer",
	"UniformBufferDynamic",
	"StorageBufferDynamic",
	"InputAttachment",
};


static_assert( CH_ARR_SIZE( gDescriptorTypeStr ) == EDescriptorType_Max );


void VK_CreateDescriptorPool()
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
	aDescriptorPoolInfo.flags         = VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT;

	VK_CheckResult( vkCreateDescriptorPool( VK_GetDevice(), &aDescriptorPoolInfo, nullptr, &gVkDescriptorPool ), "Failed to create descriptor pool!" );

	gDescriptorPoolStats.aSetsAllocated                                            = gDescriptorSetCount;
	gDescriptorPoolStats.aTypes[ EDescriptorType_CombinedImageSampler ].aAllocated = EDescriptorPoolSize_CombinedImageSamplers;
	gDescriptorPoolStats.aTypes[ EDescriptorType_SampledImage ].aAllocated         = EDescriptorPoolSize_SampledImages;
	gDescriptorPoolStats.aTypes[ EDescriptorType_UniformBuffer ].aAllocated        = EDescriptorPoolSize_Uniform;
	gDescriptorPoolStats.aTypes[ EDescriptorType_StorageBuffer ].aAllocated        = EDescriptorPoolSize_Storage;

	VK_SetObjectName( VK_OBJECT_TYPE_DESCRIPTOR_POOL, (u64)gVkDescriptorPool, "Global Descriptor Pool" );
}


void VK_CalcTextureIndices()
{
	size_t index = 0;
	for ( uint32_t j = 0; j < gTextureHandles.size(); j++ )
	{
		TextureVK* tex = VK_GetTexture( gTextureHandles.aHandles[ j ] );

		if ( !tex )
			continue;

		// must be a sampled image
		if ( !( tex->aUsage & VK_IMAGE_USAGE_SAMPLED_BIT ) )
			continue;

		tex->aIndex = index++;
	}

	Log_DevF( gLC_Render, 2, "Texture Index Count: %d\n", index );

	if ( gpOnTextureIndexUpdateFunc )
		gpOnTextureIndexUpdateFunc();
}

#if 1
void VK_UpdateImageSets()
{
	// hmm, this doesn't crash on Nvidia, though idk how AMD would react
	// also would this be >= or just >, lol
	if ( gGraphicsAPIData.aSampledTextures.size() >= EDescriptorPoolSize_CombinedImageSamplers )
	{
		Log_FatalF( gLC_Render, "Over Max Sampled Textures allocated (at %zu, max is %d)", gGraphicsAPIData.aSampledTextures.size(), EDescriptorPoolSize_CombinedImageSamplers );
	}

	WriteDescSet_t write{};
	write.aDescSetCount = gImageSets.size();
	write.apDescSets    = gImageSets.data();

	write.aBindingCount = 1;
	write.apBindings    = ch_calloc_count< WriteDescSetBinding_t >( 1 );

	ChVector< ChHandle_t > imageData;
	imageData.reserve( gGraphicsAPIData.aSampledTextures.size() );

	size_t index = 0;
	// for ( uint32_t j = 0; j < gGraphicsAPIData.aSampledTextures.size(); j++ )
	for ( ChHandle_t texHandle : gGraphicsAPIData.aSampledTextures )
	{
		TextureVK* tex = VK_GetTexture( texHandle );

		if ( !tex )
			continue;

		// must be a sampled image
		if ( !( tex->aUsage & VK_IMAGE_USAGE_SAMPLED_BIT ) )
			continue;

		tex->aIndex = index++;
		imageData.push_back( texHandle );
	}

	write.apBindings[ 0 ].aBinding = gImageBinding;
	write.apBindings[ 0 ].aType    = EDescriptorType_CombinedImageSampler;
	write.apBindings[ 0 ].aCount   = imageData.size();
	write.apBindings[ 0 ].apData   = imageData.data();

	VK_UpdateDescSets( &write, 1 );

	free( write.apBindings );

	Log_Dev( gLC_Render, 1, "Updated Image Sets\n" );

	if ( gpOnTextureIndexUpdateFunc )
		gpOnTextureIndexUpdateFunc();
}
#endif


void VK_SetImageSets( ChHandle_t* spDescSets, int sCount, u32 sBinding )
{
	gImageBinding = sBinding;
	gImageSets.resize( sCount );

	for ( int i = 0; i < sCount; i++ )
	{
		gImageSets[ i ] = spDescSets[ i ];
	}
}


void VK_CreateDescSets()
{
	VK_CreateDescriptorPool();
}


void VK_DestroyDescSets()
{
	if ( gVkDescriptorPool )
		vkDestroyDescriptorPool( VK_GetDevice(), gVkDescriptorPool, nullptr );
}


// --------------------------------------------------------------------------------------------


Handle VK_CreateDescLayout( const CreateDescLayout_t& srCreate )
{
	std::vector< VkDescriptorBindingFlagsEXT >  layoutBindingFlags;
	std::vector< VkDescriptorSetLayoutBinding > layoutBindings;

	layoutBindingFlags.resize( srCreate.aBindings.size() );
	layoutBindings.resize( srCreate.aBindings.size() );

	// Create the layout bindings
	for ( size_t i = 0; i < srCreate.aBindings.size(); i++ )
	{
		const CreateDescBinding_t& createBinding = srCreate.aBindings[ i ];

		// Unused descriptors don't need to be filled in with this
		layoutBindingFlags[ i ]                  = VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT;

		// if ( createBinding.aCount > 1 )
		// 	layoutBindingFlags[ i ] |= VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT;

		layoutBindings[ i ].descriptorCount = createBinding.aCount;
		layoutBindings[ i ].descriptorType  = VK_ToVKDescriptorType( createBinding.aType );
		layoutBindings[ i ].stageFlags      = VK_ToVkShaderStage( createBinding.aStages );
		layoutBindings[ i ].binding         = createBinding.aBinding;

		// Check Pool Stats for this Descriptor Type
		DescriptorPoolTypeStats_t& typeStats = gDescriptorPoolStats.aTypes[ layoutBindings[ i ].descriptorType ];

		if ( typeStats.aUsed >= typeStats.aAllocated )
		{
			Log_ErrorF( gLC_Render, "Out of %s Descriptors (Max of %zd)\n",
			            gDescriptorTypeStr[ layoutBindings[ i ].descriptorType ], typeStats.aAllocated );

			return CH_INVALID_HANDLE;
		}

		typeStats.aUsed += createBinding.aCount;
	}

	VkDescriptorSetLayoutBindingFlagsCreateInfoEXT extend{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO };
	extend.pNext         = nullptr;
	extend.bindingCount  = static_cast< u32 >( layoutBindingFlags.size() );
	extend.pBindingFlags = layoutBindingFlags.data();

	VkDescriptorSetLayoutCreateInfo layoutInfo{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
	layoutInfo.pNext        = &extend;
	layoutInfo.bindingCount = static_cast< u32 >( layoutBindings.size() );
	layoutInfo.pBindings    = layoutBindings.data();

	VkDescriptorSetLayout layout;

	VK_CheckResult( vkCreateDescriptorSetLayout( VK_GetDevice(), &layoutInfo, NULL, &layout ), "Failed to create descriptor set layout!" );

	VK_SetObjectName( VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT, (u64)layout, srCreate.apName );

	return gDescLayouts.Add( layout );
}


bool VK_AllocateDescLayout( const AllocDescLayout_t& srCreate, Handle* handles )
{
	if ( handles == nullptr )
		return false;

	VkDescriptorSetLayout layout = VK_GetDescLayout( srCreate.aLayout );
	if ( layout == VK_NULL_HANDLE )
		return false;

	const char* name = srCreate.apName ? srCreate.apName : "unnamed";

	if ( gDescriptorPoolStats.aSetsUsed + srCreate.aSetCount >= gDescriptorPoolStats.aSetsAllocated )
	{
		Log_ErrorF( gLC_Render, "Out of Descriptor Sets in Pool trying to allocate a set for \"%s\" (Max of %zd)\n", name, gDescriptorPoolStats.aSetsAllocated );
		return false;
	}

	gDescriptorPoolStats.aSetsUsed += srCreate.aSetCount;

	// ----------------------------------------------------------------------------
	// Allocate the Layouts

	std::vector< VkDescriptorSetLayout > layouts( srCreate.aSetCount, layout );

	VkDescriptorSetAllocateInfo          a{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
	a.pNext                   = nullptr;
	a.descriptorPool          = VK_GetDescPool();
	a.descriptorSetCount      = srCreate.aSetCount;
	a.pSetLayouts             = layouts.data();

	VkDescriptorSet* descSets = new VkDescriptorSet[ srCreate.aSetCount ];

	VK_CheckResultF( vkAllocateDescriptorSets( VK_GetDevice(), &a, descSets ), "Failed to Allocate Descriptor Sets for \"%s\"", name );

	gDescSets.EnsureSize( srCreate.aSetCount );
	for ( u32 i = 0; i < srCreate.aSetCount; i++ )
	{
		handles[ i ] = gDescSets.Add( descSets[ i ] );
		VK_SetObjectName( VK_OBJECT_TYPE_DESCRIPTOR_SET, (u64)descSets[ i ], srCreate.apName );
	}

	delete[] descSets;

	return true;
}


void VK_FreeDescLayout( Handle sLayout )
{
}


// --------------------------------------------------------------------------------------------


// currently usused
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

	VkDescriptorSetAllocateInfo          a{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
	a.pNext              = &dc;
	a.descriptorPool     = VK_GetDescPool();
	a.descriptorSetCount = srCreate.aSetCount;
	a.pSetLayouts        = layouts.data();

	VkDescriptorSet* descSets = new VkDescriptorSet[ srCreate.aSetCount ];

	VK_CheckResult( vkAllocateDescriptorSets( VK_GetDevice(), &a, descSets ), "Failed to Allocate Variable Descriptor Sets!" );

	gDescSets.EnsureSize( srCreate.aSetCount );
	for ( u32 i = 0; i < srCreate.aSetCount; i++ )
	{
		handles[ i ] = gDescSets.Add( descSets[ i ] );
		VK_SetObjectNameEx( VK_OBJECT_TYPE_DESCRIPTOR_SET, (u64)descSets[ i ], srCreate.apName, "Variable Descriptor Set" );
	}

	delete[] descSets;
	delete[] counts;

	return true;
}


void VK_FreeVariableDescLayout( Handle sLayout )
{
}


// TODO: this could be made better with multiple updates at once
void VK_UpdateDescSets( WriteDescSet_t* spUpdate, u32 sCount )
{
	if ( spUpdate == nullptr || sCount == 0 )
		return;

	ChVector< VkWriteDescriptorSet > writes;
	size_t totalWrites = 0;

	for ( uint32_t i = 0; i < sCount; i++ )
	{
		totalWrites += spUpdate[ i ].aDescSetCount * spUpdate[ i ].aBindingCount;
	}

	writes.resize( totalWrites );

	bool failed = false;

	u32  writeI = 0;
	for ( uint32_t updateI = 0; updateI < sCount; updateI++ )
	{
		WriteDescSet_t& update = spUpdate[ updateI ];

		for ( uint32_t i = 0; i < update.aDescSetCount; i++ )
		{
			for ( uint32_t b = 0; b < update.aBindingCount; b++ )
			{
				WriteDescSetBinding_t&  binding = update.apBindings[ b ];

				VkWriteDescriptorSet&   write   = writes[ writeI++ ];
				write.sType                     = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				write.pNext                     = nullptr;
				write.dstSet                    = VK_GetDescSet( update.apDescSets[ i ] );
				write.dstBinding                = binding.aBinding;
				write.dstArrayElement           = 0;
				write.descriptorCount           = binding.aCount;
				write.descriptorType            = VK_ToVKDescriptorType( binding.aType );
				write.pImageInfo                = nullptr;
				write.pBufferInfo               = nullptr;
				write.pTexelBufferView          = nullptr;

				if ( write.dstSet == VK_NULL_HANDLE )
				{
					Log_Error( gLC_Render, "Failed to Get Descriptor Set for VK_UpdateDescSets()\n" );
					failed = true;
					break;
				}
		
				switch ( binding.aType )
				{
					default:
					{
						Log_ErrorF( gLC_Render, "Unhandled Descriptor Type for VK_UpdateDescSets(): %c\n", binding.aType );
						failed = true;
						break;
					}

					case EDescriptorType_CombinedImageSampler:
					case EDescriptorType_SampledImage:
					case EDescriptorType_StorageImage:
					{
						auto images = ch_malloc_count< VkDescriptorImageInfo >( binding.aCount );

						for ( uint32_t j = 0; j < binding.aCount; j++ )
						{
							TextureVK* tex = VK_GetTexture( binding.apData[ j ] );
							if ( !tex )
							{
								Log_ErrorF( gLC_Render, "Failed to find texture index %u\n", j );
								continue;
							}

							images[ j ].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
							images[ j ].imageView   = tex->aImageView;
							images[ j ].sampler     = VK_GetSampler( tex->aFilter, tex->aSamplerAddress, tex->aDepthCompare );
						}

						write.pImageInfo = images;
						break;
					}

					case EDescriptorType_UniformBuffer:
					case EDescriptorType_StorageBuffer:
					case EDescriptorType_UniformBufferDynamic:
					case EDescriptorType_StorageBufferDynamic:
					{
						auto buffers = ch_malloc_count< VkDescriptorBufferInfo >( binding.aCount );

						for ( u32 j = 0; j < binding.aCount; j++ )
						{
							BufferVK* buffer = nullptr;
							if ( !gBufferHandles.Get( binding.apData[ j ], &buffer ) )
							{
								Log_ErrorF( gLC_Render, "Failed to find buffer %u in VK_UpdateDescSets()\n", j );
								failed = true;
								break;
							}

							buffers[ j ].buffer = buffer->aBuffer;
							buffers[ j ].offset = 0;
							buffers[ j ].range  = buffer->aSize;
						}

						write.pBufferInfo = buffers;
						break;
					}
				}
			}
		}
	}

	if ( !failed )
	{
		VK_WaitForGraphicsQueue();
		vkUpdateDescriptorSets( VK_GetDevice(), writes.size(), writes.data(), 0, nullptr );
	}

	// Free Memory
	for ( uint32_t i = 0; i < writes.size(); ++i )
	{
		VkWriteDescriptorSet& write = writes[ i ];

		if ( write.pImageInfo )
		{
			auto images = const_cast< VkDescriptorImageInfo* >( write.pImageInfo );
			free( images );
		}
		else if ( write.pBufferInfo )
		{
			auto buffers = const_cast< VkDescriptorBufferInfo* >( write.pBufferInfo );
			free( buffers );
		}
	}
}


// --------------------------------------------------------------------------------------------


VkDescriptorPool VK_GetDescPool()
{
	return gVkDescriptorPool;
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

