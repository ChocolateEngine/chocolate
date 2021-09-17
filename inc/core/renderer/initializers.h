/*
initializers.h ( Authored by p0lyh3dron )

Declares and defines many helper functions
that decrease the verbosity of allocation 
structures.
*/
#pragma once

#include <vulkan/vulkan.hpp>
/* Creates a structure specifying descriptor buffer information.  */
static inline VkDescriptorBufferInfo DescriptorBuffer( VkBuffer *spBuffers, unsigned int sRange, unsigned int sOffset, unsigned int sIndex )
{
	VkDescriptorBufferInfo 	bufferInfo{  };
	bufferInfo.buffer = spBuffers[ sIndex ];
	bufferInfo.offset = sOffset;
	bufferInfo.range  = sRange;

	return bufferInfo;
}
/* Creates a structure specifying descriptor image information.  */
static inline VkDescriptorImageInfo DescriptorImage( VkImageLayout sImageLayout, VkImageView sImageView, VkSampler sTextureSampler )
{
	VkDescriptorImageInfo 	imageInfo{  };
	imageInfo.imageLayout   = sImageLayout;
	imageInfo.imageView 	= sImageView;
	imageInfo.sampler 	= sTextureSampler;

	return imageInfo;
}
/* Creates a structure specifying a descriptor set layout binding.  */
static inline VkDescriptorSetLayoutBinding DescriptorLayoutBinding( VkDescriptorType sDescriptorType, unsigned int sDescriptorCount,
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
static inline VkImageMemoryBarrier ImageMemoryBarrier( VkImage sImage, VkImageLayout sOldLayout, VkImageLayout sNewLayout, uint32_t sMipLevels )
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
        barrier.subresourceRange.levelCount 	= sMipLevels;
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
static inline VkImageViewCreateInfo ImageView( VkImage sImage, VkFormat sFormat, VkImageAspectFlags sAspectFlags, uint32_t sMipLevels )
{
	VkImageViewCreateInfo viewInfo{  };
	viewInfo.sType 				 = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	viewInfo.image 				 = sImage;
	viewInfo.viewType 			 = VK_IMAGE_VIEW_TYPE_2D;
	viewInfo.format 			 = sFormat;
	viewInfo.subresourceRange.aspectMask 	 = sAspectFlags;
	viewInfo.subresourceRange.baseMipLevel 	 = 0;
	viewInfo.subresourceRange.levelCount 	 = sMipLevels;
	viewInfo.subresourceRange.baseArrayLayer = 0;
	viewInfo.subresourceRange.layerCount 	 = 1;

	return viewInfo;
}

static inline VkImageCreateInfo Image( uint32_t sWidth, uint32_t sHeight, VkFormat sFormat, VkImageTiling sTiling, VkImageUsageFlags sUsage, uint32_t sMipLevels )
{
	VkImageCreateInfo imageInfo{  };
	imageInfo.sType 	= VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageInfo.imageType 	= VK_IMAGE_TYPE_2D;
	imageInfo.extent.width 	= sWidth;
	imageInfo.extent.height = sHeight;
	imageInfo.extent.depth 	= 1; 
	imageInfo.mipLevels 	= sMipLevels;
	imageInfo.arrayLayers 	= 1;
	imageInfo.format 	= sFormat;
	imageInfo.tiling 	= sTiling;
	imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	imageInfo.usage	 	= sUsage;
	imageInfo.samples 	= VK_SAMPLE_COUNT_1_BIT;
	imageInfo.sharingMode 	= VK_SHARING_MODE_EXCLUSIVE;

	return imageInfo;
}
/* Creates a structure specifying the parameters of a descriptor set write operation.  */
static inline VkWriteDescriptorSet WriteDescriptor( VkDescriptorSet sDescSet, uint32_t sDstBinding, VkDescriptorType sDescType,
						    const VkDescriptorImageInfo *spImageInfo, const VkDescriptorBufferInfo *spBufferInfo )
{
	VkWriteDescriptorSet 	descriptorWrite{  };
	descriptorWrite.sType			= VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrite.dstSet			= sDescSet;
	descriptorWrite.dstBinding		= sDstBinding;
	descriptorWrite.dstArrayElement 	= 0;
	descriptorWrite.descriptorType  	= sDescType;
	descriptorWrite.descriptorCount 	= 1;
	
	if ( spImageInfo )
		descriptorWrite.pImageInfo	= spImageInfo;
	else if ( spBufferInfo )
		descriptorWrite.pBufferInfo	= spBufferInfo;

	return descriptorWrite;
}
/* Creates a structure specifying an attachment description.  */
static inline VkAttachmentDescription AttachmentDescription( VkFormat sFormat, VkAttachmentStoreOp sStoreOp, VkImageLayout sFinalLayout )
{
	VkAttachmentDescription attachment{  };
	attachment.format 		= sFormat;
	attachment.samples 		= VK_SAMPLE_COUNT_1_BIT;
	attachment.loadOp 		= VK_ATTACHMENT_LOAD_OP_CLEAR;
	attachment.storeOp 		= sStoreOp;
	attachment.stencilLoadOp 	= VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attachment.stencilStoreOp 	= VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachment.initialLayout 	= VK_IMAGE_LAYOUT_UNDEFINED;
	attachment.finalLayout 		= sFinalLayout;

	return attachment;
}
/* Creates a structure that is where thing draw.  */
/* Region of frambuffer to be rendered to, likely will always use 0, 0 and width, height.  */
static inline VkViewport Viewport( float x, float y, float width, float height, float minDepth, float maxDepth )
{
	VkViewport viewport = { x, height + y, width, height * -1.f, minDepth, maxDepth };

	return viewport;
}

static inline VkPipelineLayoutCreateInfo PipelineLayout( const VkDescriptorSetLayout *spSetLayouts, uint32_t sSetLayoutCount )
{
	VkPipelineLayoutCreateInfo pipelineLayout{  };
	pipelineLayout.sType 		= VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayout.setLayoutCount 	= sSetLayoutCount;
	pipelineLayout.pSetLayouts 	= spSetLayouts;

	return pipelineLayout;
}

static inline VkPushConstantRange PushConstantRange( VkShaderStageFlags sStageFlags, uint32_t sSize, uint32_t sOffset )
{
	VkPushConstantRange pushConstantRange{  };
	pushConstantRange.stageFlags 	= sStageFlags;
	pushConstantRange.offset 	= sOffset;
	pushConstantRange.size 		= sSize;

	return pushConstantRange;
}
