#pragma once

#include <vulkan/vulkan.hpp>

static inline VkDescriptorBufferInfo desc_buffer
	( std::vector< VkBuffer >& buffers, unsigned int range, unsigned int offset, unsigned int index )
{
	VkDescriptorBufferInfo bufferInfo{  };
	bufferInfo.buffer = buffers[ index ];
	bufferInfo.offset = offset;
	bufferInfo.range  = range;

	return bufferInfo;
}

static inline VkDescriptorImageInfo desc_image
	( VkImageLayout imageLayout, VkImageView imageView, VkSampler textureSampler )
{
	VkDescriptorImageInfo imageInfo{  };
	imageInfo.imageLayout   = imageLayout;
	imageInfo.imageView 	= imageView;
	imageInfo.sampler 	= textureSampler;

	return imageInfo;
}

static inline VkDescriptorSetLayoutBinding layout_binding
	( VkDescriptorType descriptorType, unsigned int descriptorCount, VkShaderStageFlags stageFlags, const VkSampler* pImmutableSamplers )
{
	VkDescriptorSetLayoutBinding layoutBinding{  };
	layoutBinding.descriptorCount 	 = descriptorCount;
	layoutBinding.descriptorType 	 = descriptorType;
	layoutBinding.pImmutableSamplers = pImmutableSamplers;
	layoutBinding.stageFlags 	 = stageFlags;

	return layoutBinding;
}
