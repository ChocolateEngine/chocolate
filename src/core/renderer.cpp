#include "../../inc/core/renderer.h"

#include <set>
#include <fstream>

void renderer_c::init_vulkan
	(  )
{
	init_window(  );
	init_instance(  );
	init_surface(  );
	pick_physical_device(  );
	init_logical_device(  );
	init_swap_chain(  );
	init_image_views(  );
	init_render_pass(  );
	init_graphics_pipeline(  );
}

void renderer_c::init_window
	(  )
{
	if ( SDL_Init( SDL_INIT_VIDEO | SDL_INIT_EVENTS ) != 0 )
	{
		throw std::runtime_error( "Unable to initialize SDL2!" );
	}
	win = SDL_CreateWindow(
		" - Chocolate Engine ( build: too early for version lol )",	//	Window Title
		SDL_WINDOWPOS_CENTERED,						//	X Pos
		SDL_WINDOWPOS_CENTERED,
		width,
		height,
		SDL_WINDOW_VULKAN | SDL_WINDOW_SHOWN
                );
}

void renderer_c::init_instance
	(  )
{
	if ( enableValidationLayers && !check_validation_layer_support(  ) )
	{
		throw std::runtime_error( "Validation layers requested, but not available!" );
	}
	VkApplicationInfo appInfo{  };
	appInfo.sType 			= VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pApplicationName 	= "Test";
	appInfo.applicationVersion 	= VK_MAKE_VERSION( 1, 0, 0 );
	appInfo.pEngineName 		= "Chocolate";
	appInfo.engineVersion 		= VK_MAKE_VERSION( 1, 0, 0 );
	appInfo.apiVersion 		= VK_API_VERSION_1_0;

	VkInstanceCreateInfo createInfo{  };
	createInfo.sType 		= VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	createInfo.pApplicationInfo 	= &appInfo;

	if ( enableValidationLayers )
	{
		createInfo.enabledLayerCount 	= ( unsigned int )validationLayers.size(  );
		createInfo.ppEnabledLayerNames 	= validationLayers.data(  );
	}
	else
	{
		createInfo.enabledLayerCount 	= 0;
	}

	unsigned int SDLExtensionCount = 0;
	
	if ( !SDL_Vulkan_GetInstanceExtensions( win, &SDLExtensionCount, NULL ) )
	{
		throw std::runtime_error( "Unable to get number of Vulkan instance extensions!" );
	}
	
	std::vector< const char* > SDLExtensions( SDLExtensionCount );
	
	if ( !SDL_Vulkan_GetInstanceExtensions( win, &SDLExtensionCount, SDLExtensions.data(  ) ) )
	{
		throw std::runtime_error( "Unable to get Vulkan instance extension names!" );
	}
	
	printf( "%d SDL extensions available:\n", SDLExtensionCount );
	
	for ( int i = 0; i < SDLExtensionCount; ++i )
	{
		printf( "\t%s\n", SDLExtensions[ i ] );
	}

	createInfo.enabledExtensionCount 	= SDLExtensionCount;
	createInfo.ppEnabledExtensionNames 	= SDLExtensions.data(  );
	createInfo.enabledLayerCount 		= 0;

	if ( vkCreateInstance( &createInfo, NULL, &inst ) )
	{
		throw std::runtime_error( "Failed to create Vulkan instance!" );
	}

	unsigned int extensionCount = 0;
	vkEnumerateInstanceExtensionProperties( NULL, &extensionCount, NULL );
	
	std::vector< VkExtensionProperties > extensions( extensionCount );
	vkEnumerateInstanceExtensionProperties( NULL, &extensionCount, extensions.data(  ) );

	printf( "%d Vulkan extensions available:\n", extensionCount );

	for ( const auto& extension : extensions )
	{
		printf( "\t%s\n", extension.extensionName );
	}
}

void renderer_c::init_surface
	(  )
{
	if ( !SDL_Vulkan_CreateSurface( win, inst, &surf ) )
	{
		throw std::runtime_error( "Failed to create Vulkan Surface!" );
	}
}

bool renderer_c::check_validation_layer_support
	(  )
{
	bool layerFound;
	unsigned int layerCount;
	vkEnumerateInstanceLayerProperties( &layerCount, NULL );

	std::vector< VkLayerProperties > availableLayers( layerCount );
	vkEnumerateInstanceLayerProperties( &layerCount, availableLayers.data(  ) );
	
	for ( const char* layerName : validationLayers )
	{
		layerFound = false;

		for ( const auto& layerProperties : availableLayers )
		{
			if ( strcmp( layerName, layerProperties.layerName ) == 0 )
			{
				layerFound = true;
				break;
			}
		}

		if ( !layerFound )
		{
			return false;
		}
	}
	return true;
}

void renderer_c::pick_physical_device
	(  )
{
	physicalDevice 		= VK_NULL_HANDLE;
	uint32_t deviceCount 	= 0;
	
	vkEnumeratePhysicalDevices( inst, &deviceCount, NULL );
	if ( deviceCount == 0 ) {
		throw std::runtime_error( "Failed to find GPUs with Vulkan support!" );
	}
	std::vector< VkPhysicalDevice > devices( deviceCount );
	vkEnumeratePhysicalDevices( inst, &deviceCount, devices.data(  ) );

	for ( const auto& device : devices ) {
		if ( is_suitable_device( device ) ) {
			physicalDevice = device;
			break;
		}
	}

	if ( physicalDevice == VK_NULL_HANDLE ) {
		throw std::runtime_error( "Failed to find a suitable GPU!" );
	}	
}

bool renderer_c::is_suitable_device
	( VkPhysicalDevice d )
{
	queue_family_indices_t indices 	= find_queue_families( d );
	bool extensionsSupported 	= check_device_extension_support( d );
	bool swapChainAdequate 		= false;
	if ( extensionsSupported )
	{
		swap_chain_support_info_t swapChainSupport = check_swap_chain_support( d );
		swapChainAdequate = !swapChainSupport.f.empty(  ) && !swapChainSupport.p.empty(  );
	}
	return indices.complete(  ) && extensionsSupported && swapChainAdequate;
}

bool renderer_c::check_device_extension_support
	( VkPhysicalDevice d )
{
	unsigned int extensionCount;
	vkEnumerateDeviceExtensionProperties( d, NULL, &extensionCount, NULL );

	std::vector< VkExtensionProperties > availableExtensions( extensionCount );
	vkEnumerateDeviceExtensionProperties( d, NULL, &extensionCount, availableExtensions.data(  ) );

	std::set< std::string > requiredExtensions(  deviceExtensions.begin(  ), deviceExtensions.end(  ) );

	for ( const auto& extension : availableExtensions )
	{
		requiredExtensions.erase( extension.extensionName  );
	}
	return requiredExtensions.empty(  );
}

queue_family_indices_t renderer_c::find_queue_families
	( VkPhysicalDevice d )
{
	queue_family_indices_t indices;
	uint32_t queueFamilyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties( d, &queueFamilyCount, nullptr );

	std::vector< VkQueueFamilyProperties > queueFamilies( queueFamilyCount );
	vkGetPhysicalDeviceQueueFamilyProperties( d, &queueFamilyCount, queueFamilies.data(  ) );// Logic to find queue family indices to populate struct with
	int i = 0;
	for ( const auto& queueFamily : queueFamilies ) {
		if ( queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT ) {
			VkBool32 presentSupport = false;
			vkGetPhysicalDeviceSurfaceSupportKHR( d, i, surf, &presentSupport );
			if ( presentSupport ) {
				indices.presentFamily = i;
			}
			indices.graphicsFamily = i;
		}
		if ( indices.complete(  ) ) {
			break;
		}
		i++;
	}
	return indices;
}

swap_chain_support_info_t renderer_c::check_swap_chain_support
	( VkPhysicalDevice d )
{
	swap_chain_support_info_t details;
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR( d, surf, &details.c );

	uint32_t formatCount;
	vkGetPhysicalDeviceSurfaceFormatsKHR( d, surf, &formatCount, NULL );

	if ( formatCount != 0 )
	{
		details.f.resize( formatCount );
		vkGetPhysicalDeviceSurfaceFormatsKHR( d, surf, &formatCount, details.f.data(  ) );
	}
	uint32_t presentModeCount;
	vkGetPhysicalDeviceSurfacePresentModesKHR( d, surf, &presentModeCount, NULL );

	if ( presentModeCount != 0 )
	{
		details.p.resize( presentModeCount );
		vkGetPhysicalDeviceSurfacePresentModesKHR( d, surf, &presentModeCount, details.p.data(  ) );
	}
	return details;
}

void renderer_c::init_logical_device
	(  )
{
	queue_family_indices_t indices = find_queue_families( physicalDevice );

	std::vector< VkDeviceQueueCreateInfo > queueCreateInfos;
	std::set< uint32_t > uniqueQueueFamilies = { indices.graphicsFamily.value(  ), indices.presentFamily.value(  ) };

	float queuePriority = 1.0f;
	for ( uint32_t queueFamily : uniqueQueueFamilies )
	{
		VkDeviceQueueCreateInfo queueCreateInfo{  };
		
		queueCreateInfo.sType			= VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueCreateInfo.queueFamilyIndex 	= queueFamily;
		queueCreateInfo.queueCount		= 1;
		queueCreateInfo.pQueuePriorities 	= &queuePriority;
		
		queueCreateInfos.push_back( queueCreateInfo );
	}

	VkPhysicalDeviceFeatures deviceFeatures{  };

	VkDeviceCreateInfo createInfo{  };
	createInfo.sType 			= VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	createInfo.pQueueCreateInfos 		= queueCreateInfos.data(  );
	createInfo.queueCreateInfoCount 	= ( uint32_t )queueCreateInfos.size(  );

	createInfo.pEnabledFeatures 		= &deviceFeatures;

	createInfo.enabledExtensionCount 	= ( uint32_t )( deviceExtensions.size(  ) );
	createInfo.ppEnabledExtensionNames 	= deviceExtensions.data(  );

	if ( enableValidationLayers )
	{
		createInfo.enabledLayerCount 	= ( uint32_t )( validationLayers.size(  ) );
		createInfo.ppEnabledLayerNames 	= validationLayers.data(  );
	}
	else
	{
		createInfo.enabledLayerCount 	= 0;
	}
	if ( vkCreateDevice( physicalDevice, &createInfo, NULL, &device ) != VK_SUCCESS )
	{
		throw std::runtime_error( "Failed to create logical device!" );
	}
	vkGetDeviceQueue( device, indices.graphicsFamily.value(  ), 0, &graphicsQueue );
	vkGetDeviceQueue( device, indices.presentFamily.value(  ), 0, &presentQueue );
}

VkSurfaceFormatKHR renderer_c::choose_swap_surface_format
	( const std::vector< VkSurfaceFormatKHR >& availableFormats )
{
	for ( const auto& availableFormat : availableFormats )
	{
		if ( availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR )
		{
			return availableFormat;
		}
	}
	return availableFormats[ 0 ];
}

VkPresentModeKHR renderer_c::choose_swap_present_mode
	( const std::vector< VkPresentModeKHR >& availablePresentModes )
{
	for ( const auto& availablePresentMode : availablePresentModes )
	{
		if ( availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR )
		{
			return availablePresentMode;
		}
	}
	return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D renderer_c::choose_swap_extent
	( const VkSurfaceCapabilitiesKHR& capabilities )
{
	if ( capabilities.currentExtent.width != UINT32_MAX )
	{
		return capabilities.currentExtent;
	}
	else
	{
		VkExtent2D size = {
			( unsigned int )width,
			( unsigned int )height
		};

	       size.width = std::max(capabilities.minImageExtent.width, std::min(capabilities.maxImageExtent.width, size.width));
	       size.height = std::max(capabilities.minImageExtent.height, std::min(capabilities.maxImageExtent.height, size.height));

		return size;
	}
}

void renderer_c::init_swap_chain
	(  )
{
	swap_chain_support_info_t swapChainSupport 	= check_swap_chain_support( physicalDevice );

	VkSurfaceFormatKHR surfaceFormat 		= choose_swap_surface_format( swapChainSupport.f );
	VkPresentModeKHR presentMode 			= choose_swap_present_mode( swapChainSupport.p );
	VkExtent2D extent 				= choose_swap_extent( swapChainSupport.c );

	uint32_t imageCount 				= swapChainSupport.c.minImageCount + 1;
	if ( swapChainSupport.c.maxImageCount > 0 && imageCount > swapChainSupport.c.maxImageCount )
	{
		imageCount = swapChainSupport.c.maxImageCount;
	}

	VkSwapchainCreateInfoKHR createInfo{  };
	createInfo.sType 		= VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	createInfo.surface 		= surf;

	createInfo.minImageCount 	= imageCount;
	createInfo.imageFormat 		= surfaceFormat.format;
	createInfo.imageColorSpace 	= surfaceFormat.colorSpace;
	createInfo.imageExtent 		= extent;
	createInfo.imageArrayLayers	= 1;
	createInfo.imageUsage		= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

        queue_family_indices_t indices 	= find_queue_families( physicalDevice );
	uint32_t queueFamilyIndices[  ] = { indices.graphicsFamily.value(  ), indices.presentFamily.value(  ) };

	if ( indices.graphicsFamily != indices.presentFamily )
	{
		createInfo.imageSharingMode 		= VK_SHARING_MODE_CONCURRENT;
		createInfo.queueFamilyIndexCount 	= 2;
		createInfo.pQueueFamilyIndices 		= queueFamilyIndices;
	}
	else
	{
		createInfo.imageSharingMode 		= VK_SHARING_MODE_EXCLUSIVE;
		createInfo.queueFamilyIndexCount 	= 0; // Optional
		createInfo.pQueueFamilyIndices 		= NULL; // Optional
	}

	createInfo.preTransform 	= swapChainSupport.c.currentTransform;
	createInfo.compositeAlpha 	= VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	createInfo.presentMode 		= presentMode;
	createInfo.clipped		= VK_TRUE;
	createInfo.oldSwapchain 	= VK_NULL_HANDLE;

	if ( vkCreateSwapchainKHR(device, &createInfo, NULL, &swapChain) != VK_SUCCESS ) {
		throw std::runtime_error( "Failed to create swap chain!" );
	}
	vkGetSwapchainImagesKHR( device, swapChain, &imageCount, NULL );
	swapChainImages.resize( imageCount );
	vkGetSwapchainImagesKHR( device, swapChain, &imageCount, swapChainImages.data(  ) );

	swapChainImageFormat 	= surfaceFormat.format;
	swapChainExtent 	= extent;
}

void renderer_c::init_image_views
	(  )
{
	swapChainImageViews.resize( swapChainImages.size(  ) );
	for ( int i = 0; i < swapChainImages.size(  ); i++ )
	{
		VkImageViewCreateInfo createInfo{  };
		createInfo.sType 				= VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		createInfo.image 				= swapChainImages[ i ];
		createInfo.viewType 				= VK_IMAGE_VIEW_TYPE_2D;
		createInfo.format 				= swapChainImageFormat;
		createInfo.components.r 			= VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.g 			= VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.b 			= VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.a 			= VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.subresourceRange.aspectMask 		= VK_IMAGE_ASPECT_COLOR_BIT;
		createInfo.subresourceRange.baseMipLevel 	= 0;
		createInfo.subresourceRange.levelCount 		= 1;
		createInfo.subresourceRange.baseArrayLayer	= 0;
		createInfo.subresourceRange.layerCount 		= 1;
		if ( vkCreateImageView( device, &createInfo, NULL, &swapChainImageViews[ i ] ) != VK_SUCCESS )
		{
			throw std::runtime_error( "Failed to create image views!" );
		}
	}
}

void renderer_c::init_render_pass
	(  )
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

	VkAttachmentReference colorAttachmentRef{  };
	colorAttachmentRef.attachment 	= 0;
	colorAttachmentRef.layout 	= VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpass{  };
	subpass.pipelineBindPoint 	= VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount 	= 1;
	subpass.pColorAttachments 	= &colorAttachmentRef;

	VkSubpassDependency dependency{  };
	dependency.srcSubpass 		= VK_SUBPASS_EXTERNAL;
	dependency.dstSubpass 		= 0;
	dependency.srcStageMask 	= VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.srcAccessMask 	= 0;
	dependency.dstStageMask 	= VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.dstAccessMask 	= VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

	VkRenderPassCreateInfo renderPassInfo{  };
	renderPassInfo.sType 		= VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassInfo.attachmentCount 	= 1;
	renderPassInfo.pAttachments 	= &colorAttachment;
	renderPassInfo.subpassCount	= 1;
	renderPassInfo.pSubpasses 	= &subpass;
	renderPassInfo.dependencyCount 	= 1;
	renderPassInfo.pDependencies 	= &dependency;

	if ( vkCreateRenderPass( device, &renderPassInfo, nullptr, &renderPass ) != VK_SUCCESS )
	{
		throw std::runtime_error( "Failed to create render pass!" );
	}
}

std::vector< char > renderer_c::read_file
	( const std::string& filePath )
{
	std::ifstream file( filePath, std::ios::ate | std::ios::binary );
	if ( !file.is_open(  ) ) {
		throw std::runtime_error( "Failed to open file!" );
	}
	int fileSize = ( int ) file.tellg(  );
	std::vector< char > buffer( fileSize );
	file.seekg( 0 );
	file.read( buffer.data(  ), fileSize );
	file.close(  );

	return buffer;
}

VkShaderModule renderer_c::create_shader_module
	( const std::vector< char >& code )
{
	VkShaderModuleCreateInfo createInfo{  };
	VkShaderModule shaderModule;
	createInfo.sType 	= VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	createInfo.codeSize 	= code.size(  );
	createInfo.pCode 	= ( const uint32_t* )code.data(  );
	if ( vkCreateShaderModule( device, &createInfo, NULL, &shaderModule ) != VK_SUCCESS )
	{
		throw std::runtime_error( "Failed to create shader module!" );
	}
	return shaderModule;
}

void renderer_c::init_graphics_pipeline
	(  )
{
	auto vertShaderCode = read_file( "shaders/vert.spv" );
	auto fragShaderCode = read_file( "shaders/frag.spv" );
		
	VkShaderModule vertShaderModule = create_shader_module( vertShaderCode );
	VkShaderModule fragShaderModule = create_shader_module( fragShaderCode );

	VkPipelineShaderStageCreateInfo vertShaderStageInfo{  };
	vertShaderStageInfo.sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	vertShaderStageInfo.stage  = VK_SHADER_STAGE_VERTEX_BIT;
	vertShaderStageInfo.module = vertShaderModule;
	vertShaderStageInfo.pName  = "main";

	VkPipelineShaderStageCreateInfo fragShaderStageInfo{  };
	fragShaderStageInfo.sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	fragShaderStageInfo.stage  = VK_SHADER_STAGE_FRAGMENT_BIT;
	fragShaderStageInfo.module = fragShaderModule;
	fragShaderStageInfo.pName  = "main";

	VkPipelineShaderStageCreateInfo shaderStages[  ] = { vertShaderStageInfo, fragShaderStageInfo };

	VkPipelineVertexInputStateCreateInfo vertexInputInfo{  };
	vertexInputInfo.sType 				= VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertexInputInfo.vertexBindingDescriptionCount 	= 0;
	vertexInputInfo.pVertexBindingDescriptions 	= NULL; // Optional
	vertexInputInfo.vertexAttributeDescriptionCount = 0;
	vertexInputInfo.pVertexAttributeDescriptions 	= NULL; // Optional

	VkPipelineInputAssemblyStateCreateInfo inputAssembly{  };
	inputAssembly.sType 			= VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssembly.topology 			= VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	inputAssembly.primitiveRestartEnable 	= VK_FALSE;

	VkViewport viewport{  };
	viewport.x		= 0.0f;
	viewport.y 		= 0.0f;
	viewport.width 		= ( float )swapChainExtent.width;
	viewport.height 	= ( float )swapChainExtent.height;
	viewport.minDepth 	= 0.0f;
	viewport.maxDepth 	= 1.0f;

	VkRect2D scissor{  };
	scissor.offset = { 0, 0 };
	scissor.extent = swapChainExtent;

	VkPipelineViewportStateCreateInfo viewportState{  };
	viewportState.sType 		= VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportState.viewportCount 	= 1;
	viewportState.pViewports 	= &viewport;
	viewportState.scissorCount 	= 1;
	viewportState.pScissors 	= &scissor;

	VkPipelineRasterizationStateCreateInfo rasterizer{  };
	rasterizer.sType 			= VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizer.depthClampEnable 		= VK_FALSE;
	rasterizer.rasterizerDiscardEnable 	= VK_FALSE;
	rasterizer.polygonMode 			= VK_POLYGON_MODE_FILL;
	rasterizer.lineWidth 			= 1.0f;
	rasterizer.cullMode 			= VK_CULL_MODE_BACK_BIT;
	rasterizer.frontFace 			= VK_FRONT_FACE_CLOCKWISE;
	rasterizer.depthBiasEnable 		= VK_FALSE;
	rasterizer.depthBiasConstantFactor 	= 0.0f; // Optional
	rasterizer.depthBiasClamp 		= 0.0f; // Optional
	rasterizer.depthBiasSlopeFactor 	= 0.0f; // Optional

	VkPipelineMultisampleStateCreateInfo multisampling{  };
	multisampling.sType 		    = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampling.sampleShadingEnable   = VK_FALSE;
	multisampling.rasterizationSamples  = VK_SAMPLE_COUNT_1_BIT;
	multisampling.minSampleShading 	    = 1.0f; // Optional
	multisampling.pSampleMask 	    = NULL; // Optional
	multisampling.alphaToCoverageEnable = VK_FALSE; // Optional
	multisampling.alphaToOneEnable 	    = VK_FALSE; // Optional

	VkPipelineColorBlendAttachmentState colorBlendAttachment{  };
	colorBlendAttachment.colorWriteMask 	 = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	colorBlendAttachment.blendEnable 	 = VK_TRUE;
	colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
	colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	colorBlendAttachment.colorBlendOp	 = VK_BLEND_OP_ADD;
	colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	colorBlendAttachment.alphaBlendOp 	 = VK_BLEND_OP_ADD;

	VkPipelineColorBlendStateCreateInfo colorBlending{  };
	colorBlending.sType 		  = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlending.logicOpEnable 	  = VK_FALSE;
	colorBlending.logicOp 	          = VK_LOGIC_OP_COPY; // Optional
	colorBlending.attachmentCount 	  = 1;
	colorBlending.pAttachments 	  = &colorBlendAttachment;
	colorBlending.blendConstants[ 0 ] = 0.0f; // Optional
	colorBlending.blendConstants[ 1 ] = 0.0f; // Optional
	colorBlending.blendConstants[ 2 ] = 0.0f; // Optional
	colorBlending.blendConstants[ 3 ] = 0.0f; // Optional

	VkDynamicState dynamicStates[  ] = {
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_LINE_WIDTH
	};

	VkPipelineDynamicStateCreateInfo dynamicState{  };
	dynamicState.sType 		= VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamicState.dynamicStateCount  = 2;
	dynamicState.pDynamicStates 	= dynamicStates;

	VkPipelineLayoutCreateInfo pipelineLayoutInfo{  };
	pipelineLayoutInfo.sType 			= VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount 		= 0; // Optional
	pipelineLayoutInfo.pSetLayouts 			= NULL; // Optional
	pipelineLayoutInfo.pushConstantRangeCount 	= 0; // Optional
	pipelineLayoutInfo.pPushConstantRanges 		= NULL; // Optional

	if ( vkCreatePipelineLayout( device, &pipelineLayoutInfo, NULL, &pipelineLayout ) != VK_SUCCESS )
	{
		throw std::runtime_error( "Failed to create pipeline layout!" );
	}
	VkGraphicsPipelineCreateInfo pipelineInfo{  };
	pipelineInfo.sType 			= VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineInfo.stageCount 		= 2;
	pipelineInfo.pStages 			= shaderStages;
	pipelineInfo.pVertexInputState 		= &vertexInputInfo;
	pipelineInfo.pInputAssemblyState 	= &inputAssembly;
	pipelineInfo.pViewportState 		= &viewportState;
	pipelineInfo.pRasterizationState 	= &rasterizer;
	pipelineInfo.pMultisampleState 		= &multisampling;
	pipelineInfo.pDepthStencilState 	= NULL; // Optional
	pipelineInfo.pColorBlendState 		= &colorBlending;
	pipelineInfo.pDynamicState 		= NULL; // Optional
	pipelineInfo.layout 			= pipelineLayout;
	pipelineInfo.renderPass 		= renderPass;
	pipelineInfo.subpass 			= 0;
	pipelineInfo.basePipelineHandle 	= VK_NULL_HANDLE; // Optional
	pipelineInfo.basePipelineIndex 		= -1; // Optional

	if ( vkCreateGraphicsPipelines( device, VK_NULL_HANDLE, 1, &pipelineInfo, NULL, &graphicsPipeline ) != VK_SUCCESS )
	{
		throw std::runtime_error( "Failed to create graphics pipeline!" );
	}

	vkDestroyShaderModule( device, fragShaderModule, NULL );
	vkDestroyShaderModule( device, vertShaderModule, NULL );
}

void renderer_c::cleanup
	(  )
{
	vkDestroyInstance( inst, NULL );
	
	SDL_DestroyWindow( win );
	SDL_Quit(  );
}

renderer_c::renderer_c
	(  )
{
	init_vulkan(  );
	
}

renderer_c::~renderer_c
	(  )
{
	cleanup(  );
}
