#include "instance.h"

#include "config.hh"
#include "gutil.hh"

#include "core/commandline.h"
#include "core/log.h"

VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback( VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,    VkDebugUtilsMessageTypeFlagsEXT messageType, 
                                              const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData )
{
	static bool dumpVLayers = cmdline->Find( "-dump-vlayers" ) || cmdline->Find( "-vlayers" );
	if ( dumpVLayers )
	{
		LogDev( 1, "\n[Validation Layer] %s\n\n", pCallbackData->pMessage );
	}

	return VK_FALSE;
}

constexpr VkDebugUtilsMessengerCreateInfoEXT gLayerInfo = {
	.sType 	   		  = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
	.messageSeverity  = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
	.messageType 	  = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
	.pfnUserCallback  = DebugCallback,
};

bool CheckValidationLayerSupport()
{
	bool layerFound;
	unsigned int layerCount;
	vkEnumerateInstanceLayerProperties( &layerCount, NULL );

	std::vector< VkLayerProperties > availableLayers( layerCount );
	vkEnumerateInstanceLayerProperties( &layerCount, availableLayers.data(  ) );
	
	for ( auto layerName : gpValidationLayers )
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

void GInstance::CreateWindow() 
{
	aWindow.aWidth  = cmdline->GetValue( "-w", GetOption( "Width"  ) ); 
	aWindow.aHeight = cmdline->GetValue( "-h", GetOption( "Height" ) );

	if ( SDL_Init( SDL_INIT_VIDEO | SDL_INIT_EVENTS | SDL_INIT_AUDIO ) != 0 )
		LogFatal( "SDL_Init Error: %s", SDL_GetError(  ) );

	aWindow.apWindow = SDL_CreateWindow( " - Chocolate Engine - Compiled on " __DATE__, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
				                   aWindow.aWidth, aWindow.aHeight, SDL_WINDOW_VULKAN | SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE );

	if ( !aWindow.apWindow )
		LogFatal( "SDL_CreateWindow Error: %s", SDL_GetError(  ) );
}

void GInstance::CreateInstance( void ) 
{
    if ( gEnableValidationLayers && !CheckValidationLayerSupport(  ) )
		LogFatal( "Validation layers requested, but not available!" );

	VkApplicationInfo appInfo{  };
	appInfo.sType 			    = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pApplicationName 	= "Test";
	appInfo.applicationVersion 	= VK_MAKE_VERSION( 1, 0, 0 );
	appInfo.pEngineName 		= "Chocolate";
	appInfo.engineVersion 		= VK_MAKE_VERSION( 1, 1, 0 );
	appInfo.apiVersion 		    = VK_API_VERSION_1_0;

	VkInstanceCreateInfo createInfo{  };
	createInfo.sType 		= VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	createInfo.pApplicationInfo 	= &appInfo;

	if ( gEnableValidationLayers )
	{
		createInfo.enabledLayerCount 	= ( unsigned int )ARR_SIZE( gpValidationLayers );
		createInfo.ppEnabledLayerNames 	= gpValidationLayers;
		createInfo.pNext 		        = ( VkDebugUtilsMessengerCreateInfoEXT* )&gLayerInfo;
	}
	else
	{
		createInfo.enabledLayerCount 	= 0;
		createInfo.pNext	            = NULL;
	}

	auto SDLExtensions = InitRequiredExtensions(  );
	
	createInfo.enabledExtensionCount 	= ( uint32_t )SDLExtensions.size(  );
	createInfo.ppEnabledExtensionNames 	= SDLExtensions.data(  );

	CheckVKResult( vkCreateInstance( &createInfo, NULL, &aInstance ), "Failed to create instance!" );

	unsigned int extensionCount = 0;
	vkEnumerateInstanceExtensionProperties( NULL, &extensionCount, NULL );
	
	std::vector< VkExtensionProperties > extensions( extensionCount );
	vkEnumerateInstanceExtensionProperties( NULL, &extensionCount, extensions.data(  ) );

	Print( "[Device] %d Vulkan extensions available:\n", extensionCount );

	for ( const auto& extension : extensions )
		Print( "\t%s\n", extension.extensionName );

	Print( "\n" );
}

std::vector< const char* > GInstance::InitRequiredExtensions()
{
    uint32_t extensionCount = 0;
	if ( !SDL_Vulkan_GetInstanceExtensions( aWindow.apWindow, &extensionCount, NULL ) )
		LogFatal( "Unable to query the number of Vulkan instance extensions\n" );

	/* Use the amount of extensions queried before to retrieve the names of the extensions.  */
    std::vector< const char * > extensions( extensionCount );

	if ( !SDL_Vulkan_GetInstanceExtensions( aWindow.apWindow, &extensionCount, extensions.data(  ) ) )
		LogFatal( "Unable to query the number of Vulkan instance extension names\n" );

	// Display names
	Print( "[Device] Found %d Vulkan extensions:\n", extensionCount );
	for ( int i = 0; i < extensionCount; ++i )
		Print( "\t%i : %s\n", i, extensions[ i ] );

	Print( "\n" );

	// Add debug display extension, we need this to relay debug messages
	extensions.push_back( VK_EXT_DEBUG_UTILS_EXTENSION_NAME );
	return extensions;
}

VkResult GInstance::CreateValidationLayers()
{
	auto func = ( PFN_vkCreateDebugUtilsMessengerEXT )vkGetInstanceProcAddr( aInstance, "vkCreateDebugUtilsMessengerEXT" );
	if ( func != NULL )
		return func( aInstance, &gLayerInfo, nullptr, &aLayers );
	else
		return VK_ERROR_EXTENSION_NOT_PRESENT;
}

GInstance::GInstance()
{
	CreateWindow();
    CreateInstance();
	if ( gEnableValidationLayers && CreateValidationLayers() != VK_SUCCESS )
		LogFatal( "Failed to create validation layers!" );

	if ( !SDL_Vulkan_CreateSurface( aWindow.apWindow, aInstance, &aSurface ) )
		LogFatal( "Failed to create Vulkan surface!" );
}

GInstance::~GInstance()
{
	vkDestroyInstance( aInstance, NULL );
	SDL_DestroyWindow( aWindow.apWindow );
	SDL_Quit();
}

GInstance &GetGInstance()
{
	static GInstance instance;
	return instance;
}