#ifndef RENDERER_H
#define RENDERER_H

#include "system.h"

#include <vulkan/vulkan.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_vulkan.h>

#ifdef NDEBUG
    const bool enableValidationLayers = false;
#else
    const bool enableValidationLayers = true;
#endif

const std::vector<const char*> validationLayers = {
    "VK_LAYER_KHRONOS_validation"
};

const std::vector<const char*> deviceExtensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME
};


class renderer_c : public system_c
{
	protected:

	int width = 1280, height = 720;

	SDL_Window* win;
	VkSurfaceKHR surf;
	VkSwapchainKHR swapChain;
	
	VkInstance inst;
	
	VkPhysicalDevice physicalDevice;
	VkDevice device;
	
	VkQueue graphicsQueue, presentQueue;
	
	std::vector<VkImageView> swapChainImageViews;
	std::vector<VkImage> swapChainImages;
	std::vector<VkFramebuffer> swapChainFramebuffers;
	std::vector<VkCommandBuffer> commandBuffers;
	
	VkFormat swapChainImageFormat;
	VkExtent2D swapChainExtent;
	VkRenderPass renderPass;
	
	VkPipelineLayout pipelineLayout;
	VkPipeline graphicsPipeline;
	VkCommandPool commandPool;
	
	VkSemaphore imageAvailableSemaphore, renderFinishedSemaphore;

	void init_vulkan
		(  );
	void init_window
		(  );
	void init_instance
		(  );

	bool check_validation_layer_support
		(  );
	
	void cleanup
		(  );
	
	public:

	renderer_c
		(  );
	~renderer_c
		(  );
};

#endif
