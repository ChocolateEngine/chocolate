#include "../../inc/core/renderer.h"

#define GLM_FORCE_RADIANS
#define STB_IMAGE_IMPLEMENTATION
#define TINYOBJLOADER_IMPLEMENTATION

#include <set>
#include <fstream>
#include <iostream>
#include <chrono>
#include <glm/gtc/matrix_transform.hpp>
#include <unordered_map>

#include "../../lib/io/stb_image.h"
#include "../../lib/io/tiny_obj_loader.h"

void renderer_c::init_commands
	(  )
{
	msg_s msg;
	msg.type = RENDERER_C;

	msg.msg = REND_UP;
	//msg.func = [ & ](  ){ vertices[ 0 ].pos.x += 0.05f; };
	//engineCommands.push_back( msg );
}

void renderer_c::init_vulkan
	(  )
{
	init_window(  );
	init_instance(  );
	init_validation_layers(  );
	init_surface(  );
	pick_physical_device(  );
	init_logical_device(  );
	init_swap_chain(  );
	init_image_views(  );
	init_render_pass(  );
	init_desc_set_layout(  );
	init_graphics_pipeline(  );
	init_command_pool(  );
	init_depth_resources(  );
	init_frame_buffer(  );
	init_texture_image(  );
	init_texture_image_view(  );
	init_texture_sampler(  );
	init_model( "materials/models/protogen_wip_5_plus_protodal.obj" );
	init_model( "materials/models/protogen_wip_4.obj" );
	init_uniform_buffers(  );
	init_desc_pool(  );
	init_desc_sets(  );
	init_command_buffers(  );
	init_sync(  );
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
		SDL_WINDOW_VULKAN | SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE
                );
}

std::vector< const char* > renderer_c::init_required_extensions
	(  )
{
	unsigned int extensionCount = 0;
	if ( !SDL_Vulkan_GetInstanceExtensions( win, &extensionCount, NULL ) )
	{
		throw std::runtime_error( "Unable to query the number of Vulkan instance extensions\n" );
	}
	
	// Use the amount of extensions queried before to retrieve the names of the extensions
	std::vector< const char* > extensions( extensionCount );
	if (!SDL_Vulkan_GetInstanceExtensions( win, &extensionCount, extensions.data(  ) ) )
	{
		throw std::runtime_error( "Unable to query the number of Vulkan instance extension names\n" );
	}
	// Display names
	printf( "Found %d Vulkan extensions:\n", extensionCount );
	for ( int i = 0; i < extensionCount; ++i )
	{
	        printf( "%i : %s\n", i, extensions[ i ] );
	}

	// Add debug display extension, we need this to relay debug messages
	extensions.push_back( VK_EXT_DEBUG_UTILS_EXTENSION_NAME );
	return extensions;
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

	VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo;
	if ( enableValidationLayers )
	{
		createInfo.enabledLayerCount 	= ( unsigned int )validationLayers.size(  );
		createInfo.ppEnabledLayerNames 	= validationLayers.data(  );
		init_debug_messenger_info( debugCreateInfo );
		createInfo.pNext 		= ( VkDebugUtilsMessengerCreateInfoEXT* )&debugCreateInfo;
	}
	else
	{
		createInfo.enabledLayerCount 	= 0;
		createInfo.pNext	        = NULL;
	}

	auto SDLExtensions = init_required_extensions(  );
	
	createInfo.enabledExtensionCount 	= ( uint32_t )SDLExtensions.size(  );
	createInfo.ppEnabledExtensionNames 	= SDLExtensions.data(  );

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

VKAPI_ATTR VkBool32 VKAPI_CALL renderer_c::debug_callback
	( VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
	  VkDebugUtilsMessageTypeFlagsEXT messageType,
	  const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
	  void* pUserData )
{
	std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;

        return VK_FALSE;
}

void renderer_c::init_debug_messenger_info
	( VkDebugUtilsMessengerCreateInfoEXT& c )
{
	c = {  };
	c.sType 	   = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        c.messageSeverity  = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        c.messageType 	   = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        c.pfnUserCallback  = debug_callback;
}

VkResult renderer_c::init_debug_messenger
	( VkInstance instance,
	  const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
	  const VkAllocationCallbacks* pAllocator,
	  VkDebugUtilsMessengerEXT* pDebugMessenger )
{
	auto func = ( PFN_vkCreateDebugUtilsMessengerEXT )vkGetInstanceProcAddr( inst, "vkCreateDebugUtilsMessengerEXT" );
	if ( func != NULL )
	{
		return func( inst, pCreateInfo, pAllocator, pDebugMessenger );
	}
	else
	{
		return VK_ERROR_EXTENSION_NOT_PRESENT;
	}
}

void renderer_c::init_validation_layers
	(  )
{
	if ( !enableValidationLayers ) return;

        VkDebugUtilsMessengerCreateInfoEXT createInfo{  };
	init_debug_messenger_info( createInfo );

        if ( init_debug_messenger( inst, &createInfo, NULL, &debugMessenger ) != VK_SUCCESS )
	{
		throw std::runtime_error( "Failed to set up debug messenger!" );
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
	deviceFeatures.samplerAnisotropy = VK_TRUE;

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
		swapChainImageViews[ i ] = init_image_view( swapChainImages[ i ], swapChainImageFormat, VK_IMAGE_ASPECT_COLOR_BIT );
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

VkShaderModule renderer_c::create_shader_module	//	Wraps shader bytecode into objects for pipeline to work
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

void renderer_c::init_desc_set_layout
	(  )
{
	VkDescriptorSetLayoutBinding uboLayoutBinding{  };
	uboLayoutBinding.binding 		= 0;
	uboLayoutBinding.descriptorType 	= VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	uboLayoutBinding.descriptorCount 	= 1;
	uboLayoutBinding.stageFlags 		= VK_SHADER_STAGE_VERTEX_BIT;
	uboLayoutBinding.pImmutableSamplers 	= nullptr; // Optional

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

	if ( vkCreateDescriptorSetLayout( device, &layoutInfo, nullptr, &descSetLayout ) != VK_SUCCESS )
	{
		throw std::runtime_error( "Failed to create descriptor set layout!" );
	}
}

void renderer_c::init_graphics_pipeline
	(  )
{
	auto vertShaderCode = read_file( "materials/shaders/vert.spv" );
	auto fragShaderCode = read_file( "materials/shaders/frag.spv" );
		
	VkShaderModule vertShaderModule = create_shader_module( vertShaderCode );	//	Processes incoming verticies, taking world position, color, and texture coordinates as an input
	VkShaderModule fragShaderModule = create_shader_module( fragShaderCode );	//	Fills verticies with fragments to produce color, and depth

	VkPipelineShaderStageCreateInfo vertShaderStageInfo{  };
	vertShaderStageInfo.sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;	//	Tells Vulkan which stage the shader is going to be used
	vertShaderStageInfo.stage  = VK_SHADER_STAGE_VERTEX_BIT;
	vertShaderStageInfo.module = vertShaderModule;
	vertShaderStageInfo.pName  = "main";							//	void main() in the shader to be executed

	VkPipelineShaderStageCreateInfo fragShaderStageInfo{  };
	fragShaderStageInfo.sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	fragShaderStageInfo.stage  = VK_SHADER_STAGE_FRAGMENT_BIT;
	fragShaderStageInfo.module = fragShaderModule;
	fragShaderStageInfo.pName  = "main";

	VkPipelineShaderStageCreateInfo shaderStages[  ] = { vertShaderStageInfo, fragShaderStageInfo };
	auto bindingDescription 	= vertex_t::get_binding_desc(  );
	auto attributeDescriptions 	= vertex_t::get_attribute_desc(  );

	VkPipelineVertexInputStateCreateInfo vertexInputInfo{  };	//	Format of vertex data
	vertexInputInfo.sType 				= VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertexInputInfo.vertexBindingDescriptionCount 	= 1;
	vertexInputInfo.pVertexBindingDescriptions 	= &bindingDescription;			//	Contains details for loading vertex data
	vertexInputInfo.vertexAttributeDescriptionCount = ( uint32_t )( attributeDescriptions.size(  ) );
	vertexInputInfo.pVertexAttributeDescriptions 	= attributeDescriptions.data(  );	//	Same as above

	VkPipelineInputAssemblyStateCreateInfo inputAssembly{  };	//	Collects raw vertex data from buffers
	inputAssembly.sType 			= VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssembly.topology 			= VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	inputAssembly.primitiveRestartEnable 	= VK_FALSE;

	VkViewport viewport{  };	//	Region of frambuffer to be rendered to, likely will always use 0, 0 and width, height
	viewport.x		= 0.0f;
	viewport.y 		= 0.0f;
	viewport.width 		= ( float )swapChainExtent.width;
	viewport.height 	= ( float )swapChainExtent.height;
	viewport.minDepth 	= 0.0f;
	viewport.maxDepth 	= 1.0f;

	VkRect2D scissor{  };		//	More agressive cropping than viewport, defining which regions pixels are to be stored
	scissor.offset = { 0, 0 };
	scissor.extent = swapChainExtent;

	VkPipelineViewportStateCreateInfo viewportState{  };	//	Combines viewport and scissor
	viewportState.sType 		= VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportState.viewportCount 	= 1;
	viewportState.pViewports 	= &viewport;
	viewportState.scissorCount 	= 1;
	viewportState.pScissors 	= &scissor;

	VkPipelineRasterizationStateCreateInfo rasterizer{  };		//	Turns  primitives into fragments, aka, pixels for the framebuffer
	rasterizer.sType 			= VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizer.depthClampEnable 		= VK_FALSE;
	rasterizer.rasterizerDiscardEnable 	= VK_FALSE;
	rasterizer.polygonMode 			= VK_POLYGON_MODE_FILL;		//	Fill with fragments, can optionally use VK_POLYGON_MODE_LINE for a wireframe
	rasterizer.lineWidth 			= 1.0f;
	rasterizer.cullMode 			= VK_CULL_MODE_BACK_BIT;
	rasterizer.frontFace 			= VK_FRONT_FACE_COUNTER_CLOCKWISE;
	rasterizer.depthBiasEnable 		= VK_FALSE;
	rasterizer.depthBiasConstantFactor 	= 0.0f; // Optional
	rasterizer.depthBiasClamp 		= 0.0f; // Optional
	rasterizer.depthBiasSlopeFactor 	= 0.0f; // Optional

	VkPipelineMultisampleStateCreateInfo multisampling{  };		//	Performs anti-aliasing
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

	VkPipelineDepthStencilStateCreateInfo depthStencil{  };
	depthStencil.sType 			= VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	depthStencil.depthTestEnable 		= VK_TRUE;
	depthStencil.depthWriteEnable		= VK_TRUE;
	depthStencil.depthCompareOp 		= VK_COMPARE_OP_LESS;
	depthStencil.depthBoundsTestEnable 	= VK_FALSE;
	depthStencil.minDepthBounds 		= 0.0f; // Optional
	depthStencil.maxDepthBounds 		= 1.0f; // Optional
	depthStencil.stencilTestEnable 		= VK_FALSE;
	depthStencil.front 			= {  }; // Optional
	depthStencil.back 			= {  }; // Optional

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

	VkDynamicState dynamicStates[  ] = {	//	Allows for some pipeline configuration values to change during runtime
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_LINE_WIDTH
	};

	VkPipelineDynamicStateCreateInfo dynamicState{  };
	dynamicState.sType 		= VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamicState.dynamicStateCount  = 2;
	dynamicState.pDynamicStates 	= dynamicStates;

	VkPipelineLayoutCreateInfo pipelineLayoutInfo{  };	//	Allows for use of uniform values
	pipelineLayoutInfo.sType 			= VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount 		= 1; // Optional
	pipelineLayoutInfo.pSetLayouts 			= &descSetLayout; // Optional

	if ( vkCreatePipelineLayout( device, &pipelineLayoutInfo, NULL, &pipelineLayout ) != VK_SUCCESS )
	{
		throw std::runtime_error( "Failed to create pipeline layout!" );
	}
	VkGraphicsPipelineCreateInfo pipelineInfo{  };	//	Combine all the objects above into one parameter for graphics pipeline creation
	pipelineInfo.sType 			= VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineInfo.stageCount 		= 2;
	pipelineInfo.pStages 			= shaderStages;
	pipelineInfo.pVertexInputState 		= &vertexInputInfo;
	pipelineInfo.pInputAssemblyState 	= &inputAssembly;
	pipelineInfo.pViewportState 		= &viewportState;
	pipelineInfo.pRasterizationState 	= &rasterizer;
	pipelineInfo.pMultisampleState 		= &multisampling;
	pipelineInfo.pDepthStencilState 	= &depthStencil;
	pipelineInfo.pColorBlendState 		= &colorBlending;
	pipelineInfo.pDynamicState 		= NULL; // Optional
	pipelineInfo.layout 			= pipelineLayout;
	pipelineInfo.renderPass 		= renderPass;
	pipelineInfo.subpass 			= 0;
	pipelineInfo.basePipelineHandle 	= VK_NULL_HANDLE; // Optional, very important for later when making new pipelines. It is less expensive to reference an existing similar pipeline
	pipelineInfo.basePipelineIndex 		= -1; // Optional

	if ( vkCreateGraphicsPipelines( device, VK_NULL_HANDLE, 1, &pipelineInfo, NULL, &graphicsPipeline ) != VK_SUCCESS )
	{
		throw std::runtime_error( "Failed to create graphics pipeline!" );
	}

	vkDestroyShaderModule( device, fragShaderModule, NULL );
	vkDestroyShaderModule( device, vertShaderModule, NULL );
}

void renderer_c::init_frame_buffer
	(  )
{
	swapChainFramebuffers.resize( swapChainImageViews.size(  ) );
	for ( int i = 0; i < swapChainImageViews.size(  ); i++ )
	{
		std::array< VkImageView, 2 > attachments = {
			swapChainImageViews[ i ],
			depthImageView
		};

		VkFramebufferCreateInfo framebufferInfo{  };
		framebufferInfo.sType 		= VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferInfo.renderPass 	= renderPass;
		framebufferInfo.attachmentCount = ( uint32_t )attachments.size(  );
		framebufferInfo.pAttachments 	= attachments.data(  );
		framebufferInfo.width 		= swapChainExtent.width;
		framebufferInfo.height 		= swapChainExtent.height;
		framebufferInfo.layers 		= 1;

		if ( vkCreateFramebuffer(device, &framebufferInfo, NULL, &swapChainFramebuffers[ i ] ) != VK_SUCCESS )
		{
			throw std::runtime_error( "Failed to create framebuffer!" );
		}
	}
}

void renderer_c::init_command_pool
	(  )
{
        queue_family_indices_t indices = find_queue_families( physicalDevice );

	VkCommandPoolCreateInfo poolInfo{  };
	poolInfo.sType 			= VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	poolInfo.queueFamilyIndex 	= indices.graphicsFamily.value();
	poolInfo.flags 			= 0; // Optional

	if ( vkCreateCommandPool( device, &poolInfo, NULL, &commandPool ) != VK_SUCCESS )
	{
		throw std::runtime_error( "Failed to create command pool!" );
	}
}

VkFormat renderer_c::find_supported_fmt
	( const std::vector< VkFormat >& candidates,
	  VkImageTiling tiling,
	  VkFormatFeatureFlags features )
{
	for ( VkFormat format : candidates )
	{
		VkFormatProperties props;
		vkGetPhysicalDeviceFormatProperties( physicalDevice, format, &props );
		if ( tiling == VK_IMAGE_TILING_LINEAR && ( props.linearTilingFeatures & features ) == features )
		{
			return format;
		}
		else if ( tiling == VK_IMAGE_TILING_OPTIMAL && ( props.optimalTilingFeatures & features ) == features )
		{
			return format;
		}
	}
	throw std::runtime_error( "Failed to find supported format!" );
}

VkFormat renderer_c::find_depth_format
	(  )
{
	return find_supported_fmt(
		{ VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
		VK_IMAGE_TILING_OPTIMAL,
        	VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT );
}

bool renderer_c::has_stencil_component
	( VkFormat fmt )
{
	return fmt == VK_FORMAT_D32_SFLOAT_S8_UINT || fmt == VK_FORMAT_D24_UNORM_S8_UINT;
}

void renderer_c::init_depth_resources
	(  )
{
	VkFormat depthFormat = find_depth_format(  );
	init_image( swapChainExtent.width,
		    swapChainExtent.height,
		    depthFormat,
		    VK_IMAGE_TILING_OPTIMAL,
		    VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
		    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		    depthImage,
		    depthImageMemory );
	depthImageView = init_image_view( depthImage, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT );
	transition_image_layout( depthImage,
				 depthFormat,
				 VK_IMAGE_LAYOUT_UNDEFINED,
				 VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL );
}

void renderer_c::init_command_buffers
	(  )
{
	commandBuffers.resize( swapChainFramebuffers.size(  ) );
	VkCommandBufferAllocateInfo allocInfo{  };
	allocInfo.sType 		= VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool 		= commandPool;
	allocInfo.level 		= VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandBufferCount	= ( uint32_t )commandBuffers.size(  );

	if ( vkAllocateCommandBuffers( device, &allocInfo, commandBuffers.data(  ) ) != VK_SUCCESS )
	{
		throw std::runtime_error( "Failed to allocate command buffers!" );
	}
	for ( int i = 0; i < commandBuffers.size(  ); i++ )
	{
		VkCommandBufferBeginInfo beginInfo{  };
		beginInfo.sType 		= VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags 		= 0; // Optional
		beginInfo.pInheritanceInfo 	= NULL; // Optional

		if ( vkBeginCommandBuffer( commandBuffers[ i ], &beginInfo ) != VK_SUCCESS )
		{
			throw std::runtime_error( "Failed to begin recording command buffer!" );
		}
		VkRenderPassBeginInfo renderPassInfo{  };
		renderPassInfo.sType 		 = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassInfo.renderPass 	 = renderPass;
		renderPassInfo.framebuffer 	 = swapChainFramebuffers[ i ];
		renderPassInfo.renderArea.offset = { 0, 0 };
		renderPassInfo.renderArea.extent = swapChainExtent;

		std::array< VkClearValue, 2 > clearValues{  };
		clearValues[ 0 ].color = { 0.0f, 0.0f, 0.0f, 1.0f };
		clearValues[ 1 ].depthStencil = { 1.0f, 0 };
			
		renderPassInfo.clearValueCount 	 = ( uint32_t )clearValues.size(  );
		renderPassInfo.pClearValues 	 = clearValues.data(  );

		vkCmdBeginRenderPass( commandBuffers[ i ], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE );
		vkCmdBindPipeline( commandBuffers[ i ], VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline );

		for ( auto& model : models )
		{
			model.bind( commandBuffers[ i ] );
			vkCmdBindDescriptorSets( commandBuffers[ i ], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descSets[ i ], 0, NULL );
			model.draw( commandBuffers[ i ] );
		}
		
		vkCmdEndRenderPass( commandBuffers[ i ] );

		if ( vkEndCommandBuffer( commandBuffers[ i ] ) != VK_SUCCESS )
		{
			throw std::runtime_error( "Failed to record command buffer!" );
		}
	}
}

void renderer_c::init_sync
	(  )
{
	imageAvailableSemaphores.resize( MAX_FRAMES_PROCESSING );
	renderFinishedSemaphores.resize( MAX_FRAMES_PROCESSING );
	inFlightFences.resize( MAX_FRAMES_PROCESSING );
	imagesInFlight.resize( swapChainImages.size(  ), VK_NULL_HANDLE );
	
	VkSemaphoreCreateInfo semaphoreInfo{  };
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	VkFenceCreateInfo fenceInfo{  };
	fenceInfo.sType	    = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceInfo.flags	    = VK_FENCE_CREATE_SIGNALED_BIT;

	for ( int i = 0 ; i < MAX_FRAMES_PROCESSING; ++i )
	{
		if ( vkCreateSemaphore( device, &semaphoreInfo, NULL, &imageAvailableSemaphores[ i ] ) != VK_SUCCESS ||
		     vkCreateSemaphore( device, &semaphoreInfo, NULL, &renderFinishedSemaphores[ i ] ) != VK_SUCCESS ||
		     vkCreateFence( device, &fenceInfo, NULL, &inFlightFences[ i ] ) != VK_SUCCESS )
		{
			throw std::runtime_error( "Failed to create sync objects!" );
		}
	}
}

uint32_t renderer_c::find_memory_type
	( uint32_t typeFilter, VkMemoryPropertyFlags properties )
{
	VkPhysicalDeviceMemoryProperties memProperties;
	vkGetPhysicalDeviceMemoryProperties( physicalDevice, &memProperties );

	for ( uint32_t i = 0; i < memProperties.memoryTypeCount; ++i )
	{
		if ( ( typeFilter & ( 1 << i ) ) && ( memProperties.memoryTypes[ i ].propertyFlags & properties ) == properties )
		{
			return i;
		}
	}

	throw std::runtime_error( "Failed to find suitable memory type!" );
}

void renderer_c::init_model
	( const std::string& modelPath )
{
	tinyobj::attrib_t attrib;
	std::vector< tinyobj::shape_t > shapes;
	std::vector< tinyobj::material_t > materials;
	std::vector< vertex_t > vertices;
	std::vector< uint32_t > indices;
	std::string warn, err;

	if ( !tinyobj::LoadObj( &attrib, &shapes, &materials, &warn, &err, modelPath.c_str(  ) ) )
	{
		throw std::runtime_error( warn + err );
	}

	std::unordered_map< vertex_t, uint32_t > uniqueVertices{  };
	
	for ( const auto& shape : shapes )
	{
		for ( const auto& index : shape.mesh.indices )
		{
			vertex_t vertex{  };

			vertex.pos = {
				attrib.vertices[ 3 * index.vertex_index + 0 ],
				attrib.vertices[ 3 * index.vertex_index + 1 ],
				attrib.vertices[ 3 * index.vertex_index + 2 ]
			};

			vertex.texCoord = {
				attrib.texcoords[ 2 * index.texcoord_index + 0 ],
				1.0f - attrib.texcoords[ 2 * index.texcoord_index + 1 ]
			};

			vertex.color = { 1.0f, 1.0f, 1.0f };
			if ( uniqueVertices.count( vertex ) == 0 )
			{
				uniqueVertices[ vertex ] = ( uint32_t )vertices.size(  );
				vertices.push_back( vertex );
			}
			
			indices.push_back( uniqueVertices[ vertex ] );
		}
	}
	model_data_t model{  };
	model.vCount = ( uint32_t )vertices.size(  );
	model.iCount = ( uint32_t )indices.size(  );
	update_vertex_buffer( vertices, model.vBuffer, model.vBufferMem );
	update_index_buffer( indices, model.iBuffer, model.iBufferMem );
	models.push_back( model );
	printf( "%d vertices loaded!\n", vertices.size(  ) );
}

void renderer_c::init_image
	( uint32_t width,
	  uint32_t height,
	  VkFormat format,
	  VkImageTiling tiling,
	  VkImageUsageFlags usage,
	  VkMemoryPropertyFlags properties,
	  VkImage& image,
	  VkDeviceMemory& imageMemory )
{
	VkImageCreateInfo imageInfo{  };
	imageInfo.sType 	= VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageInfo.imageType 	= VK_IMAGE_TYPE_2D;
	imageInfo.extent.width 	= width;
	imageInfo.extent.height = height;
	imageInfo.extent.depth 	= 1;
	imageInfo.mipLevels 	= 1;
	imageInfo.arrayLayers 	= 1;
	imageInfo.format 	= format;
	imageInfo.tiling 	= tiling;
	imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	imageInfo.usage	 	= usage;
	imageInfo.samples 	= VK_SAMPLE_COUNT_1_BIT;
	imageInfo.sharingMode 	= VK_SHARING_MODE_EXCLUSIVE;

	if ( vkCreateImage( device, &imageInfo, NULL, &image ) != VK_SUCCESS )
	{
		throw std::runtime_error( "Failed to create image!" );
	}

	VkMemoryRequirements memRequirements;
	vkGetImageMemoryRequirements( device, image, &memRequirements );

	VkMemoryAllocateInfo allocInfo{  };
	allocInfo.sType 	  = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize  = memRequirements.size;
	allocInfo.memoryTypeIndex = find_memory_type( memRequirements.memoryTypeBits, properties );

	if ( vkAllocateMemory( device, &allocInfo, NULL, &imageMemory ) != VK_SUCCESS )
	{
		throw std::runtime_error( "Failed to allocate image memory!" );
	}

	vkBindImageMemory( device, image, imageMemory, 0 );
}

void renderer_c::init_texture_image
	(  )
{
	int texWidth, texHeight, texChannels;
	stbi_uc* pixels = stbi_load( TEXTUREPATH.c_str(  ), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha );
	VkDeviceSize imageSize = texWidth * texHeight * 4;

	if ( !pixels ) {
		throw std::runtime_error( "Failed to load texture image!" );
	}

	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;
	init_buffer( imageSize,
		     VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		     stagingBuffer,
		     stagingBufferMemory );

	void* data;
	vkMapMemory( device, stagingBufferMemory, 0, imageSize, 0, &data );
	memcpy( data, pixels, ( size_t )imageSize );
	vkUnmapMemory( device, stagingBufferMemory );

	stbi_image_free( pixels );

        init_image( texWidth,
		    texHeight,
		    VK_FORMAT_R8G8B8A8_SRGB,
		    VK_IMAGE_TILING_OPTIMAL,
		    VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
		    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		    textureImage,
		    textureImageMemory );
	transition_image_layout( textureImage,
				 VK_FORMAT_R8G8B8A8_SRGB,
				 VK_IMAGE_LAYOUT_UNDEFINED,
				 VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL );
	copy_buffer_to_img( stagingBuffer,
			    textureImage,
			    ( uint32_t )texWidth,
			    ( uint32_t )texHeight );
	transition_image_layout( textureImage,
				 VK_FORMAT_R8G8B8A8_SRGB,
				 VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				 VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL );

	vkDestroyBuffer( device, stagingBuffer, NULL );
	vkFreeMemory( device, stagingBufferMemory, NULL );
}

void renderer_c::init_texture_image_view
	(  )
{
	textureImageView = init_image_view( textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT );
}

void renderer_c::init_texture_sampler
	(  )
{
	VkSamplerCreateInfo samplerInfo{  };
	samplerInfo.sType  	 = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	samplerInfo.magFilter    = VK_FILTER_LINEAR;
	samplerInfo.minFilter    = VK_FILTER_LINEAR;
	samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;

	VkPhysicalDeviceProperties properties{  };
	vkGetPhysicalDeviceProperties( physicalDevice, &properties );

	samplerInfo.anisotropyEnable 		= VK_TRUE;
	samplerInfo.maxAnisotropy 		= properties.limits.maxSamplerAnisotropy;
	samplerInfo.borderColor 		= VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	samplerInfo.unnormalizedCoordinates 	= VK_FALSE;
	samplerInfo.compareEnable 		= VK_FALSE;
	samplerInfo.compareOp 			= VK_COMPARE_OP_ALWAYS;
	samplerInfo.mipmapMode 			= VK_SAMPLER_MIPMAP_MODE_LINEAR;
	samplerInfo.mipLodBias 			= 0.0f;
	samplerInfo.minLod 			= 0.0f;
	samplerInfo.maxLod 			= 0.0f;

	if ( vkCreateSampler( device, &samplerInfo, NULL, &textureSampler) != VK_SUCCESS )
	{
		throw std::runtime_error( "Failed to create texture sampler!" );
	}
}

void renderer_c::update_vertex_buffer
	( const std::vector< vertex_t >& v, VkBuffer& vBuffer, VkDeviceMemory& vBufferMem )
{
	VkDeviceSize bufferSize = sizeof( v[ 0 ] ) * v.size(  );
	
	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;
	init_buffer( bufferSize,
		     VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
		     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		     stagingBuffer,
		     stagingBufferMemory );
	
	map_memory( stagingBufferMemory, bufferSize, v.data(  ) );

	init_buffer( bufferSize,
		     VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
		     VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		     vBuffer,
		     vBufferMem );

	buf_copy( stagingBuffer, vBuffer, bufferSize );

	vkDestroyBuffer( device, stagingBuffer, NULL );
        vkFreeMemory( device, stagingBufferMemory, NULL );
}

void renderer_c::update_index_buffer
	( std::vector< uint32_t >& i, VkBuffer& iBuffer, VkDeviceMemory& iBufferMem )	//	pls nuke this in the future, it is a copy of the above function
{
	VkDeviceSize bufferSize = sizeof( i[ 0 ] ) * i.size(  );

	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;
	init_buffer( bufferSize,
		     VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		     stagingBuffer, stagingBufferMemory );

	map_memory( stagingBufferMemory, bufferSize, i.data(  ) );

	init_buffer( bufferSize,
		     VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
		     VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		     iBuffer,
		     iBufferMem );

	buf_copy( stagingBuffer, iBuffer, bufferSize );

	vkDestroyBuffer( device, stagingBuffer, NULL );
	vkFreeMemory( device, stagingBufferMemory, NULL );
}

void renderer_c::init_uniform_buffers
	(  )
{
	VkDeviceSize bufferSize = sizeof( ubo_t );

	uniformBuffers.resize( swapChainImages.size(  ) );
	uniformBuffersMemory.resize( swapChainImages.size(  ) );

	for ( int i = 0; i < swapChainImages.size(  ); i++ ) {
		init_buffer( bufferSize,
			     VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			     uniformBuffers[ i ],
			     uniformBuffersMemory[ i ] );
	}
}

void renderer_c::init_desc_pool
	(  )
{
	std::array< VkDescriptorPoolSize, 2 > poolSizes{  };
	poolSizes[ 0 ].type 		 = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	poolSizes[ 0 ].descriptorCount 	 = ( uint32_t )swapChainImages.size(  );
	poolSizes[ 1 ].type 		 = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	poolSizes[ 1 ].descriptorCount 	 = ( uint32_t )swapChainImages.size(  );

	VkDescriptorPoolCreateInfo poolInfo{  };
	poolInfo.sType 		= VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolInfo.poolSizeCount  = ( uint32_t )poolSizes.size(  );
	poolInfo.pPoolSizes 	= poolSizes.data(  );
	poolInfo.maxSets 	= ( uint32_t )swapChainImages.size(  );

	if ( vkCreateDescriptorPool( device, &poolInfo, NULL, &descPool ) != VK_SUCCESS )
	{
		throw std::runtime_error( "Failed to create descriptor pool!" );
	}
}

void renderer_c::init_desc_sets
	(  )
{
	std::vector< VkDescriptorSetLayout > layouts( swapChainImages.size(  ), descSetLayout );
	VkDescriptorSetAllocateInfo allocInfo{  };
	allocInfo.sType 		= VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool 	= descPool;
	allocInfo.descriptorSetCount 	= ( uint32_t )swapChainImages.size(  );
	allocInfo.pSetLayouts 		= layouts.data(  );

	descSets.resize( swapChainImages.size(  ) );
	if ( vkAllocateDescriptorSets( device, &allocInfo, descSets.data(  ) ) != VK_SUCCESS )
	{
		throw std::runtime_error( "Failed to allocate descriptor sets!" );
	}

	for ( int i = 0; i < swapChainImages.size(  ); i++ )
	{
		VkDescriptorBufferInfo bufferInfo{  };
		bufferInfo.buffer = uniformBuffers[ i ];
		bufferInfo.offset = 0;
		bufferInfo.range  = sizeof( ubo_t );

		VkDescriptorImageInfo imageInfo{  };
		imageInfo.imageLayout   = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		imageInfo.imageView 	= textureImageView;
		imageInfo.sampler 	= textureSampler;

		std::array< VkWriteDescriptorSet, 2 > descriptorWrites{  };
		descriptorWrites[ 0 ].sType 		 = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[ 0 ].dstSet 		 = descSets[ i ];
		descriptorWrites[ 0 ].dstBinding 	 = 0;
		descriptorWrites[ 0 ].dstArrayElement  	 = 0;
		descriptorWrites[ 0 ].descriptorType   	 = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		descriptorWrites[ 0 ].descriptorCount  	 = 1;
		descriptorWrites[ 0 ].pBufferInfo 	 = &bufferInfo;

		descriptorWrites[ 1 ].sType 		= VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[ 1 ].dstSet 		= descSets[ i ];
		descriptorWrites[ 1 ].dstBinding 	= 1;
		descriptorWrites[ 1 ].dstArrayElement 	= 0;
		descriptorWrites[ 1 ].descriptorType 	= VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		descriptorWrites[ 1 ].descriptorCount	= 1;
		descriptorWrites[ 1 ].pImageInfo 	= &imageInfo;

		vkUpdateDescriptorSets( device, ( uint32_t )descriptorWrites.size(  ), descriptorWrites.data(  ), 0, NULL );
	}
}

void renderer_c::reinit_swap_chain
	(  )
{
	vkDeviceWaitIdle( device );

	destroy_swap_chain(  );
	
	init_swap_chain(  );
	init_image_views(  );
	init_render_pass(  );
	init_graphics_pipeline(  );
	init_depth_resources(  );
	init_frame_buffer(  );
	init_uniform_buffers(  );
	init_desc_pool(  );
	init_desc_sets(  );
	init_command_buffers(  );
}

void renderer_c::destroy_swap_chain
	(  )
{
	vkDestroyImageView( device, depthImageView, nullptr );
	vkDestroyImage( device, depthImage, nullptr );
	vkFreeMemory( device, depthImageMemory, nullptr );
	for ( int i = 0; i < swapChainImages.size(  ); i++ ) {
		vkDestroyBuffer( device, uniformBuffers[ i ], NULL );
		vkFreeMemory( device, uniformBuffersMemory[ i ], NULL );
	}
	vkDestroyDescriptorPool( device, descPool, NULL );
        for ( auto framebuffer : swapChainFramebuffers )
	{
		vkDestroyFramebuffer( device, framebuffer, NULL );
        }
	vkFreeCommandBuffers( device, commandPool, ( uint32_t )commandBuffers.size(  ), commandBuffers.data(  ) );

	vkDestroyPipeline( device, graphicsPipeline, NULL );
        vkDestroyPipelineLayout( device, pipelineLayout, NULL );
        vkDestroyRenderPass( device, renderPass, NULL );

        for ( auto imageView : swapChainImageViews )
	{
		vkDestroyImageView( device, imageView, NULL );
        }

        vkDestroySwapchainKHR( device, swapChain, NULL );
}

VkImageView renderer_c::init_image_view
	( VkImage image, VkFormat format, VkImageAspectFlags aspectFlags )
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

	VkImageView imageView;
	if ( vkCreateImageView( device, &viewInfo, nullptr, &imageView ) != VK_SUCCESS )
	{
		throw std::runtime_error( "Failed to create texture image view!" );
	}

	return imageView;
}

void renderer_c::transition_image_layout
	( VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout )
{
	VkCommandBuffer c = begin_single_time_commands(  );

	VkImageMemoryBarrier barrier{  };
        barrier.sType 				= VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.oldLayout 			= oldLayout;
        barrier.newLayout 			= newLayout;
        barrier.srcQueueFamilyIndex 		= VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex 		= VK_QUEUE_FAMILY_IGNORED;
        barrier.image 				= image;
        barrier.subresourceRange.aspectMask 	= VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.baseMipLevel 	= 0;
        barrier.subresourceRange.levelCount 	= 1;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount 	= 1;

	VkPipelineStageFlags sourceStage;
	VkPipelineStageFlags destinationStage;

	if ( oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL )
	{
		barrier.srcAccessMask 	= 0;
		barrier.dstAccessMask 	= VK_ACCESS_TRANSFER_WRITE_BIT;

		sourceStage 		= VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		destinationStage	= VK_PIPELINE_STAGE_TRANSFER_BIT;
	}
	else if ( oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL )
	{
		barrier.srcAccessMask 	= VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask 	= VK_ACCESS_SHADER_READ_BIT;

		sourceStage 		= VK_PIPELINE_STAGE_TRANSFER_BIT;
		destinationStage 	= VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	}
	else if ( oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL )
	{
		barrier.srcAccessMask 	= 0;
		barrier.dstAccessMask 	= VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

		sourceStage 		= VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		destinationStage 	= VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	}
	else
	{
		throw std::invalid_argument( "Unsupported layout transition!" );
	}

	vkCmdPipelineBarrier(
		c,
		sourceStage, destinationStage,
		0,
		0,
		NULL,
		0,
		NULL,
		1, &barrier );

	end_single_time_commands( c );
}

VkCommandBuffer renderer_c::begin_single_time_commands
	(  )
{
	VkCommandBufferAllocateInfo allocInfo{  };
	allocInfo.sType 		= VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.level 		= VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandPool 		= commandPool;
	allocInfo.commandBufferCount 	= 1;

	VkCommandBuffer commandBuffer;
	vkAllocateCommandBuffers( device, &allocInfo, &commandBuffer );

	VkCommandBufferBeginInfo beginInfo{  };
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	vkBeginCommandBuffer( commandBuffer, &beginInfo );

	return commandBuffer;
}

void renderer_c::end_single_time_commands
	( VkCommandBuffer c )
{
	vkEndCommandBuffer( c );

	VkSubmitInfo submitInfo{  };
	submitInfo.sType 		= VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount 	= 1;
	submitInfo.pCommandBuffers 	= &c;

	vkQueueSubmit( graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE );
	vkQueueWaitIdle( graphicsQueue );

	vkFreeCommandBuffers( device, commandPool, 1, &c );
}

void renderer_c::init_buffer
		( VkDeviceSize size,
		  VkBufferUsageFlags usage,
		  VkMemoryPropertyFlags properties,
		  VkBuffer& buffer,
		  VkDeviceMemory& bufferMemory )
{
	VkBufferCreateInfo bufferInfo{  };
	bufferInfo.sType 	= VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.size 	= size;
	bufferInfo.usage 	= usage;
	bufferInfo.sharingMode  = VK_SHARING_MODE_EXCLUSIVE;

	if ( vkCreateBuffer( device, &bufferInfo, NULL, &buffer ) != VK_SUCCESS )
	{
		throw std::runtime_error( "Failed to create buffer!" );
	}

	VkMemoryRequirements memRequirements;
	vkGetBufferMemoryRequirements( device, buffer, &memRequirements );

	VkMemoryAllocateInfo allocInfo{  };
	allocInfo.sType 		= VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize 	= memRequirements.size;
	allocInfo.memoryTypeIndex 	= find_memory_type( memRequirements.memoryTypeBits, properties );

	if ( vkAllocateMemory( device, &allocInfo, nullptr, &bufferMemory ) != VK_SUCCESS )
	{
		throw std::runtime_error( "Failed to allocate buffer memory!" );
	}

	vkBindBufferMemory( device, buffer, bufferMemory, 0 );
}

void renderer_c::map_memory
	( VkDeviceMemory bufMem, VkDeviceSize size, const void* in )
{
	void* data;
	vkMapMemory( device, bufMem, 0, size, 0, &data );
	memcpy( data, in, ( size_t )size );
	vkUnmapMemory( device, bufMem );
}

void renderer_c::buf_copy
	( VkBuffer src, VkBuffer dst, VkDeviceSize size )
{
	VkCommandBuffer commandBuffer = begin_single_time_commands(  );
	
	VkBufferCopy copyRegion{  };
	copyRegion.srcOffset 	= 0; // Optional
	copyRegion.dstOffset 	= 0; // Optional
	copyRegion.size 	= size;
	vkCmdCopyBuffer( commandBuffer, src, dst, 1, &copyRegion );
	end_single_time_commands( commandBuffer );
}

void renderer_c::copy_buffer_to_img
	( VkBuffer buffer, VkImage image, uint32_t width, uint32_t height )
{
	VkCommandBuffer c = begin_single_time_commands(  );

	VkBufferImageCopy region{  };
	region.bufferOffset 		= 0;
	region.bufferRowLength 		= 0;
	region.bufferImageHeight 	= 0;

	region.imageSubresource.aspectMask 	= VK_IMAGE_ASPECT_COLOR_BIT;
	region.imageSubresource.mipLevel 	= 0;
	region.imageSubresource.baseArrayLayer  = 0;
	region.imageSubresource.layerCount 	= 1;

	region.imageOffset = { 0, 0, 0 };
	region.imageExtent = {
		width,
		height,
		1
	};

	vkCmdCopyBufferToImage(
		c,
		buffer,
		image,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		1,
		&region
		);

	end_single_time_commands( c );
}

void renderer_c::update_uniform_buffers
	( uint32_t currentImage )
{
	static auto startTime 	= std::chrono::high_resolution_clock::now(  );

	auto currentTime 	= std::chrono::high_resolution_clock::now();
	float time 		= std::chrono::duration< float, std::chrono::seconds::period >( currentTime - startTime ).count(  );

	ubo_t ubo{  };
	ubo.model = glm::rotate( glm::mat4( 1.0f ), time * glm::radians( 45.0f ), glm::vec3( 0.0f, 0.0f, 1.0f ) );
	ubo.view  = glm::lookAt( glm::vec3( 0.1f, 10.0f, 25.0f ), glm::vec3( 0.0f, 0.0f, 0.0f ), glm::vec3( 0.0f, 0.0f, 1.0f ) );
	ubo.proj  = glm::perspective( glm::radians( 90.0f ), swapChainExtent.width / ( float ) swapChainExtent.height, 0.1f, 256.0f );

	ubo.proj[ 1 ][ 1 ] *= -1;

	void* data;
	vkMapMemory( device, uniformBuffersMemory[ currentImage ], 0, sizeof( ubo ), 0, &data );
	memcpy( data, &ubo, sizeof( ubo ) );
	vkUnmapMemory( device, uniformBuffersMemory[ currentImage ] );
}

void renderer_c::draw_frame
	(  )
{
	vkWaitForFences( device, 1, &inFlightFences[ currentFrame ], VK_TRUE, UINT64_MAX );
	
	uint32_t imageIndex;
	VkResult res =  vkAcquireNextImageKHR( device, swapChain, UINT64_MAX, imageAvailableSemaphores[ currentFrame ], VK_NULL_HANDLE, &imageIndex );

	if ( res == VK_ERROR_OUT_OF_DATE_KHR )
	{
		reinit_swap_chain(  );
		return;
	}
	else if ( res != VK_SUCCESS && res != VK_SUBOPTIMAL_KHR )
	{
		throw std::runtime_error( "Failed ot acquire swap chain image!" );
	}
	if ( imagesInFlight[ imageIndex ] != VK_NULL_HANDLE )
	{
		vkWaitForFences( device, 1, &imagesInFlight[ imageIndex ], VK_TRUE, UINT64_MAX );
	}
	imagesInFlight[ imageIndex ] = inFlightFences[ currentFrame ];

	update_uniform_buffers( imageIndex );

	VkSubmitInfo submitInfo{  };
	submitInfo.sType 			= VK_STRUCTURE_TYPE_SUBMIT_INFO;

	VkSemaphore waitSemaphores[  ] 		= { imageAvailableSemaphores[ currentFrame ] };
	VkPipelineStageFlags waitStages[  ] 	= { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	submitInfo.waitSemaphoreCount 		= 1;
	submitInfo.pWaitSemaphores 		= waitSemaphores;
	submitInfo.pWaitDstStageMask 		= waitStages;
	submitInfo.commandBufferCount 		= 1;
	submitInfo.pCommandBuffers 		= &commandBuffers[imageIndex];
	VkSemaphore signalSemaphores[  ] 	= { renderFinishedSemaphores[ currentFrame ] };
	submitInfo.signalSemaphoreCount 	= 1;
	submitInfo.pSignalSemaphores 		= signalSemaphores;

	vkResetFences( device, 1, &inFlightFences[ currentFrame ] );
	if ( vkQueueSubmit( graphicsQueue, 1, &submitInfo, inFlightFences[ currentFrame ] ) != VK_SUCCESS )
	{
		throw std::runtime_error( "Failed to submit draw command buffer!" );
	}
	VkPresentInfoKHR presentInfo{  };
	presentInfo.sType 			= VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

	presentInfo.waitSemaphoreCount 		= 1;
	presentInfo.pWaitSemaphores 		= signalSemaphores;

	VkSwapchainKHR swapChains[  ] 		= { swapChain };
	presentInfo.swapchainCount 		= 1;
	presentInfo.pSwapchains 		= swapChains;
	presentInfo.pImageIndices 		= &imageIndex;
	presentInfo.pResults 			= NULL; // Optional

	res = vkQueuePresentKHR( presentQueue, &presentInfo );

	if ( res == VK_ERROR_OUT_OF_DATE_KHR )
	{
		reinit_swap_chain(  );
	}
	else if ( res != VK_SUCCESS && res != VK_SUBOPTIMAL_KHR )
	{
		throw std::runtime_error( "Failed ot present swap chain image!" );
	}
	
	vkQueueWaitIdle( presentQueue );

	currentFrame = ( currentFrame + 1 ) % MAX_FRAMES_PROCESSING;
	
	//SDL_Delay( 1000 / 144 );
}

void renderer_c::destroy_debug_messenger
	( VkInstance instance,
	  VkDebugUtilsMessengerEXT debugMessenger,
	  const VkAllocationCallbacks* pAllocator )
{
	auto func = ( PFN_vkDestroyDebugUtilsMessengerEXT )vkGetInstanceProcAddr( inst, "vkDestroyDebugUtilsMessengerEXT" );
	if ( func != NULL )
	{
		func( instance, debugMessenger, pAllocator );
	}
}

void renderer_c::cleanup
	(  )
{
	destroy_swap_chain(  );
	vkDestroySampler( device, textureSampler, NULL );
	vkDestroyImageView( device, textureImageView, NULL );
	vkDestroyImage( device, textureImage, NULL );
	vkFreeMemory( device, textureImageMemory, NULL );
	vkDestroyDescriptorSetLayout( device, descSetLayout, NULL );
	for ( auto& model : models )
	{
		vkDestroyBuffer( device, model.iBuffer, NULL );
		vkFreeMemory( device, model.iBufferMem, NULL );
		vkDestroyBuffer( device, model.vBuffer, NULL );
		vkFreeMemory( device, model.vBufferMem, NULL );
	}
	for ( int i = 0; i < MAX_FRAMES_PROCESSING; i++ )
	{
		vkDestroySemaphore( device, renderFinishedSemaphores[ i ], NULL );
		vkDestroySemaphore( device, imageAvailableSemaphores[ i ], NULL );
		vkDestroyFence( device, inFlightFences[ i ], NULL);
        }

        vkDestroyCommandPool( device, commandPool, NULL );

        vkDestroyDevice( device, NULL );

        if ( enableValidationLayers ) {
		destroy_debug_messenger( inst, debugMessenger, NULL );
        }

        vkDestroySurfaceKHR( inst, surf, NULL );
        vkDestroyInstance( inst, NULL );
	
	SDL_DestroyWindow( win );
	SDL_Quit(  );
}

renderer_c::renderer_c
	(  )
{
	init_vulkan(  );

	systemType = RENDERER_C;
	init_commands(  );
}

renderer_c::~renderer_c
	(  )
{
	cleanup(  );
}
