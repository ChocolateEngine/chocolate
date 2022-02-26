#include "device.h"
#include "instance.h"
#include "gutil.hh"

#include "core/log.h"

#include <set>

constexpr char const *gpDeviceExtensions[] = { VK_KHR_SWAPCHAIN_EXTENSION_NAME, "VK_EXT_descriptor_indexing" };

bool CheckDeviceExtensionSupport( VkPhysicalDevice sDevice )
{
	unsigned int extensionCount;
	vkEnumerateDeviceExtensionProperties( sDevice, NULL, &extensionCount, NULL );

	std::vector< VkExtensionProperties > availableExtensions( extensionCount );
	vkEnumerateDeviceExtensionProperties( sDevice, NULL, &extensionCount, availableExtensions.data(  ) );

	std::set< std::string > requiredExtensions( gpDeviceExtensions, gpDeviceExtensions + sizeof( gpDeviceExtensions ) / sizeof( gpDeviceExtensions[ 0 ] ) );

	for ( const auto& extension : availableExtensions )
	{
		requiredExtensions.erase( extension.extensionName  );
	}
	return requiredExtensions.empty(  );
}

bool SuitableCard( VkPhysicalDevice sDevice )
{
    QueueFamilyIndices indices 		= FindQueueFamilies( sDevice );
	bool extensionsSupported 		= CheckDeviceExtensionSupport( sDevice );
	bool swapChainAdequate 			= false;
	if ( extensionsSupported )
	{
		SwapChainSupportInfo swapChainSupport = CheckSwapChainSupport( sDevice );
		swapChainAdequate = !swapChainSupport.aFormats.empty(  ) && !swapChainSupport.aPresentModes.empty(  );
	}
	
	return indices.Complete(  ) && extensionsSupported && swapChainAdequate;
}

VkSampleCountFlagBits Device::GetMaxUsableSampleCount()
{
	VkPhysicalDeviceProperties 	physicalDeviceProperties;
	vkGetPhysicalDeviceProperties( aPhysicalDevice, &physicalDeviceProperties );

	VkSampleCountFlags counts = physicalDeviceProperties.limits.framebufferColorSampleCounts & physicalDeviceProperties.limits.framebufferDepthSampleCounts;
	if ( counts & VK_SAMPLE_COUNT_64_BIT ) 	return VK_SAMPLE_COUNT_64_BIT;
	if ( counts & VK_SAMPLE_COUNT_32_BIT ) 	return VK_SAMPLE_COUNT_32_BIT;
	if ( counts & VK_SAMPLE_COUNT_16_BIT ) 	return VK_SAMPLE_COUNT_16_BIT;
	if ( counts & VK_SAMPLE_COUNT_8_BIT ) 	return VK_SAMPLE_COUNT_8_BIT;
	if ( counts & VK_SAMPLE_COUNT_4_BIT ) 	return VK_SAMPLE_COUNT_4_BIT;
	if ( counts & VK_SAMPLE_COUNT_2_BIT ) 	return VK_SAMPLE_COUNT_2_BIT;

	return VK_SAMPLE_COUNT_1_BIT;
}

void Device::SetupPhysicalDevice() 
{
    aPhysicalDevice 	   = VK_NULL_HANDLE;
	uint32_t deviceCount   = 0;
	
	vkEnumeratePhysicalDevices( gInstance.GetInstance(), &deviceCount, NULL );
	if ( deviceCount == 0 ) {
		throw std::runtime_error( "Failed to find GPUs with Vulkan support!" );
	}
	std::vector< VkPhysicalDevice > devices( deviceCount );
	vkEnumeratePhysicalDevices( gInstance.GetInstance(), &deviceCount, devices.data(  ) );

	for ( const auto& device : devices ) {
		if ( SuitableCard( device ) ) {
			aPhysicalDevice = device;
			aSampleCount    = GetMaxUsableSampleCount();
			break;
		}
	}

	if ( aPhysicalDevice == VK_NULL_HANDLE ) 
		LogFatal( "Failed to find a suitable GPU!" );
}

void Device::CreateDevice()
{
    QueueFamilyIndices indices = FindQueueFamilies( aPhysicalDevice );

	std::vector< VkDeviceQueueCreateInfo > queueCreateInfos;
	std::set< uint32_t > uniqueQueueFamilies = { ( uint32_t )indices.aGraphicsFamily, ( uint32_t )indices.aPresentFamily };

	float queuePriority = 1.0f;
	for ( uint32_t queueFamily : uniqueQueueFamilies )
	{
		VkDeviceQueueCreateInfo queueCreateInfo = {
		    .sType			    = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
		    .queueFamilyIndex 	= queueFamily,
		    .queueCount		    = 1,
		    .pQueuePriorities 	= &queuePriority,
        };
		
		queueCreateInfos.push_back( queueCreateInfo );
	}

	VkPhysicalDeviceFeatures deviceFeatures{  };
	deviceFeatures.samplerAnisotropy = VK_FALSE;	//	Temporarily disabled for PBP builds

	VkPhysicalDeviceDescriptorIndexingFeaturesEXT indexing{};
    indexing.sType                                    = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES_EXT;
    indexing.pNext                                    = nullptr;
    indexing.descriptorBindingPartiallyBound          = VK_TRUE;
    indexing.runtimeDescriptorArray                   = VK_TRUE;
    indexing.descriptorBindingVariableDescriptorCount = VK_TRUE;


	VkDeviceCreateInfo createInfo = {
	    .sType 			         = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
	    .pNext                   = &indexing,
        .flags                   = 0,	    
	    .queueCreateInfoCount 	 = ( uint32_t )queueCreateInfos.size(  ),
        .pQueueCreateInfos 		 = queueCreateInfos.data(  ),
        .enabledLayerCount       = ( uint32_t )sizeof( gpValidationLayers ),
        .ppEnabledLayerNames     = gpValidationLayers,
        .enabledExtensionCount   = ( uint32_t )sizeof( gpDeviceExtensions ),
        .ppEnabledExtensionNames = gpDeviceExtensions,
        .pEnabledFeatures        = &deviceFeatures,
    };

	if ( vkCreateDevice( aPhysicalDevice, &createInfo, NULL, &aDevice ) != VK_SUCCESS )
        LogFatal( "Failed to create logical device!" );

	vkGetDeviceQueue( aDevice, indices.aGraphicsFamily, 0, &aGraphicsQueue );
	vkGetDeviceQueue( aDevice, indices.aPresentFamily,  0, &aPresentQueue );
}

Device::Device()
{
    SetupPhysicalDevice();
    CreateDevice();
}

Device::~Device()
{
    vkDestroyDevice( aDevice, NULL );
}

Device gDevice{};