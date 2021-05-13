#ifndef ALLOCATOR_H
#define ALLOCATOR_H

#include "device.h"

class allocator_c
{
	private:

	VkFormat find_depth_format
		(  );

	public:

	void init_image_view
		( VkImageView& imageView, VkImage image, VkFormat format, VkImageAspectFlags aspectFlags );
	void init_image_views
		( std::vector< VkImage >& swapChainImages, std::vector< VkImageView >& swapChainImageViews, VkFormat& swapChainImageFormat );
	void init_render_pass
		( VkRenderPass& renderPass, VkFormat& swapChainImageFormat );
	void init_desc_set_layout
		( VkDescriptorSetLayout& descSetLayout );

	device_c dev;
};

#endif
