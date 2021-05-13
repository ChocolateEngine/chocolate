#include "../../../inc/core/renderer/allocator.h"

#include <array>

VkFormat allocator_c::find_depth_format
	(  )
{
	return dev.find_supported_fmt(
		{ VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
		VK_IMAGE_TILING_OPTIMAL,
        	VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT );
}

void allocator_c::init_image_view
	( VkImageView& imageView, VkImage image, VkFormat format, VkImageAspectFlags aspectFlags )
{
	VkImageViewCreateInfo viewInfo{  };
	viewInfo.sType 				 = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	viewInfo.image 				 = image;
	viewInfo.viewType 			 = VK_IMAGE_VIEW_TYPE_2D;
	viewInfo.format 			 = format;
	viewInfo.subresourceRange.aspectMask 	 = aspectFlags;
	viewInfo.subresourceRange.baseMipLevel 	 = 0;
	viewInfo.subresourceRange.levelCount 	 = 1;
	viewInfo.subresourceRange.baseArrayLayer = 0;
	viewInfo.subresourceRange.layerCount 	 = 1;

	if ( vkCreateImageView( dev.get(  ), &viewInfo, NULL, &imageView ) != VK_SUCCESS )
	{
		throw std::runtime_error( "Failed to create texture image view!" );
	}
}

void allocator_c::init_image_views
	( std::vector< VkImage >& swapChainImages, std::vector< VkImageView >& swapChainImageViews, VkFormat& swapChainImageFormat )
{
	swapChainImageViews.resize( swapChainImages.size(  ) );
	for ( int i = 0; i < swapChainImages.size(  ); i++ )
	{
		init_image_view( swapChainImageViews[ i ], swapChainImages[ i ], swapChainImageFormat, VK_IMAGE_ASPECT_COLOR_BIT );
	}
}

void allocator_c::init_render_pass
	( VkRenderPass& renderPass, VkFormat& swapChainImageFormat )
{
	VkAttachmentDescription colorAttachment{  };
	colorAttachment.format 		= swapChainImageFormat;
	colorAttachment.samples 	= VK_SAMPLE_COUNT_1_BIT;
	colorAttachment.loadOp 		= VK_ATTACHMENT_LOAD_OP_CLEAR;
	colorAttachment.storeOp 	= VK_ATTACHMENT_STORE_OP_STORE;
	colorAttachment.stencilLoadOp 	= VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachment.stencilStoreOp 	= VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colorAttachment.initialLayout 	= VK_IMAGE_LAYOUT_UNDEFINED;
	colorAttachment.finalLayout 	= VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	VkAttachmentDescription depthAttachment{  };
	depthAttachment.format 		= find_depth_format(  );
	depthAttachment.samples 	= VK_SAMPLE_COUNT_1_BIT;
	depthAttachment.loadOp 		= VK_ATTACHMENT_LOAD_OP_CLEAR;
	depthAttachment.storeOp 	= VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttachment.stencilLoadOp 	= VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	depthAttachment.stencilStoreOp 	= VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttachment.initialLayout 	= VK_IMAGE_LAYOUT_UNDEFINED;
	depthAttachment.finalLayout 	= VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkAttachmentReference colorAttachmentRef{  };
	colorAttachmentRef.attachment 	= 0;	//	Referenced by index, so 0 means color is referenced first since type is ambiguous
	colorAttachmentRef.layout 	= VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkAttachmentReference depthAttachmentRef{  };
	depthAttachmentRef.attachment 	= 1;
	depthAttachmentRef.layout 	= VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpass{  };	//	Smaller render operations, store them here
	subpass.pipelineBindPoint 	= VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount 	= 1;
	subpass.pColorAttachments 	= &colorAttachmentRef;
	subpass.pDepthStencilAttachment = &depthAttachmentRef;

	VkSubpassDependency dependency{  };
	dependency.srcSubpass 		= VK_SUBPASS_EXTERNAL;
	dependency.dstSubpass 		= 0;
	dependency.srcStageMask 	= VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	dependency.srcAccessMask 	= 0;
	dependency.dstStageMask 	= VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	dependency.dstAccessMask 	= VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

	std::array< VkAttachmentDescription, 2 > attachments = { colorAttachment, depthAttachment };
	VkRenderPassCreateInfo renderPassInfo{  };
	renderPassInfo.sType 		= VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassInfo.attachmentCount 	= ( uint32_t )attachments.size(  );
	renderPassInfo.pAttachments 	= attachments.data(  );
	renderPassInfo.subpassCount	= 1;
	renderPassInfo.pSubpasses 	= &subpass;
	renderPassInfo.dependencyCount 	= 1;
	renderPassInfo.pDependencies 	= &dependency;

	if ( vkCreateRenderPass( dev.get(  ), &renderPassInfo, NULL, &renderPass ) != VK_SUCCESS )
	{
		throw std::runtime_error( "Failed to create render pass!" );
	}
}

void allocator_c::init_desc_set_layout
	( VkDescriptorSetLayout& descSetLayout )
{
	VkDescriptorSetLayoutBinding uboLayoutBinding{  };
	uboLayoutBinding.binding 		= 0;
	uboLayoutBinding.descriptorType 	= VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	uboLayoutBinding.descriptorCount 	= 1;
	uboLayoutBinding.stageFlags 		= VK_SHADER_STAGE_VERTEX_BIT;
	uboLayoutBinding.pImmutableSamplers 	= NULL; // Optional

	VkDescriptorSetLayoutBinding samplerLayoutBinding{  };
	samplerLayoutBinding.binding 		= 1;
	samplerLayoutBinding.descriptorCount 	= 1;
	samplerLayoutBinding.descriptorType 	= VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	samplerLayoutBinding.pImmutableSamplers = NULL;
	samplerLayoutBinding.stageFlags 	= VK_SHADER_STAGE_FRAGMENT_BIT;

	std::array< VkDescriptorSetLayoutBinding, 2 > bindings = { uboLayoutBinding, samplerLayoutBinding };
	VkDescriptorSetLayoutCreateInfo layoutInfo{  };
	layoutInfo.sType 			= VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.bindingCount 		= ( uint32_t )bindings.size(  );
	layoutInfo.pBindings 			= bindings.data(  );

	if ( vkCreateDescriptorSetLayout( dev.get(  ), &layoutInfo, NULL, &descSetLayout ) != VK_SUCCESS )
	{
		throw std::runtime_error( "Failed to create descriptor set layout!" );
	}
}
