/*
initializers.h ( Authored by p0lyh3dron )

Declares and defines many helper functions
that decrease the verbosity of allocation 
structures.
*/
#pragma once

#include <vulkan/vulkan.hpp>
/* Creates a structure specifying descriptor buffer information.  */
static inline VkDescriptorBufferInfo DescriptorBuffer
	( std::vector< VkBuffer > &srBuffers, unsigned int sRange, unsigned int sOffset, unsigned int sIndex )
{
	VkDescriptorBufferInfo 	bufferInfo{  };
	bufferInfo.buffer = srBuffers[ sIndex ];
	bufferInfo.offset = sOffset;
	bufferInfo.range  = sRange;

	return bufferInfo;
}
/* Creates a structure specifying descriptor image information.  */
static inline VkDescriptorImageInfo DescriptorImage
	( VkImageLayout sImageLayout, VkImageView sImageView, VkSampler sTextureSampler )
{
	VkDescriptorImageInfo 	imageInfo{  };
	imageInfo.imageLayout   = sImageLayout;
	imageInfo.imageView 	= sImageView;
	imageInfo.sampler 	= sTextureSampler;

	return imageInfo;
}
/* Creates a structure specifying a descriptor set layout binding.  */
static inline VkDescriptorSetLayoutBinding DescriptorLayoutBinding( VkDescriptorType sDescriptorType, unsigned int sDescriptorCount, \
							   	    VkShaderStageFlags sStageFlags, const VkSampler *spImmutableSamplers )
{
	VkDescriptorSetLayoutBinding 	layoutBinding{  };
	layoutBinding.descriptorCount 	 = sDescriptorCount;
	layoutBinding.descriptorType 	 = sDescriptorType;
	layoutBinding.pImmutableSamplers = spImmutableSamplers;
	layoutBinding.stageFlags 	 = sStageFlags;

	return layoutBinding;
}
/* Creates a structure specifying the parameters of an image memory barrier.  */
static inline VkImageMemoryBarrier ImageMemoryBarrier( VkImage sImage, VkImageLayout sOldLayout, VkImageLayout sNewLayout )
{
	VkImageMemoryBarrier 	barrier{  };
        barrier.sType 				= VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.oldLayout 			= sOldLayout;
        barrier.newLayout 			= sNewLayout;
        barrier.srcQueueFamilyIndex 		= VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex 		= VK_QUEUE_FAMILY_IGNORED;
        barrier.image 				= sImage;
        barrier.subresourceRange.aspectMask 	= VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.baseMipLevel 	= 0;
        barrier.subresourceRange.levelCount 	= 1;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount 	= 1;

	return barrier;
}
/* Creates a structure specifying the parameters of a newly created buffer object.  */
static inline VkBufferCreateInfo BufferCreate( VkDeviceSize sSize, VkBufferUsageFlags sUsage )
{
	VkBufferCreateInfo 	bufferInfo{  };
	bufferInfo.sType 	= VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.size 	= sSize;
	bufferInfo.usage 	= sUsage;
	bufferInfo.sharingMode  = VK_SHARING_MODE_EXCLUSIVE;

	return bufferInfo;
}
/* Create a structure containing parameters of a memory allocation.  */
static inline VkMemoryAllocateInfo MemoryAllocate( VkDeviceSize sSize, uint32_t sMemoryTypeIndex )
{
	VkMemoryAllocateInfo	allocInfo{  };
	allocInfo.sType 		= VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize 	= sSize;
	allocInfo.memoryTypeIndex 	= sMemoryTypeIndex;

	return allocInfo;
}
/* Create a structure specifying a buffer image copy operation.  */
static inline VkBufferImageCopy BufferImageCopy( uint32_t sWidth, uint32_t sHeight )
{
	VkBufferImageCopy region{  };
	region.bufferOffset 		= 0;
	region.bufferRowLength 		= 0;
	region.bufferImageHeight 	= 0;

	region.imageSubresource.aspectMask 	= VK_IMAGE_ASPECT_COLOR_BIT;
	region.imageSubresource.mipLevel 	= 0;
	region.imageSubresource.baseArrayLayer  = 0;
	region.imageSubresource.layerCount 	= 1;

	region.imageOffset = { 0, 0, 0 };
	region.imageExtent = { sWidth, sHeight, 1 };

	return region;
}
/* Create a structure specifying parameters of a newly created image view.  */
static inline VkImageViewCreateInfo ImageView( VkImage sImage, VkFormat sFormat, VkImageAspectFlags sAspectFlags )
{
	VkImageViewCreateInfo viewInfo{  };
	viewInfo.sType 				 = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	viewInfo.image 				 = sImage;
	viewInfo.viewType 			 = VK_IMAGE_VIEW_TYPE_2D;
	viewInfo.format 			 = sFormat;
	viewInfo.subresourceRange.aspectMask 	 = sAspectFlags;
	viewInfo.subresourceRange.baseMipLevel 	 = 0;
	viewInfo.subresourceRange.levelCount 	 = 1;
	viewInfo.subresourceRange.baseArrayLayer = 0;
	viewInfo.subresourceRange.layerCount 	 = 1;

	return viewInfo;
}

static inline VkImageCreateInfo Image( uint32_t sWidth, uint32_t sHeight, VkFormat sFormat, VkImageTiling sTiling, VkImageUsageFlags sUsage )
{
	VkImageCreateInfo imageInfo{  };
	imageInfo.sType 	= VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageInfo.imageType 	= VK_IMAGE_TYPE_2D;
	imageInfo.extent.width 	= sWidth;
	imageInfo.extent.height = sHeight;
	imageInfo.extent.depth 	= 1;
	imageInfo.mipLevels 	= 1;
	imageInfo.arrayLayers 	= 1;
	imageInfo.format 	= sFormat;
	imageInfo.tiling 	= sTiling;
	imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	imageInfo.usage	 	= sUsage;
	imageInfo.samples 	= VK_SAMPLE_COUNT_1_BIT;
	imageInfo.sharingMode 	= VK_SHARING_MODE_EXCLUSIVE;
}
