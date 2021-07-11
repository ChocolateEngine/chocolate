#pragma once

#include "allocator.h"

#include <vulkan/vulkan.hpp>
#include <functional>

class gui_renderer_c
{
  protected:
	
	VkSampler sampler;
	VkBuffer vertexBuffer, indexBuffer;
	uint32_t vertexCount, indexCount;
	VkDeviceMemory fontMemory = VK_NULL_HANDLE;
	VkImage fontImage = VK_NULL_HANDLE;
	VkImageView fontView = VK_NULL_HANDLE;
	VkPipelineCache piplineCache;
	VkPipelineLayout pipelineLayout;
	VkPipeline pipeline;
	VkDescriptorPool descriptorPool;
	VkDescriptorSetLayout descriptorSetLayout;
	VkDescriptorSet descriptorSet;

	void init_pipeline
		(  );

  public:
	
	device_c* device;
	allocator_c allocator;

	void init
		(  );

};
