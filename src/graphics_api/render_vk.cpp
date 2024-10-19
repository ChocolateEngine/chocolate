// #include "../render.h"
#include "core/platform.h"
#include "core/filesystem.h"
#include "core/log.h"
#include "core/systemmanager.h"
#include "core/app_info.h"
#include "core/build_number.h"
#include "util.h"

#include "imgui_impl_sdl2.h"
#include "imgui_impl_vulkan.h"

#include "SDL_hints.h"

#include <unordered_map>
#include <filesystem>

#if CH_USE_MIMALLOC
  #include "mimalloc-new-delete.h"
#endif

#ifdef _WIN32
  #define WIN32_LEAN_AND_MEAN 1
#endif

#include "render_vk.h"

LOG_CHANNEL_REGISTER( GraphicsAPI, ELogColor_Cyan );
LOG_CHANNEL_REGISTER( Vulkan, ELogColor_DarkYellow );
LOG_CHANNEL_REGISTER( Validation, ELogColor_DarkYellow );

log_channel_h_t                                               gLC_Render = gLC_GraphicsAPI;

static VkCommandPool                                     gSingleTime;
static VkCommandPool                                     gPrimary;
static VkCommandPool                                     gCmdPoolTransfer;

struct ImGuiTexture
{
	VkDescriptorSet set;
	u32             refCount = 1;
};

static std::unordered_map< ch_handle_t, ImGuiTexture >    gImGuiTextures;

static std::vector< VkBuffer >                           gBuffers;

ResourceList< BufferVK >                                 gBufferHandles;
ResourceList< TextureVK* >                               gTextureHandles;
// static ResourceManager< BufferVK >        gBufferHandles;

static std::unordered_map< std::string, ch_handle_t >         gTexturePaths;
static std::unordered_map< ch_handle_t, TextureCreateData_t > gTextureInfo;

// Static Memory Pools for Vulkan Commands
static VkViewport*                                       gpViewports                = nullptr;
static u32                                               gMaxViewports              = 0;

Render_OnTextureIndexUpdate*                             gpOnTextureIndexUpdateFunc = nullptr;

bool                                                     gNeedTextureUpdate         = false;

GraphicsAPI_t                                            gGraphicsAPIData;

// debug thing
size_t                                                   gTotalBufferCopyPerFrame = 0;

// HACK HACK HACK HACK
// VULKAN NEEDS THE SURFACE BEFORE WE CREATE A DEVICE
VkSurfaceKHR                                             gSurfaceHack = VK_NULL_HANDLE;
SDL_Window*                                              gWindowHack  = nullptr;
void*                                                    gWindowHackSys  = nullptr;


// tracy contexts
#if TRACY_ENABLE
static std::unordered_map< VkCommandBuffer, TracyVkCtx > gTracyCtx;
#endif


CONVAR_BOOL( r_dbg_show_buffer_copy, 0, "Show all copies of the buffer" );


CONVAR_BOOL_CMD( r_msaa, 1, CVARF_ARCHIVE, "Enable/Disable MSAA Globally" )
{
	VK_ResetAll( ERenderResetFlags_MSAA );
}


CONVAR_INT_CMD( r_msaa_samples, 8, CVARF_ARCHIVE, "Set the Default Amount of MSAA Samples Globally" )
{
	if ( !r_msaa )
		return;

	VK_ResetAll( ERenderResetFlags_MSAA );
}


void VK_CheckCorruption()
{
	// VkResult result = vmaCheckCorruption( gVmaAllocator, 0 );
	// 
	// if ( result != VK_SUCCESS )
	// 	Log_FatalF( gLC_Render, "Vulkan Error: Corruption Detected: %s\n", VKString( result ) );
}


void VK_DumpCheckpoints()
{
#if NV_CHECKPOINTS
	if ( !pfnGetQueueCheckpointDataNV )
		return;

	u32 checkpointCount = 0;
	pfnGetQueueCheckpointDataNV( VK_GetGraphicsQueue(), &checkpointCount, 0 );

	ChVector< VkCheckpointDataNV > checkpoints( checkpointCount );  // { VK_STRUCTURE_TYPE_CHECKPOINT_DATA_NV }
	pfnGetQueueCheckpointDataNV( VK_GetGraphicsQueue(), &checkpointCount, checkpoints.data() );

	for ( auto& cp : checkpoints )
	{
		Log_DevF( gLC_Render, 1, "NV CHECKPOINT: stage %08x name %s\n", cp.stage, cp.pCheckpointMarker ? static_cast< const char* >( cp.pCheckpointMarker ) : "??" );
	}
#endif
}


void VK_CheckResultF( VkResult sResult, char const* spArgs, ... )
{
	if ( sResult == VK_SUCCESS )
		return;

	va_list args;
	va_start( args, spArgs );

	va_list copy;
	va_copy( copy, args );
	int len = std::vsnprintf( nullptr, 0, spArgs, copy );
	va_end( copy );

	if ( len < 0 )
	{
		Log_Error( "\n *** Sys_ExecuteV: vsnprintf failed?\n\n" );
		Log_FatalF( gLC_GraphicsAPI, "Vulkan Error: %s", VKString( sResult ) );
		return;
	}

	std::string argString;
	argString.resize( std::size_t( len ) + 1, '\0' );
	std::vsnprintf( argString.data(), argString.size(), spArgs, args );
	argString.resize( len );

	va_end( args );

	VK_CheckResult( sResult, argString.data() );
}


bool VK_CheckResultEF( VkResult sResult, char const* spArgs, ... )
{
	if ( sResult == VK_SUCCESS )
		return true;

	va_list args;
	va_start( args, spArgs );

	va_list copy;
	va_copy( copy, args );
	int len = std::vsnprintf( nullptr, 0, spArgs, copy );
	va_end( copy );

	if ( len < 0 )
	{
		Log_Error( "\n *** Sys_ExecuteV: vsnprintf failed?\n\n" );
		Log_ErrorF( gLC_GraphicsAPI, "Vulkan Error: %s", VKString( sResult ) );
		return false;
	}

	std::string argString;
	argString.resize( std::size_t( len ) + 1, '\0' );
	std::vsnprintf( argString.data(), argString.size(), spArgs, args );
	argString.resize( len );

	va_end( args );

	VK_CheckResultE( sResult, argString.data() );
}


void VK_CheckResult( VkResult sResult, char const* spMsg )
{
	if ( sResult == VK_SUCCESS )
		return;

	Log_FatalF( gLC_Render, "Vulkan Error: %s: %s", spMsg, VKString( sResult ) );
}


void VK_CheckResult( VkResult sResult )
{
	if ( sResult == VK_SUCCESS )
		return;

	Log_FatalF( gLC_Render, "Vulkan Error: %s", VKString( sResult ) );
}


// Non-Fatal Version
bool VK_CheckResultE( VkResult sResult, char const* spMsg )
{
	if ( sResult == VK_SUCCESS )
		return true;

	Log_ErrorF( gLC_Render, "Vulkan Error: %s: %s", spMsg, VKString( sResult ) );
	return false;
}


bool VK_CheckResultE( VkResult sResult )
{
	if ( sResult == VK_SUCCESS )
		return true;

	Log_ErrorF( gLC_Render, "Vulkan Error: %s", VKString( sResult ) );
	return false;
}


// Copies memory to the GPU.
void VK_memcpy( VkDeviceMemory sBufferMemory, VkDeviceSize sSize, const void* spData )
{
	#pragma message( "BE ABLE TO ONLY MAP CERTAIN REGIONS OF A BUFFER" )

	// maybe what you could do is manually map and unmap the memory? probably useless, idk

	// render->BufferMap();
	// render->BufferUnmap();

	// or you could copy the idea of VkBufferCopy and make your own buffer region writing stuff
	// and then internally we can automatically map the regions needed and write to it

	// TODO: add an option that doesn't need the size here
	void* pData = nullptr;
	VK_CheckResult( vkMapMemory( VK_GetDevice(), sBufferMemory, 0, sSize, 0, &pData ), "Failed to map memory" );
	memcpy( pData, spData, (size_t)sSize );
	vkUnmapMemory( VK_GetDevice(), sBufferMemory );
}


// Copies memory from the GPU to the host
void VK_memread( VkDeviceMemory sBufferMemory, VkDeviceSize sSize, void* spData )
{
	// TODO: add an option that doesn't need the size here
	void* pData = nullptr;
	VK_CheckResult( vkMapMemory( VK_GetDevice(), sBufferMemory, 0, sSize, 0, &pData ), "Failed to map memory" );
	memcpy( spData, pData, (size_t)sSize );
	vkUnmapMemory( VK_GetDevice(), sBufferMemory );
}


bool VK_UseMSAA()
{
	return r_msaa;
}


VkSampleCountFlagBits VK_GetMSAASamples()
{
	if ( !r_msaa )
		return VK_SAMPLE_COUNT_1_BIT;

	VkSampleCountFlags maxSamples = VK_GetMaxMSAASamples();

	if ( r_msaa_samples >= 64 && maxSamples & VK_SAMPLE_COUNT_64_BIT )
		return VK_SAMPLE_COUNT_64_BIT;

	if ( r_msaa_samples >= 32 && maxSamples & VK_SAMPLE_COUNT_32_BIT )
		return VK_SAMPLE_COUNT_32_BIT;

	if ( r_msaa_samples >= 16 && maxSamples & VK_SAMPLE_COUNT_16_BIT )
		return VK_SAMPLE_COUNT_16_BIT;

	if ( r_msaa_samples >= 8 && maxSamples & VK_SAMPLE_COUNT_8_BIT )
		return VK_SAMPLE_COUNT_8_BIT;

	if ( r_msaa_samples >= 4 && maxSamples & VK_SAMPLE_COUNT_4_BIT )
		return VK_SAMPLE_COUNT_4_BIT;

	if ( r_msaa_samples >= 2 && maxSamples & VK_SAMPLE_COUNT_2_BIT )
		return VK_SAMPLE_COUNT_2_BIT;

	Log_Warn( gLC_Render, "Minimum Sample Count with MSAA on is 2!\n" );
	return VK_SAMPLE_COUNT_2_BIT;
}


void VK_SetObjectNameEx( VkObjectType type, u64 handle, const char* name, const char* typeName )
{
	// add a debug label onto it
	if ( !pfnSetDebugUtilsObjectName || !name )
		return;

	std::string                         finalName = vstring( "%s - %s", typeName, name );

	const VkDebugUtilsObjectNameInfoEXT nameInfo = {
		VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,  // sType
		NULL,                                                // pNext
		type,                                                // objectType
		handle,                                              // objectHandle
        finalName.c_str(),                                   // pObjectName
	};

	VkResult result = pfnSetDebugUtilsObjectName( VK_GetDevice(), &nameInfo );

	if ( result == VK_SUCCESS )
	{
		Log_DevF( 2, "Set Name for Object \"%s\" - \"%s\"\n", VK_ObjectTypeStr( type ), name );
		return;
	}

	Log_ErrorF( gLC_Render, "Failed to Set Name for Object \"%s\" - \"%s\"\n", typeName, name );
}


void VK_SetObjectName( VkObjectType type, u64 handle, const char* name )
{
	VK_SetObjectNameEx( type, handle, name, VK_ObjectTypeStr( type ) );
}


void VK_CreateCommandPool( VkCommandPool& sCmdPool, u32 sQueueFamily, VkCommandPoolCreateFlags sFlags )
{
	// u32 graphics;
	// VK_FindQueueFamilies( VK_GetPhysicalDeviceProperties(), VK_GetPhysicalDevice(), &graphics, nullptr );

	VkCommandPoolCreateInfo aCommandPoolInfo{ VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };
	aCommandPoolInfo.pNext            = nullptr;
	aCommandPoolInfo.flags            = sFlags;
	aCommandPoolInfo.queueFamilyIndex = sQueueFamily;

	VK_CheckResult( vkCreateCommandPool( VK_GetDevice(), &aCommandPoolInfo, nullptr, &sCmdPool ), "Failed to create command pool!" );
}


void VK_DestroyCommandPool( VkCommandPool& srPool )
{
	vkDestroyCommandPool( VK_GetDevice(), srPool, nullptr );
}


void VK_ResetCommandPool( VkCommandPool& srPool, VkCommandPoolResetFlags sFlags )
{
	VK_CheckResult( vkResetCommandPool( VK_GetDevice(), srPool, sFlags ), "Failed to reset command pool!" );
}


VkCommandPool& VK_GetSingleTimeCommandPool()
{
	return gSingleTime;
}


VkCommandPool& VK_GetPrimaryCommandPool()
{
	return gPrimary;
}


VkCommandPool& VK_GetTransferCommandPool()
{
	return gCmdPoolTransfer;
}


void VK_CreateBuffer( BufferVK* spBuffer, VkBufferUsageFlags sUsage, VkMemoryPropertyFlags sMemBits )
{
	PROF_SCOPE();

	// create a buffer
	VkBufferCreateInfo aBufferInfo{ VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
	aBufferInfo.size        = spBuffer->aSize;
	aBufferInfo.usage       = sUsage;
	aBufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	VK_CheckResult( vkCreateBuffer( VK_GetDevice(), &aBufferInfo, nullptr, &spBuffer->aBuffer ), "Failed to create buffer" );

	VK_SetObjectName( VK_OBJECT_TYPE_BUFFER, (u64)spBuffer->aBuffer, spBuffer->apName );

	// allocate memory for the buffer
	VkMemoryRequirements aMemReqs;
	vkGetBufferMemoryRequirements( VK_GetDevice(), spBuffer->aBuffer, &aMemReqs );

	VkMemoryAllocateInfo aMemAllocInfo{ VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
	aMemAllocInfo.allocationSize  = aMemReqs.size;
	aMemAllocInfo.memoryTypeIndex = VK_GetMemoryType( aMemReqs.memoryTypeBits, sMemBits );

	VK_CheckResult( vkAllocateMemory( VK_GetDevice(), &aMemAllocInfo, nullptr, &spBuffer->aMemory ), "Failed to allocate buffer memory" );

	VK_SetObjectName( VK_OBJECT_TYPE_DEVICE_MEMORY, (u64)spBuffer->aMemory, spBuffer->apName );

	// bind the buffer to the device memory
	VK_CheckResult( vkBindBufferMemory( VK_GetDevice(), spBuffer->aBuffer, spBuffer->aMemory, 0 ), "Failed to bind buffer" );

	gBuffers.push_back( spBuffer->aBuffer );
}


void VK_CreateBuffer( const char* spName, BufferVK* spBuffer, VkBufferUsageFlags sUsage, VkMemoryPropertyFlags sMemBits )
{
	spBuffer->apName = spName;
	VK_CreateBuffer( spBuffer, sUsage, sMemBits );
}


void VK_DestroyBuffer( BufferVK* spBuffer )
{
	PROF_SCOPE();

	if ( !spBuffer )
		return;

	if ( spBuffer->aBuffer )
	{
		vec_remove_if( gBuffers, spBuffer->aBuffer );
		vkDestroyBuffer( VK_GetDevice(), spBuffer->aBuffer, nullptr );
	}

	if ( spBuffer->aMemory )
		vkFreeMemory( VK_GetDevice(), spBuffer->aMemory, nullptr );
}


bool VK_CreateImGuiFonts()
{
	// VkCommandBuffer c = VK_BeginOneTimeCommand();

	if ( !ImGui_ImplVulkan_CreateFontsTexture() )
	{
		Log_Error( gLC_Render, "VK_CreateImGuiFonts(): Failed to create ImGui Fonts Texture!\n" );
		return false;
	}

	// VK_EndOneTimeCommand( c );

	// ImGui_ImplVulkan_DestroyFontUploadObjects();

	// TODO: manually free FontData in ImGui fonts, why is it holding onto it and trying to free() it on shutdown?
	return true;
}


bool VK_InitImGui( VkRenderPass sRenderPass )
{
	ImGui_ImplVulkan_InitInfo initInfo{};
	initInfo.Instance        = VK_GetInstance();
	initInfo.PhysicalDevice  = VK_GetPhysicalDevice();
	initInfo.Device          = VK_GetDevice();
	initInfo.Queue           = VK_GetGraphicsQueue();
	initInfo.DescriptorPool  = VK_GetDescPool();
	initInfo.RenderPass      = sRenderPass;
	initInfo.MinImageCount   = VK_GetSurfaceImageCount();
	initInfo.ImageCount      = VK_GetSurfaceImageCount();
	initInfo.CheckVkResultFn = VK_CheckResult;

	RenderPassInfoVK* info   = VK_GetRenderPassInfo( sRenderPass );
	if ( info == nullptr )
	{
		Log_Error( gLC_Render, "Failed to find RenderPass info\n" );
		return false;
	}

	initInfo.MSAASamples = info->aUsesMSAA ? VK_GetMSAASamples() : VK_SAMPLE_COUNT_1_BIT;

	if ( !ImGui_ImplVulkan_Init( &initInfo ) )
		return false;

	return true;
}


// ----------------------------------------------------------------------------------
// Render Abstraction


void Render_Shutdown();

bool Render_CreateDevice( VkSurfaceKHR surface )
{
	if ( VK_GetDevice() != VK_NULL_HANDLE )
		return true;

	if ( !VK_SetupPhysicalDevice( surface ) )
	{
		Log_Error( "Failed to setup physical device\n" );
		return false;
	}

	VK_CreateDevice( surface );

	VK_CreateCommandPool( VK_GetSingleTimeCommandPool(), gGraphicsAPIData.aQueueFamilyGraphics );
	VK_CreateCommandPool( VK_GetPrimaryCommandPool(), gGraphicsAPIData.aQueueFamilyGraphics );
	VK_CreateCommandPool( VK_GetTransferCommandPool(), gGraphicsAPIData.aQueueFamilyTransfer );

	VK_CreateDescSets();
	VK_AllocateOneTimeCommands();

	const auto& deviceProps = VK_GetPhysicalDeviceProperties();

	gpViewports             = new VkViewport[ deviceProps.limits.maxViewports ];
	gMaxViewports           = deviceProps.limits.maxViewports;

	VK_CreateTextureSamplers();
	VK_CreateMissingTexture();

	return true;
}

bool Render_Init()
{
	if ( !VK_CreateInstance() )
	{
		Log_Error( gLC_Render, "Failed to create Vulkan Instance\n" );
		return false;
	}

	// stupid ugly hack
	if ( gWindowHack == nullptr )
	{
		Log_FatalF( "Forgot to call SetMainSurface before GraphicsAPI Init with SDL_Window* because vulkan funny\n" );
		return false;
	}

	gSurfaceHack = VK_CreateSurface( gWindowHackSys, gWindowHack );

	if ( gSurfaceHack == VK_NULL_HANDLE )
	{
		Log_Error( gLC_GraphicsAPI, "Failed to create surface hack\n" );
		return false;
	}

	if ( !Render_CreateDevice( gSurfaceHack ) )
	{
		Log_Error( gLC_GraphicsAPI, "Failed to create device\n" );
		return false;
	}

	if ( !KTX_Init() )
	{
		Log_Error( "Failed to init KTX Texture Loader\n" );
		return false;
	}

	Log_Msg( gLC_Render, "Loaded Vulkan Renderer\n" );

	return true;
}


void Render_Shutdown()
{
	if ( gpViewports )
		delete gpViewports;

	KTX_Shutdown();

	VK_DestroyShaders();

	VK_DestroyRenderTargets();
	VK_DestroyRenderPasses();

	VK_DestroyAllTextures();

	VK_FreeOneTimeCommands();
	VK_DestroyCommandPool( VK_GetSingleTimeCommandPool() );
	VK_DestroyCommandPool( VK_GetPrimaryCommandPool() );
	VK_DestroyCommandPool( VK_GetTransferCommandPool() );

	VK_DestroyDescSets();

	VK_DestroyInstance();
}


void VK_Reset( ch_handle_t windowHandle, WindowVK* window, ERenderResetFlags sFlags )
{
	VK_RecreateSwapchain( window );

	// recreate backbuffer
	VK_DestroyRenderTarget( window->backbuffer );

	VK_CreateBackBuffer( window );

	if ( window->onResetFunc )
		window->onResetFunc( windowHandle, sFlags );
}


void VK_ResetAll( ERenderResetFlags sFlags )
{
	if ( sFlags & ERenderResetFlags_MSAA )
	{
		VK_DestroyMainRenderPass();
		VK_CreateMainRenderPass();

		// Recreate ImGui's Vulkan Backend for this context
		ImGui_ImplVulkan_Shutdown();
		VK_InitImGui( VK_GetRenderPass() );
		VK_CreateImGuiFonts();
	}

	for ( u32 i = 0; i < gGraphicsAPIData.windows.GetHandleCount(); i++ )
	{
		ch_handle_t handle = gGraphicsAPIData.windows.GetHandleByIndex( i );
		WindowVK*  window = gGraphicsAPIData.windows.Get( handle );

		VK_Reset( handle, window, sFlags );
	}
}


void VK_SetCheckpoint( VkCommandBuffer c, const char* spName )
{
#if NV_CHECKPOINTS
	if ( pfnCmdSetCheckpointNV )
	{
		pfnCmdSetCheckpointNV( c, spName );
	}
#endif
}


// ----------------------------------------------------------------------------------
// Chocolate Engine Render Abstraction

static ImWchar gFontRanges[] = { 0x1, 0xFFFF, 0 };


class RenderVK : public IRender
{
public:
	// --------------------------------------------------------------------------------------------
	// ISystem Functions
	// --------------------------------------------------------------------------------------------

	bool Init() override
	{
		if ( !Render_Init() )
			return false;

		return true;
	}

	void Shutdown()
	{
		for ( u32 i = 0; i < gGraphicsAPIData.aBufferCopies.aCapacity; i++ )
		{
			QueuedBufferCopy_t& bufferCopy = gGraphicsAPIData.aBufferCopies.apData[ i ];

			if ( bufferCopy.apRegions )
				free( bufferCopy.apRegions );

			bufferCopy.apRegions = nullptr;
		}

		VK_WaitForGraphicsQueue();
		VK_WaitForTransferQueue();

		// shutdown all windows
		for ( u32 i = 0; i < gGraphicsAPIData.windows.GetHandleCount(); i++ )
		{
			WindowVK* window = nullptr;
			if ( !gGraphicsAPIData.windows.GetByIndex( i, &window ) )
				continue;
			
			DestroyWindowInternal( window );
		}

		gGraphicsAPIData.windows.clear();

		Render_Shutdown();
	}

	void Update( float sDT ) override
	{
	}

	void SetMainSurface( SDL_Window* window, void* sysWindow ) override
	{
		gWindowHack    = window;
		gWindowHackSys = sysWindow;
	}

	// --------------------------------------------------------------------------------------------
	// General Purpose Functions
	// --------------------------------------------------------------------------------------------

	// very odd
	bool InitImGui( ch_handle_t shRenderPass ) override
	{
		VkRenderPass renderPass = VK_GetRenderPass( shRenderPass );

		if ( renderPass == VK_NULL_HANDLE )
		{
			Log_Error( gLC_Render, "Failed to get RenderPass for ImGui\n" );
			Render_Shutdown();
			return false;
		}

		if ( !VK_InitImGui( renderPass ) )
		{
			Log_Error( gLC_Render, "Failed to init ImGui for Vulkan\n" );
			Render_Shutdown();
			return false;
		}

		return VK_CreateImGuiFonts();
	}

	void ShutdownImGui() override
	{
		ImGui_ImplVulkan_Shutdown();
	}

	void GetSurfaceSize( ch_handle_t windowHandle, int& srWidth, int& srHeight ) override
	{
		WindowVK* window = nullptr;
		if ( !gGraphicsAPIData.windows.Get( windowHandle, &window ) )
		{
			Log_ErrorF( gLC_GraphicsAPI, "Failed to find window!\n" );
			return;
		}

		srWidth  = window->swapExtent.width;
		srHeight = window->swapExtent.height;

		// SDL_GetWindowSize( window->window, &srWidth, &srHeight );
	}

	ImTextureID AddTextureToImGui( ch_handle_t sHandle ) override
	{
		// User wants the Missing Texture
		if ( sHandle == CH_INVALID_HANDLE )
			sHandle = gMissingTexHandle;

		// Is this already loaded?
		auto it = gImGuiTextures.find( sHandle );
		if ( it != gImGuiTextures.end() )
		{
			it->second.refCount++;
			return it->second.set;
		}

		TextureVK* tex = VK_GetTexture( sHandle );
		if ( tex == nullptr )
		{
			Log_Error( gLC_Render, "AddTextureToImGui(): No Vulkan Texture created for Image!\n" );
			return nullptr;
		}

		// imgui can't handle 2d array textures
		if ( tex->aRenderTarget || !( tex->aUsage & VK_IMAGE_USAGE_SAMPLED_BIT ) || tex->aViewType != VK_IMAGE_VIEW_TYPE_2D )
		// if ( !( tex->aUsage & VK_IMAGE_USAGE_SAMPLED_BIT ) || tex->aViewType != VK_IMAGE_VIEW_TYPE_2D )
		{
			return nullptr;
		}

		// VK_SetImageLayout( tex->aImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 1 );

		auto desc = ImGui_ImplVulkan_AddTexture( VK_GetSampler( tex->aFilter, VK_SAMPLER_ADDRESS_MODE_REPEAT, VK_FALSE ), tex->aImageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL );

		if ( desc )
		{
			gImGuiTextures[ sHandle ] = { desc, 1 };
			return desc;
		}

		Log_Error( "Render_AddTextureToImGui(): Failed to add texture to ImGui\n" );
		return nullptr;
	}

	void FreeTextureFromImGui( ch_handle_t sHandle ) override
	{
		if ( sHandle == CH_INVALID_HANDLE )
		{
			Log_Error( gLC_Render, "FreeTextureFromImGui(): Invalid Image Handle!\n" );
			return;
		}

		auto it = gImGuiTextures.find( sHandle );
		if ( it == gImGuiTextures.end() )
		{
			Log_Error( gLC_Render, "FreeTextureFromImGui(): Could not find ImGui TextureID!\n" );
			return;
		}

		if ( --it->second.refCount != 0 )
			return;

		ImGui_ImplVulkan_RemoveTexture( it->second.set );
		gImGuiTextures.erase( sHandle );
	}

	bool BuildFonts() override
	{
		return VK_CreateImGuiFonts();
	}

	int GetMaxMSAASamples() override
	{
		VkSampleCountFlags maxSamples = VK_GetMaxMSAASamples();

		if ( maxSamples & VK_SAMPLE_COUNT_64_BIT )
			return 64;

		if ( maxSamples & VK_SAMPLE_COUNT_32_BIT )
			return 32;

		if ( maxSamples & VK_SAMPLE_COUNT_16_BIT )
			return 16;

		if ( maxSamples & VK_SAMPLE_COUNT_8_BIT )
			return 8;

		if ( maxSamples & VK_SAMPLE_COUNT_4_BIT )
			return 4;

		if ( maxSamples & VK_SAMPLE_COUNT_2_BIT )
			return 2;

		return 1;
	}

	// --------------------------------------------------------------------------------------------
	// Windows
	// --------------------------------------------------------------------------------------------

	virtual ch_handle_t CreateWindow( SDL_Window* window, void* sysWindow ) override
	{
		WindowVK*   windowVK     = nullptr;
		ch_handle_t  windowHandle = gGraphicsAPIData.windows.Create( &windowVK );

		const char* title        = SDL_GetWindowTitle( window ) == nullptr ? SDL_GetWindowTitle( window ) : CH_DEFAULT_WINDOW_NAME;

		if ( !windowVK )
		{
			Log_ErrorF( gLC_GraphicsAPI, "Failed to allocate data for vulkan window: \"%s\"\n", title );
			return CH_INVALID_HANDLE;
		}

		windowVK->window = window;

		// fun
		if ( window == gWindowHack )
		{
			windowVK->surface           = gSurfaceHack;
			gGraphicsAPIData.mainWindow = windowHandle;
		}
		else
		{
			windowVK->surface = VK_CreateSurface( sysWindow, window );

			if ( windowVK->surface == VK_NULL_HANDLE )
			{
				Log_ErrorF( gLC_GraphicsAPI, "Failed to create surface for window: \"%s\"\n", title );
				gGraphicsAPIData.windows.Remove( windowHandle );
				return CH_INVALID_HANDLE;
			}
		}

		windowVK->context = ImGui::GetCurrentContext();

		if ( !VK_CreateSwapchain( windowVK ) )
		{
			Log_ErrorF( gLC_GraphicsAPI, "Failed to create swapchain for window: \"%s\"\n", title );
			DestroyWindow( windowHandle );
			return CH_INVALID_HANDLE;
		}

		VK_CreateBackBuffer( windowVK );
		VK_CreateFences( windowVK );
		VK_CreateSemaphores( windowVK );

		VK_AllocateCommands( windowVK );

		InitImGui( 0 );

		if ( !ImGui_ImplSDL2_InitForVulkan( window ) )
		{
			Log_ErrorF( gLC_Render, "Failed to init ImGui SDL2 for Vulkan Window: \"%s\"\n", title );
			DestroyWindow( windowHandle );
			return false;
		}

		if ( !VK_CreateImGuiFonts() )
		{
			Log_ErrorF( gLC_Render, "Failed to create ImGui fonts for Window: \"%s\"\n", title );
			DestroyWindow( windowHandle );
			return false;
		}

		return windowHandle;
	}

	void DestroyWindowInternal( WindowVK* window )
	{
		ImGuiContext* origContext = ImGui::GetCurrentContext();

		if ( window->context )
			ImGui::SetCurrentContext( window->context );

		if ( window->swapchain )
			VK_DestroySwapchain( window );

		if ( window->surface )
			VK_DestroySurface( window->surface );

		VK_DestroyRenderTarget( window->backbuffer );
		VK_DestroyFences( window );
		VK_DestroySemaphores( window );
		VK_FreeCommands( window );

		ImGui_ImplVulkan_Shutdown();

		ImGui::SetCurrentContext( origContext );
	}

	virtual void DestroyWindow( ch_handle_t windowHandle ) override
	{
		WindowVK* window = nullptr;
		if ( !gGraphicsAPIData.windows.Get( windowHandle, &window ) )
		{
			Log_ErrorF( gLC_GraphicsAPI, "Failed to find window to destroy!\n" );
			return;
		}

		DestroyWindowInternal( window );
		gGraphicsAPIData.windows.Remove( windowHandle );
	}

	// --------------------------------------------------------------------------------------------
	// Buffers
	// --------------------------------------------------------------------------------------------
	
	ch_handle_t CreateBuffer( const char* spName, u32 sSize, EBufferFlags sBufferFlags, EBufferMemory sBufferMem ) override
	{
		PROF_SCOPE();

		BufferVK* buffer = nullptr;
		ch_handle_t    handle = gBufferHandles.Create( &buffer );

		buffer->aBuffer  = VK_NULL_HANDLE;
		buffer->aMemory  = VK_NULL_HANDLE;
		buffer->aSize    = sSize;
		buffer->apName   = spName;

		int flagBits     = 0;

		if ( sBufferFlags & EBufferFlags_TransferSrc )
			flagBits |= VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

		if ( sBufferFlags & EBufferFlags_TransferDst )
			flagBits |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;

		if ( sBufferFlags & EBufferFlags_Uniform )
			flagBits |= VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;

		if ( sBufferFlags & EBufferFlags_Storage )
			flagBits |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;

		if ( sBufferFlags & EBufferFlags_Index )
			flagBits |= VK_BUFFER_USAGE_INDEX_BUFFER_BIT;

		if ( sBufferFlags & EBufferFlags_Vertex )
			flagBits |= VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;

		if ( sBufferFlags & EBufferFlags_Indirect )
			flagBits |= VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT;

		int memBits = 0;

		if ( sBufferMem & EBufferMemory_Device )
			memBits |= VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

		if ( sBufferMem & EBufferMemory_Host )
			memBits |= VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

		if ( flagBits == 0 )
		{
			Log_ErrorF( gLC_Render, "Tried to create buffer \"%s\" without any flags", spName ? spName : "UNNAMED" );
			return CH_INVALID_HANDLE;
		}

		if ( memBits == 0 )
		{
			Log_ErrorF( gLC_Render, "Tried to create buffer \"%s\" without any memory bits", spName ? spName : "UNNAMED" );
			return CH_INVALID_HANDLE;
		}

		VK_CreateBuffer( spName, buffer, flagBits, memBits );

		return handle;
	}
	
	ch_handle_t CreateBuffer( u32 sSize, EBufferFlags sBufferFlags, EBufferMemory sBufferMem ) override
	{
		return CreateBuffer( nullptr, sSize, sBufferFlags, sBufferMem );
	}

	void DestroyBuffer( ch_handle_t buffer ) override
	{
		BufferVK* bufVK = gBufferHandles.Get( buffer );

		if ( !bufVK )
		{
			Log_Error( gLC_Render, "DestroyBuffer: Failed to find Buffer\n" );
			return;
		}

		VK_DestroyBuffer( bufVK );

		gBufferHandles.Remove( buffer );
	}

	void DestroyBuffers( ch_handle_t* spBuffers, u32 sCount ) override
	{
		for ( u32 i = 0; i < sCount; i++ )
			DestroyBuffer( spBuffers[ i ] );
	}

	virtual u32 BufferWrite( ch_handle_t buffer, u32 sSize, void* spData ) override
	{
		PROF_SCOPE();

		BufferVK* bufVK = gBufferHandles.Get( buffer );

		if ( !bufVK )
		{
			Log_ErrorF( gLC_Render, "BufferWrite: Failed to find Buffer (Handle: %zd)\n", buffer );
			return 0;
		}

		if ( sSize > bufVK->aSize )
		{
			Log_WarnF( gLC_Render, "BufferWrite: Trying to write more data than buffer size can store (data size: %zd > buffer size: %zd)\n", sSize, bufVK->aSize );
			VK_memcpy( bufVK->aMemory, bufVK->aSize, spData );
			return bufVK->aSize;
		}

		// VK_memcpy( bufVK->aMemory, bufVK->aSize, spData );
		VK_memcpy( bufVK->aMemory, sSize, spData );
		return bufVK->aSize;
	}

	virtual u32 BufferRead( ch_handle_t buffer, u32 sSize, void* spData ) override
	{
		PROF_SCOPE();

		BufferVK* bufVK = gBufferHandles.Get( buffer );

		if ( !bufVK )
		{
			Log_ErrorF( gLC_Render, "BufferRead: Failed to find Buffer (Handle: %zd)\n", buffer );
			return 0;
		}

		if ( sSize > bufVK->aSize )
		{
			Log_WarnF( gLC_Render, "BufferRead: Trying to write more data than buffer size can store (data size: %zd > buffer size: %zd)\n", sSize, bufVK->aSize );
			VK_memread( bufVK->aMemory, bufVK->aSize, spData );
			return sSize;
		}

		VK_memread( bufVK->aMemory, sSize, spData );
		return bufVK->aSize;
	}

	virtual bool BufferCopy( ch_handle_t shSrc, ch_handle_t shDst, BufferRegionCopy_t* spRegions, u32 sRegionCount ) override
	{
		PROF_SCOPE();

		if ( spRegions == nullptr || sRegionCount == 0 )
		{
			Log_Error( gLC_Render, "BufferCopy: No Regions to Copy\n" );
			return false;
		}

		BufferVK* bufSrc = gBufferHandles.Get( shSrc );
		if ( !bufSrc )
		{
			Log_Error( gLC_Render, "BufferCopy: Failed to find Source Buffer\n" );
			return false;
		}

		BufferVK* bufDst = gBufferHandles.Get( shDst );
		if ( !bufDst )
		{
			Log_Error( gLC_Render, "BufferCopy: Failed to find Dest Buffer\n" );
			return false;
		}

		auto regions = CH_STACK_NEW( VkBufferCopy, sRegionCount );

		for ( u32 i = 0; i < sRegionCount; i++ )
		{
			BufferRegionCopy_t& region = spRegions[ i ];
			u32                 size   = region.aSize;

			if ( size > bufSrc->aSize )
			{
				size = bufSrc->aSize;
			}

			if ( size > bufDst->aSize )
			{
				size = bufDst->aSize;
			}

			regions[ i ].srcOffset = region.aSrcOffset;
			regions[ i ].dstOffset = region.aDstOffset;
			regions[ i ].size      = size;

			gTotalBufferCopyPerFrame += size;
		}

		// TODO: PERF: only use the transfer queue if you need to copy from host to device local memory
		// this is slower than the graphics queue when copying from device local to device local
		VkCommandBuffer c = VK_BeginOneTimeTransferCommand();
		vkCmdCopyBuffer( c, bufSrc->aBuffer, bufDst->aBuffer, sRegionCount, regions );
		VK_EndOneTimeTransferCommand( c );

		CH_STACK_FREE( regions );

		return true;
	}

	virtual bool BufferCopyQueued( ch_handle_t shSrc, ch_handle_t shDst, BufferRegionCopy_t* spRegions, u32 sRegionCount ) override
	{
		PROF_SCOPE();

		if ( spRegions == nullptr || sRegionCount == 0 )
		{
			Log_Error( gLC_Render, "BufferCopy: No Regions to Copy\n" );
			return false;
		}

		BufferVK* bufSrc = gBufferHandles.Get( shSrc );
		if ( !bufSrc )
		{
			Log_Error( gLC_Render, "BufferCopy: Failed to find Source Buffer\n" );
			return false;
		}

		BufferVK* bufDst = gBufferHandles.Get( shDst );
		if ( !bufDst )
		{
			Log_Error( gLC_Render, "BufferCopy: Failed to find Dest Buffer\n" );
			return false;
		}

		QueuedBufferCopy_t& bufferCopy = gGraphicsAPIData.aBufferCopies.emplace_back( false );
		bufferCopy.aSource             = bufSrc->aBuffer;
		bufferCopy.aDest               = bufDst->aBuffer;

		if ( bufferCopy.aRegionCount < sRegionCount && bufferCopy.apRegions )
		{
			// from an earlier copy, we need more memory allocated here, so free it and allocate more after
			free( bufferCopy.apRegions );
			bufferCopy.apRegions = nullptr;
		}

		bufferCopy.aRegionCount = sRegionCount;

		if ( !bufferCopy.apRegions )
			bufferCopy.apRegions = ch_malloc< VkBufferCopy >( sRegionCount );

		for ( u32 i = 0; i < sRegionCount; i++ )
		{
			BufferRegionCopy_t& region = spRegions[ i ];
			u32                 size   = region.aSize;

			if ( size > bufSrc->aSize )
			{
				size = bufSrc->aSize;
			}

			if ( size > bufDst->aSize )
			{
				size = bufDst->aSize;
			}

			bufferCopy.apRegions[ i ].srcOffset = region.aSrcOffset;
			bufferCopy.apRegions[ i ].dstOffset = region.aDstOffset;
			bufferCopy.apRegions[ i ].size      = size;
		}

		return true;
	}

	const char* BufferGetName( ch_handle_t bufferHandle ) override
	{
		BufferVK* buffer = gBufferHandles.Get( bufferHandle );
		if ( !buffer )
		{
			Log_Error( gLC_Render, "BufferGetName: Failed to find Source Buffer\n" );
			return nullptr;
		}

		return buffer->apName;
	}

	// --------------------------------------------------------------------------------------------
	// Materials and Textures
	// --------------------------------------------------------------------------------------------

	ch_handle_t LoadTexture( ch_handle_t& srHandle, const std::string& srTexturePath, const TextureCreateData_t& srCreateData ) override
	{
		// wants missing texture
		if ( srTexturePath.empty() )
		{
			srHandle = CH_INVALID_HANDLE;
			return CH_INVALID_HANDLE;
		}

		if ( srHandle == CH_INVALID_HANDLE )
		{
			auto it = gTexturePaths.find( srTexturePath );
			if ( it != gTexturePaths.end() && it->second != CH_INVALID_HANDLE )
			{
				srHandle = it->second;
				gGraphicsAPIData.aTextureRefs[ srHandle ]++;
				return srHandle;
			}
		}

		ch_string fullPath;

		// we only support ktx right now
		if ( srTexturePath.ends_with( ".ktx" ) )
		{
			fullPath = FileSys_FindFile( srTexturePath.data(), srTexturePath.size() );
		}
		else
		{
			const char*    strings[] = { srTexturePath.data(), ".ktx" };
			const u64      lengths[] = { srTexturePath.size(), 4 };
			ch_string_auto path      = ch_str_join( 2, strings, lengths );

			fullPath                 = FileSys_FindFile( path.data, path.size );
		}

		if ( !fullPath.data )
		{
			// add it to the paths anyway, if you do a texture reload, then maybe the texture will have been added
			// TODO: We should probably allocate a handle and a new texture anyway,
			// so if it appears on the disk, we can use it in hotloading later
			gTexturePaths[ srTexturePath ] = CH_INVALID_HANDLE;
			Log_ErrorF( gLC_Render, "Failed to find Texture: \"%s\"\n", srTexturePath.c_str() );
			return CH_INVALID_HANDLE;
		}

		if ( srHandle )
		{
#pragma message( "TODO: HANDLE UPDATING IMGUI TEXTURES WHEN REPLACING THE DATA IN THE HANDLE" )

			// free old texture data
			TextureVK* tex = nullptr;
			if ( !gTextureHandles.Get( srHandle, &tex ) )
			{
				Log_Error( gLC_Render, "Failed to find old texture\n" );
				ch_str_free( fullPath.data );
				return CH_INVALID_HANDLE;
			}

			if ( tex->aImageView )
				vkDestroyImageView( VK_GetDevice(), tex->aImageView, nullptr );

			if ( tex->aMemory )
			{
				if ( tex->aImage )
					vkDestroyImage( VK_GetDevice(), tex->aImage, nullptr );

				vkFreeMemory( VK_GetDevice(), tex->aMemory, nullptr );
			}

			if ( !VK_LoadTexture( srHandle, tex, fullPath, srCreateData ) )
			{
				VK_DestroyTexture( srHandle );
				ch_str_free( fullPath.data );
				return CH_INVALID_HANDLE;
			}

			// write new texture data
			if ( !gTextureHandles.Update( srHandle, tex ) )
			{
				Log_ErrorF( gLC_Render, "Failed to Update Texture Handle: \"%s\"\n", srTexturePath.c_str() );
				gGraphicsAPIData.aTextureRefs.erase( srHandle );
				ch_str_free( fullPath.data );
				return CH_INVALID_HANDLE;
			}
		}
		else
		{
			TextureVK* tex = VK_NewTexture( srHandle );
			if ( !VK_LoadTexture( srHandle, tex, fullPath, srCreateData ) )
			{
				VK_DestroyTexture( srHandle );
				ch_str_free( fullPath.data );
				return CH_INVALID_HANDLE;
			}

			gTexturePaths[ srTexturePath ]            = srHandle;
			gTextureInfo[ srHandle ]                  = srCreateData;
			gGraphicsAPIData.aTextureRefs[ srHandle ] = 1;
		}

		ch_str_free( fullPath.data );
		return srHandle;
	}

	ch_handle_t CreateTexture( const TextureCreateInfo_t& srTextureCreateInfo, const TextureCreateData_t& srCreateData ) override
	{
		ch_handle_t handle = CH_INVALID_HANDLE;
		TextureVK* tex    = VK_CreateTexture( handle, srTextureCreateInfo, srCreateData );
		if ( tex == nullptr )
			return CH_INVALID_HANDLE;

		gTextureInfo[ handle ]                  = srCreateData;
		gGraphicsAPIData.aTextureRefs[ handle ] = 1;
		return handle;
	}

	void FreeTexture( ch_handle_t sTexture ) override
	{
		if ( sTexture == gMissingTexHandle )
			return;

		gGraphicsAPIData.aTextureRefs[ sTexture ]--;
		if ( gGraphicsAPIData.aTextureRefs[ sTexture ] > 0 )
			return;

		VK_DestroyTexture( sTexture );
		gTextureInfo.erase( sTexture );
		gGraphicsAPIData.aTextureRefs.erase( sTexture );

		for ( auto& [ path, handle ] : gTexturePaths )
		{
			if ( sTexture == handle )
			{
				gTexturePaths.erase( path );
				break;
			}
		}
	}

	// EImageUsage GetTextureUsage( ch_handle_t shTexture ) override
	// {
	// 	VK_ToImageUsage();
	// }

	int GetTextureIndex( ch_handle_t shTexture ) override
	{
		PROF_SCOPE();

		if ( shTexture == CH_INVALID_HANDLE )
			return 0;

		TextureVK* tex = nullptr;
		if ( !gTextureHandles.Get( shTexture, &tex ) )
		{
			Log_Error( gLC_Render, "GetTextureIndex: Failed to find texture\n" );
			return 0;
		}

		if ( gNeedTextureUpdate )
			VK_UpdateImageSets();

		gNeedTextureUpdate = false;

		return tex->aIndex;
	}

	GraphicsFmt GetTextureFormat( ch_handle_t shTexture ) override
	{
		PROF_SCOPE();

		TextureVK* tex = VK_GetTexture( shTexture );

		if ( tex )
			return VK_ToGraphicsFmt( tex->aFormat );

		Log_ErrorF( gLC_Render, "GetTextureFormat: Failed to find texture %zd\n", shTexture );
		return GraphicsFmt::INVALID;
	}
	
	glm::uvec2 GetTextureSize( ch_handle_t shTexture ) override
	{
		TextureVK* tex = VK_GetTexture( shTexture );

		if ( tex )
			return tex->aSize;

		Log_ErrorF( gLC_Render, "GetTextureSize: Failed to find texture %zd\n", shTexture );
		return {};
	}

	void ReloadTextures() override
	{
		VK_WaitForGraphicsQueue();
		VK_WaitForTransferQueue();

		for ( auto& [ path, handle ] : gTexturePaths )
		{
			LoadTexture( handle, path, gTextureInfo[ handle ] );
		}
	}
	
	const std::vector< ch_handle_t >& GetTextureList() override
	{
		return gTextureHandles.aHandles;
	}
	
	TextureInfo_t GetTextureInfo( ch_handle_t sTexture ) override
	{
		TextureInfo_t info;

		TextureVK*    tex = VK_GetTexture( sTexture );
		if ( !tex )
		{
			Log_Error( gLC_Render, "GetTextureInfo: Failed to find texture\n" );
			return info;
		}

		if ( gNeedTextureUpdate )
			VK_UpdateImageSets();

		gNeedTextureUpdate = false;

		info.aFormat   = VK_ToGraphicsFmt( tex->aFormat );

		if ( tex->name.data )
			info.name = ch_str_copy( tex->name.data, tex->name.size );

		info.aSize         = tex->aSize;
		info.aGpuIndex     = tex->aIndex;
		info.aMemoryUsage  = tex->aMemorySize;
		info.aRefCount     = gGraphicsAPIData.aTextureRefs[ sTexture ];
		// info.aUsage        = tex->aUsage;
		info.aRenderTarget = tex->aRenderTarget || tex->aSwapChain;
		// info.aViewType = tex->aViewType

		for ( auto& [ path, handle ] : gTexturePaths )
		{
			if ( handle == sTexture )
			{
				info.aPath = ch_str_copy( path.data(), path.size() );

				if ( !info.name.data )
				{
					info.name = FileSys_GetFileName( path.data(), path.size() );
				}

				break;
			}
		}

		return info;
	}

	void FreeTextureInfo( TextureInfo_t& srInfo ) override
	{
		if ( srInfo.name.data )
			ch_str_free( srInfo.name.data );

		if ( srInfo.aPath.data )
			ch_str_free( srInfo.aPath.data );
	}

	void Screenshot()
	{
		// VkCommandBuffer c = VK_BeginOneTimeCommand();
		// 
		// // blit the image (TODO: not all devices support this)
		// 
		// 
		// 
		// VK_EndOneTimeCommand( c );
	}

	ReadTexture ReadTextureFromDevice( ch_handle_t textureHandle ) override
	{
		TextureVK*  tex = VK_GetTexture( textureHandle );

		// if ( tex->aRenderTarget || !( tex->aUsage & VK_IMAGE_USAGE_SAMPLED_BIT ) || tex->aViewType != VK_IMAGE_VIEW_TYPE_2D )
		// // if ( !( tex->aUsage & VK_IMAGE_USAGE_SAMPLED_BIT ) || tex->aViewType != VK_IMAGE_VIEW_TYPE_2D )
		// {
		// 	return;
		// }

		// probably an awful way of doing this

		ReadTexture readTexture;
		readTexture.size             = tex->aSize;
		readTexture.dataSize         = tex->aSize.x * tex->aSize.y * sizeof( u32 );
		readTexture.pData            = new u32[ tex->aSize.x * tex->aSize.y ];

		// Copy to a buffer
		ch_handle_t        tempBuffer = CreateBuffer( "READ TEXTURE BUFFER", readTexture.dataSize, EBufferFlags_TransferDst, EBufferMemory_Host );
		BufferVK*         bufVK      = gBufferHandles.Get( tempBuffer );

		// Get layout of the image (including row pitch)
		// VkImageSubresource  subResource{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 0 };
		// VkSubresourceLayout subResourceLayout;
		// vkGetImageSubresourceLayout( VK_GetDevice(), tex->aImage, &subResource, &subResourceLayout );

		// do i use the transfer queue or graphics queue?
		VkCommandBuffer   c          = VK_BeginOneTimeTransferCommand();

		VkBufferImageCopy bufferCopy{};
		bufferCopy.imageSubresource   = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 };
		bufferCopy.bufferImageHeight  = tex->aSize.y;
		bufferCopy.imageExtent.width  = tex->aSize.x;
		bufferCopy.imageExtent.height = tex->aSize.y;
		bufferCopy.imageExtent.depth  = 1;

		// TODO: PERF: change the layout to VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL according to vulkan
		vkCmdCopyImageToBuffer( c, tex->aImage, VK_IMAGE_LAYOUT_GENERAL, bufVK->aBuffer, 1, &bufferCopy );

		VK_EndOneTimeTransferCommand( c );

		BufferRead( tempBuffer, readTexture.dataSize, readTexture.pData );

		DestroyBuffer( tempBuffer );

#if 0
		// Get layout of the image (including row pitch)
		VkImageSubresource  subResource{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 0 };
		VkSubresourceLayout subResourceLayout;
		vkGetImageSubresourceLayout( VK_GetDevice(), tex->aImage, &subResource, &subResourceLayout );

		// Map image memory so we can start copying from it

		vkMapMemory( VK_GetDevice(), tex->aMemory, 0, tex->aMemorySize, 0, (void**)&readTexture.pData );
		readTexture.pData += subResourceLayout.offset;

		// readTexture.dataSize = tex->aMemorySize;
		
		// Clean up resources
		vkUnmapMemory( VK_GetDevice(), tex->aMemory );
		vkFreeMemory( VK_GetDevice(), tex->aMemory, nullptr );
		vkDestroyImage( VK_GetDevice(), tex->aImage, nullptr );
#endif

		return readTexture;
	}

	void FreeReadTexture( ReadTexture* pData ) override
	{
		if ( !pData )
			return;

		delete[] pData->pData;
	}

	ch_handle_t CreateFramebuffer( const CreateFramebuffer_t& srCreate ) override
	{
		return VK_CreateFramebuffer( srCreate );
	}

	void DestroyFramebuffer( ch_handle_t shTarget ) override
	{
		if ( shTarget == CH_INVALID_HANDLE )
			return;

		VK_DestroyFramebuffer( shTarget );
	}

	glm::uvec2 GetFramebufferSize( ch_handle_t shFramebuffer ) override
	{
		if ( shFramebuffer == CH_INVALID_HANDLE )
			return {};

		return VK_GetFramebufferSize( shFramebuffer );
	}

	void SetTextureDescSet( ch_handle_t* spDescSets, int sCount, u32 sBinding ) override
	{
		VK_SetImageSets( spDescSets, sCount, sBinding );
	}

	// --------------------------------------------------------------------------------------------
	// Shader System
	// --------------------------------------------------------------------------------------------

	bool CreatePipelineLayout( ch_handle_t& sHandle, PipelineLayoutCreate_t& srPipelineCreate ) override
	{
		return VK_CreatePipelineLayout( sHandle, srPipelineCreate );
	}

	bool CreateGraphicsPipeline( ch_handle_t& sHandle, GraphicsPipelineCreate_t& srGraphicsCreate ) override
	{
		return VK_CreateGraphicsPipeline( sHandle, srGraphicsCreate );
	}

	bool CreateComputePipeline( ch_handle_t& sHandle, ComputePipelineCreate_t& srComputeCreate ) override
	{
		return VK_CreateComputePipeline( sHandle, srComputeCreate );
	}

	void DestroyPipeline( ch_handle_t sPipeline ) override
	{
		VK_DestroyPipeline( sPipeline );
	}

	void DestroyPipelineLayout( ch_handle_t sPipeline ) override
	{
		VK_DestroyPipelineLayout( sPipeline );
	}

	// --------------------------------------------------------------------------------------------
	// Descriptor Pool
	// --------------------------------------------------------------------------------------------

	ch_handle_t CreateDescLayout( const CreateDescLayout_t& srCreate ) override
	{
		return VK_CreateDescLayout( srCreate );
	}

	void UpdateDescSets( WriteDescSet_t* spUpdate, u32 sCount ) override
	{
		VK_UpdateDescSets( spUpdate, sCount );
	}
	
	bool AllocateVariableDescLayout( const AllocVariableDescLayout_t& srCreate, ch_handle_t* handles ) override
	{
		return VK_AllocateVariableDescLayout( srCreate, handles );
	}
	
	bool AllocateDescLayout( const AllocDescLayout_t& srCreate, ch_handle_t* handles ) override
	{
		return VK_AllocateDescLayout( srCreate, handles );
	}

	// --------------------------------------------------------------------------------------------
	// Back Buffer Info
	// --------------------------------------------------------------------------------------------

	GraphicsFmt GetSwapFormatColor( ch_handle_t windowHandle ) override
	{
		WindowVK* window = nullptr;
		if ( !gGraphicsAPIData.windows.Get( windowHandle, &window ) )
		{
			Log_ErrorF( gLC_GraphicsAPI, "Failed to find window!\n" );
			return GraphicsFmt::INVALID;
		}

		return VK_ToGraphicsFmt( window->swapSurfaceFormat.format );
	}

	GraphicsFmt GetSwapFormatDepth() override
	{
		return VK_ToGraphicsFmt( VK_FORMAT_D32_SFLOAT );
	}

	ch_handle_t GetBackBufferColor( ch_handle_t windowHandle ) override
	{
		WindowVK* window = nullptr;
		if ( !gGraphicsAPIData.windows.Get( windowHandle, &window ) )
		{
			Log_ErrorF( gLC_GraphicsAPI, "Failed to find window to destroy!\n" );
			return CH_INVALID_HANDLE;
		}

		if ( !window->backbuffer )
		{
			Log_Error( gLC_Render, "No Backbuffer????\n" );
			return CH_INVALID_HANDLE;
		}

		return VK_GetFramebufferHandle( window->backbuffer->aFrameBuffers[ 0 ].aBuffer );
	}

	ch_handle_t GetBackBufferDepth( ch_handle_t windowHandle ) override
	{
		WindowVK* window = nullptr;
		if ( !gGraphicsAPIData.windows.Get( windowHandle, &window ) )
		{
			Log_ErrorF( gLC_GraphicsAPI, "Failed to find window!\n" );
			return CH_INVALID_HANDLE;
		}
		
		if ( !window->backbuffer )
		{
			Log_Error( gLC_Render, "No Backbuffer????\n" );
			return CH_INVALID_HANDLE;
		}

		return VK_GetFramebufferHandle( window->backbuffer->aFrameBuffers[ 1 ].aBuffer );
	}

	// --------------------------------------------------------------------------------------------
	// Rendering
	// --------------------------------------------------------------------------------------------

	void SetResetCallback( ch_handle_t windowHandle, Render_OnReset_t sFunc ) override
	{
		WindowVK* window = nullptr;
		if ( !gGraphicsAPIData.windows.Get( windowHandle, &window ) )
		{
			Log_ErrorF( gLC_GraphicsAPI, "Failed to find window!\n" );
			return;
		}

		window->onResetFunc = sFunc;
	}

	void SetTextureIndexUpdateCallback( Render_OnTextureIndexUpdate* spFunc ) override
	{
		gpOnTextureIndexUpdateFunc = spFunc;
	}

	void NewFrame() override
	{
		ImGui_ImplVulkan_NewFrame();
	}

	void Reset( ch_handle_t windowHandle ) override
	{
		WindowVK* window = nullptr;
		if ( !gGraphicsAPIData.windows.Get( windowHandle, &window ) )
		{
			Log_ErrorF( gLC_GraphicsAPI, "Failed to find window to reset!\n" );
			return;
		}

		VK_Reset( windowHandle, window );
	}

	void PreRenderPass() override
	{
	}

	void CopyQueuedBuffers() override
	{
		VkCommandBuffer c = VK_BeginOneTimeTransferCommand();

		if ( r_dbg_show_buffer_copy )
			Log_DevF( gLC_Render, 1, "Copy Queued Buffers:\n" );

		size_t totalSize = 0;
		for ( QueuedBufferCopy_t& bufferCopy : gGraphicsAPIData.aBufferCopies )
		{
			vkCmdCopyBuffer( c, bufferCopy.aSource, bufferCopy.aDest, bufferCopy.aRegionCount, bufferCopy.apRegions );

			// calc size
			size_t size = 0;

			for ( u32 region = 0; region < bufferCopy.aRegionCount; region++ )
			{
				size += bufferCopy.apRegions[ region ].size;
			}

			if ( r_dbg_show_buffer_copy )
				Log_DevF( gLC_Render, 1, "  Size: %.6f KB\n", Util_BytesToKB( size ) );

			totalSize += size;
		}

		VK_EndOneTimeTransferCommand( c );

		if ( r_dbg_show_buffer_copy )
			Log_DevF( gLC_Render, 1, "Total Copy Size: %.6f KB\n", Util_BytesToKB( totalSize ) );

		gTotalBufferCopyPerFrame += totalSize;

		gGraphicsAPIData.aBufferCopies.clear();
	}

	u32 GetFlightImageIndex( ch_handle_t windowHandle ) override
	{
		WindowVK* window = nullptr;
		if ( !gGraphicsAPIData.windows.Get( windowHandle, &window ) )
		{
			Log_ErrorF( gLC_GraphicsAPI, "Failed to find window!\n" );
			return UINT32_MAX;
		}

		return VK_GetNextImage( windowHandle, window );
	}

	void Present( ch_handle_t windowHandle, u32 sImageIndex ) override
	{
		WindowVK* window = nullptr;
		if ( !gGraphicsAPIData.windows.Get( windowHandle, &window ) )
		{
			Log_ErrorF( gLC_GraphicsAPI, "Failed to find window!\n" );
			return;
		}

		VK_Present( windowHandle, window, sImageIndex );

		if ( r_dbg_show_buffer_copy )
			Log_DevF( gLC_Render, 1, "Total Buffer Copy Per Frame: %.6f KB\n", Util_BytesToKB( gTotalBufferCopyPerFrame ) );

		gTotalBufferCopyPerFrame = 0;
	}

	void WaitForQueues() override
	{
		PROF_SCOPE();

		VK_WaitForGraphicsQueue();
		VK_WaitForTransferQueue();
	}

	void ResetCommandPool() override
	{
		PROF_SCOPE();

		VK_ResetCommandPool( VK_GetPrimaryCommandPool() );
	}

	// blech again
	u32 GetCommandBufferHandles( ch_handle_t windowHandle, ch_handle_t* spHandles ) override
	{
		WindowVK* window = nullptr;
		if ( !gGraphicsAPIData.windows.Get( windowHandle, &window ) )
		{
			Log_ErrorF( gLC_GraphicsAPI, "Failed to find window!\n" );
			return 0;
		}

		if ( spHandles != nullptr )
		{
			for ( size_t i = 0; i < window->swapImageCount; i++ )
			{
				spHandles[ i ] = window->commandBufferHandles[ i ];
			}
		}

		return window->swapImageCount;
	}

#if 0
	ch_handle_t AllocCommandBuffer() override
	{
		return CH_INVALID_HANDLE;
	}

	void FreeCommandBuffer( ch_handle_t cmd ) override
	{
	}
#endif

	void BeginCommandBuffer( ch_handle_t cmd ) override
	{
		PROF_SCOPE();

		VkCommandBuffer c = VK_GetCommandBuffer( cmd );

		// TODO: expose this
		VkCommandBufferBeginInfo begin{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
		begin.pNext            = nullptr;
		begin.flags            = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
		begin.pInheritanceInfo = nullptr;

		VK_CheckResult( vkBeginCommandBuffer( c, &begin ), "Failed to begin command buffer" );

		// gTracyCtx[ c ] = TracyVkContext( VK_GetPhysicalDevice(), VK_GetDevice(), VK_GetGraphicsQueue(), c );
	}

	void EndCommandBuffer( ch_handle_t cmd ) override
	{
		PROF_SCOPE();

		VkCommandBuffer c = VK_GetCommandBuffer( cmd );

		VK_CheckResult( vkEndCommandBuffer( c ), "Failed to end command buffer" );

		// auto it = gTracyCtx.find( c );
		// if ( it == gTracyCtx.end() )
		// 	return;
		// 
		// TracyVkDestroy( it->second );
		// gTracyCtx.erase( it );
	}

	ch_handle_t CreateRenderPass( const RenderPassCreate_t& srCreate ) override
	{
		return VK_CreateRenderPass( srCreate );
	}

	void DestroyRenderPass( ch_handle_t shPass ) override
	{
		VK_DestroyRenderPass( shPass );
	}

	void BeginRenderPass( ch_handle_t cmd, const RenderPassBegin_t& srBegin ) override
	{
		PROF_SCOPE();

		VkCommandBuffer c = VK_GetCommandBuffer( cmd );

		if ( c == nullptr )
		{
			Log_Error( gLC_Render, "BeginRenderPass: Invalid Command Buffer\n" );
			return;
		}

		VkRenderPassBeginInfo renderPassBeginInfo{ VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO };
		renderPassBeginInfo.pNext             = nullptr;
		renderPassBeginInfo.renderPass        = VK_GetRenderPass( srBegin.aRenderPass );
		renderPassBeginInfo.framebuffer       = VK_GetFramebuffer( srBegin.aFrameBuffer );
		renderPassBeginInfo.renderArea.offset = { 0, 0 };  // TODO: ADD renderArea offset TO RenderPassBegin_t

		glm::uvec2 size = VK_GetFramebufferSize( srBegin.aFrameBuffer );

		renderPassBeginInfo.renderArea.extent = { size.x, size.y };

		std::vector< VkClearValue > clearValues( srBegin.aClear.size() );
		if ( srBegin.aClear.size() )
		{
			for ( size_t i = 0; i < srBegin.aClear.size(); i++ )
			{
				if ( srBegin.aClear[ i ].aIsDepth )
				{
					clearValues[ i ].depthStencil.depth   = srBegin.aClear[ i ].color.w;
					clearValues[ i ].depthStencil.stencil = srBegin.aClear[ i ].aStencil;
				}
				else
				{
					clearValues[ i ].color = {
						srBegin.aClear[ i ].color.x,
						srBegin.aClear[ i ].color.y,
						srBegin.aClear[ i ].color.z,
						srBegin.aClear[ i ].color.w
					};
				}
			}

			renderPassBeginInfo.clearValueCount = static_cast< u32 >( clearValues.size() );
			renderPassBeginInfo.pClearValues    = clearValues.data();
		}
		else
		{
			renderPassBeginInfo.clearValueCount = 0;
			renderPassBeginInfo.pClearValues    = nullptr;
		}

		vkCmdBeginRenderPass( c, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE );
	}

	void EndRenderPass( ch_handle_t cmd ) override
	{
		PROF_SCOPE();

		VkCommandBuffer c = VK_GetCommandBuffer( cmd );

		if ( c == nullptr )
		{
			Log_Error( gLC_Render, "EndRenderPass: Invalid Command Buffer\n" );
			return;
		}

		vkCmdEndRenderPass( c );
	}

	void DrawImGui( ImDrawData* spDrawData, ch_handle_t cmd ) override
	{
		PROF_SCOPE();

		VkCommandBuffer c = VK_GetCommandBuffer( cmd );
		ImGui_ImplVulkan_RenderDrawData( spDrawData, c );
	}

	// --------------------------------------------------------------------------------------------
	// Vulkan Commands
	// --------------------------------------------------------------------------------------------

	void CmdSetViewport( ch_handle_t cmd, u32 sOffset, const Viewport_t* spViewports, u32 sCount ) override
	{
		PROF_SCOPE();

		if ( sCount > gMaxViewports )
		{
			Log_ErrorF( gLC_Render, "CmdSetViewport: Trying to set more than max viewports (Max: %u - Count: %u)", gMaxViewports, sCount );
			return;
		}

		VkCommandBuffer c = VK_GetCommandBuffer( cmd );

		if ( c == nullptr )
		{
			Log_Error( gLC_Render, "CmdSetViewport: Invalid Command Buffer\n" );
			return;
		}

		for ( u32 i = 0; i < sCount; i++ )
		{
			gpViewports[ i ].x        = spViewports[ i ].x;
			gpViewports[ i ].y        = spViewports[ i ].y;
			gpViewports[ i ].width    = spViewports[ i ].width;
			gpViewports[ i ].height   = spViewports[ i ].height;
			gpViewports[ i ].minDepth = spViewports[ i ].minDepth;
			gpViewports[ i ].maxDepth = spViewports[ i ].maxDepth;
		}

		vkCmdSetViewport( c, sOffset, sCount, gpViewports );
	}

	void CmdSetScissor( ch_handle_t cmd, u32 sOffset, const Rect2D_t* spScissors, u32 sCount ) override
	{
		PROF_SCOPE();

		VkCommandBuffer c = VK_GetCommandBuffer( cmd );

		if ( c == nullptr )
		{
			Log_Error( gLC_Render, "CmdSetScissor: Invalid Command Buffer\n" );
			return;
		}

		ChVector< VkRect2D > vkScissors( sCount );
		for ( u32 i = 0; i < sCount; i++ )
		{
			VkRect2D& rect     = vkScissors[ i ];
			rect.offset.x      = spScissors[ i ].aOffset.x;
			rect.offset.y      = spScissors[ i ].aOffset.y;
			rect.extent.width  = spScissors[ i ].aExtent.x;
			rect.extent.height = spScissors[ i ].aExtent.y;
		}

		vkCmdSetScissor( c, sOffset, vkScissors.size(), vkScissors.data() );
	}

	void CmdSetDepthBias( ch_handle_t cmd, float sConstantFactor, float sClamp, float sSlopeFactor ) override
	{
		PROF_SCOPE();

		VkCommandBuffer c = VK_GetCommandBuffer( cmd );

		if ( c == nullptr )
		{
			Log_Error( gLC_Render, "CmdSetDepthBias: Invalid Command Buffer\n" );
			return;
		}

		vkCmdSetDepthBias( c, sConstantFactor, sClamp, sSlopeFactor );
	}

	void CmdSetLineWidth( ch_handle_t cmd, float sLineWidth ) override
	{
		PROF_SCOPE();

		VkCommandBuffer c = VK_GetCommandBuffer( cmd );

		if ( c == nullptr )
		{
			Log_Error( gLC_Render, "CmdSetDepthBias: Invalid Command Buffer\n" );
			return;
		}

		// clamp within the limits
		const VkPhysicalDeviceLimits& limits = VK_GetPhysicalDeviceLimits();

		if ( limits.lineWidthRange[ 0 ] > sLineWidth )
		{
			Log_ErrorF( gLC_Render, "Line Width is below minimum width, clamping to %.6f (was %.6f)\n", limits.lineWidthRange[ 0 ], sLineWidth );
			sLineWidth = limits.lineWidthRange[ 0 ];
		}
		else if ( limits.lineWidthRange[ 1 ] < sLineWidth )
		{
			Log_ErrorF( gLC_Render, "Line Width is above maximum width, clamping to %.6f (was %.6f)\n", limits.lineWidthRange[ 1 ], sLineWidth );
			sLineWidth = limits.lineWidthRange[ 1 ];
		}

		vkCmdSetLineWidth( c, sLineWidth );
	}

	bool CmdBindPipeline( ch_handle_t cmd, ch_handle_t shader ) override
	{
		PROF_SCOPE();

		VkCommandBuffer c = VK_GetCommandBuffer( cmd );

		if ( c == nullptr )
		{
			Log_Error( gLC_Render, "CmdBindPipeline: Invalid Command Buffer\n" );
			return false;
		}

		return VK_BindShader( c, shader );
	}

	void CmdPushConstants( ch_handle_t      cmd,
	                       ch_handle_t      shPipelineLayout,
	                       ShaderStage sShaderStage,
	                       u32         sOffset,
	                       u32         sSize,
	                       void*       spValues ) override
	{
		PROF_SCOPE();

		VkCommandBuffer c = VK_GetCommandBuffer( cmd );

		if ( c == nullptr )
		{
			Log_Error( gLC_Render, "CmdPushConstants: Invalid Command Buffer\n" );
			return;
		}

		VkPipelineLayout layout = VK_GetPipelineLayout( shPipelineLayout );
		if ( !layout )
			return;

		VkShaderStageFlags flags = VK_ToVkShaderStage( sShaderStage );

		vkCmdPushConstants( c, layout, flags, sOffset, sSize, spValues );
	}

#if 0
	// IDEA (from Godot)
	void CmdPushConstants( ch_handle_t      sDrawList,
	                       u32         sOffset,
	                       u32         sSize,
	                       void*       spValues )
	{
		DrawListVK* draw = VK_GetDrawList( sDrawList );

		if ( draw == nullptr )
		{
			Log_Error( gLC_Render, "CmdPushConstants: Invalid DrawList\n" );
			return;
		}

		vkCmdPushConstants( draw->aCmdBuf, draw->aState.aPipelineLayout, draw->aState.PushConstStages, sOffset, sSize, spValues );
	}
#endif

	// will most likely change
	void CmdBindDescriptorSets( ch_handle_t             cmd,
	                            size_t             sCmdIndex,
	                            EPipelineBindPoint sBindPoint,
	                            ch_handle_t             shPipelineLayout,
	                            ch_handle_t*            spSets,
	                            u32                sSetCount ) override
	{
		PROF_SCOPE();

		VkCommandBuffer c = VK_GetCommandBuffer( cmd );

		if ( c == nullptr )
		{
			Log_Error( gLC_Render, "CmdBindDescriptorSets: Invalid Command Buffer\n" );
			return;
		}

		VkPipelineLayout layout = VK_GetPipelineLayout( shPipelineLayout );
		if ( !layout )
			return;

		VkPipelineBindPoint bindPoint = VK_ToPipelineBindPoint( sBindPoint );

		if ( bindPoint == VK_PIPELINE_BIND_POINT_MAX_ENUM )
		{
			Log_Error( gLC_Render, "CmdBindDescriptorSets: Invalid Pipeline Bind Point\n" );
			return;
		}

		VkDescriptorSet* vkDescSets = CH_STACK_NEW( VkDescriptorSet, sSetCount );

		if ( bindPoint == VK_PIPELINE_BIND_POINT_MAX_ENUM )
		{
			Log_ErrorF( gLC_Render, "CmdBindDescriptorSets: Failed to stack alloc VkDescriptorSets (%u sets, %zu bytes)\n", sSetCount, sizeof( VkDescriptorSet ) * sSetCount );
			return;
		}

		for ( u32 i = 0; i < sSetCount; i++ )
		{
			VkDescriptorSet set = VK_GetDescSet( spSets[ i ] );
			if ( set == VK_NULL_HANDLE )
			{
				Log_ErrorF( gLC_Render, "CmdBindDescriptorSets: Failed to find VkDescriptorSet %u\n", i );
				CH_STACK_FREE( vkDescSets );
				return;
			}

			vkDescSets[ i ] = set;
		}

		vkCmdBindDescriptorSets( c, bindPoint, layout, 0, sSetCount, vkDescSets, 0, nullptr );
		CH_STACK_FREE( vkDescSets );
	}

	void CmdBindVertexBuffers( ch_handle_t cmd, u32 sFirstBinding, u32 sCount, const ch_handle_t* spBuffers, const uint64_t* spOffsets ) override
	{
		PROF_SCOPE();

		VkCommandBuffer c = VK_GetCommandBuffer( cmd );

		if ( c == nullptr )
		{
			Log_Error( gLC_Render, "CmdBindVertexBuffers: Invalid Command Buffer\n" );
			return;
		}

		VkBuffer* vkBuffers = CH_MALLOC( VkBuffer, sCount );

		for ( u32 i = 0; i < sCount; i++ )
		{
			BufferVK* bufVK = gBufferHandles.Get( spBuffers[ i ] );

			if ( !bufVK )
			{
				Log_Error( gLC_Render, "CmdBindVertexBuffers: Failed to find Buffer\n" );
				ch_free( vkBuffers );
				return;
			}

			vkBuffers[ i ]  = bufVK->aBuffer;
		}

		vkCmdBindVertexBuffers( c, sFirstBinding, sCount, vkBuffers, spOffsets );
		ch_free( vkBuffers );
	}

	void CmdBindIndexBuffer( ch_handle_t cmd, ch_handle_t shBuffer, size_t offset, EIndexType indexType ) override
	{
		PROF_SCOPE();

		VkCommandBuffer c = VK_GetCommandBuffer( cmd );

		if ( c == nullptr )
		{
			Log_Error( gLC_Render, "CmdBindIndexBuffer: Invalid Command Buffer\n" );
			return;
		}

		BufferVK* bufVK = gBufferHandles.Get( shBuffer );

		if ( !bufVK )
		{
			Log_Error( gLC_Render, "CmdBindIndexBuffer: Failed to find Buffer\n" );
			return;
		}

		VkIndexType     vkIndexType;

		if ( indexType == EIndexType_U16 )
			vkIndexType = VK_INDEX_TYPE_UINT16;

		else if ( indexType == EIndexType_U32 )
			vkIndexType = VK_INDEX_TYPE_UINT32;

		else
		{
			Log_Error( gLC_Render, "CmdBindIndexBuffer: Unknown Index Type\n" );
			return;
		}

		vkCmdBindIndexBuffer( c, bufVK->aBuffer, offset, vkIndexType );
	}
	
	void CmdDraw( ch_handle_t cmd, u32 sVertCount, u32 sInstanceCount, u32 sFirstVert, u32 sFirstInstance ) override
	{
		PROF_SCOPE();

		VkCommandBuffer c = VK_GetCommandBuffer( cmd );

		if ( c == nullptr )
		{
			Log_Error( gLC_Render, "CmdDrawIndexed: Invalid Command Buffer\n" );
			return;
		}

		vkCmdDraw( c, sVertCount, sInstanceCount, sFirstVert, sFirstInstance );
	}

	void CmdDrawIndexed( ch_handle_t cmd, u32 sIndexCount, u32 sInstanceCount, u32 sFirstIndex, int sVertOffset, u32 sFirstInstance ) override
	{
		PROF_SCOPE();

		VkCommandBuffer c = VK_GetCommandBuffer( cmd );

		if ( c == nullptr )
		{
			Log_Error( gLC_Render, "CmdDrawIndexed: Invalid Command Buffer\n" );
			return;
		}

		vkCmdDrawIndexed( c, sIndexCount, sInstanceCount, sFirstIndex, sVertOffset, sFirstInstance );
	}

	void CmdDispatch( ch_handle_t sCmd, u32 sGroupCountX, u32 sGroupCountY, u32 sGroupCountZ ) override
	{
		PROF_SCOPE();

		VkCommandBuffer c = VK_GetCommandBuffer( sCmd );

		if ( c == nullptr )
		{
			Log_Error( gLC_Render, "CmdDrawIndexed: Invalid Command Buffer\n" );
			return;
		}

		vkCmdDispatch( c, sGroupCountX, sGroupCountY, sGroupCountZ );
	}

	void CmdPipelineBarrier( ch_handle_t sCmd, PipelineBarrier_t& srBarrier ) override
	{
		PROF_SCOPE();
	
		VkCommandBuffer c = VK_GetCommandBuffer( sCmd );
	
		if ( c == nullptr )
		{
			Log_Error( gLC_Render, "CmdDrawIndexed: Invalid Command Buffer\n" );
			return;
		}

		VkPipelineStageFlags srcStage = VK_ToPipelineStageFlags( srBarrier.aSrcStageMask );
		VkPipelineStageFlags dstStage = VK_ToPipelineStageFlags( srBarrier.aDstStageMask );

		VkDependencyFlags    dependFlags = 0;

		if ( srBarrier.aDependencyFlags & EDependency_ByRegion )
			dependFlags |= VK_DEPENDENCY_BY_REGION_BIT;

		if ( srBarrier.aDependencyFlags & EDependency_DeviceGroup )
			dependFlags |= VK_DEPENDENCY_DEVICE_GROUP_BIT;

		if ( srBarrier.aDependencyFlags & EDependency_ViewLocal )
			dependFlags |= VK_DEPENDENCY_VIEW_LOCAL_BIT;

		if ( srBarrier.aDependencyFlags & EDependency_FeedbackLoop )
			dependFlags |= VK_DEPENDENCY_FEEDBACK_LOOP_BIT_EXT;

		auto bufferMem = CH_STACK_NEW( VkBufferMemoryBarrier, srBarrier.aBufferMemoryBarrierCount );
		memset( bufferMem, 0, sizeof( VkBufferMemoryBarrier ) * srBarrier.aBufferMemoryBarrierCount );

		for ( u32 i = 0; i < srBarrier.aBufferMemoryBarrierCount; i++ )
		{
			GraphicsBufferMemoryBarrier_t& bufBarrier = srBarrier.apBufferMemoryBarriers[ i ];
			BufferVK*                      bufVK      = nullptr;

			if ( !gBufferHandles.Get( srBarrier.apBufferMemoryBarriers[ i ].aBuffer, &bufVK ) )
			{
				Log_Error( gLC_Render, "CmdPipelineBarrier: Failed to find Buffer\n" );
				CH_STACK_FREE( bufferMem );
				return;
			}

			bufferMem[ i ].buffer              = bufVK->aBuffer;
			bufferMem[ i ].offset              = bufBarrier.aOffset;
			bufferMem[ i ].size                = bufBarrier.aSize == 0 ? bufVK->aSize : bufBarrier.aSize;

			bufferMem[ i ].srcQueueFamilyIndex = gGraphicsAPIData.aQueueFamilyGraphics;
			bufferMem[ i ].dstQueueFamilyIndex = gGraphicsAPIData.aQueueFamilyGraphics;

			bufferMem[ i ].srcAccessMask       = VK_ToAccessFlags( bufBarrier.aSrcAccessMask );
			bufferMem[ i ].dstAccessMask       = VK_ToAccessFlags( bufBarrier.aDstAccessMask );

			bufferMem[ i ].sType               = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
		}

		vkCmdPipelineBarrier(
		  c, srcStage, dstStage, dependFlags,
		  0, nullptr,
		  srBarrier.aBufferMemoryBarrierCount, bufferMem,
		  0, nullptr );

		CH_STACK_FREE( bufferMem );
	}
};


static RenderVK gRenderer;

static ModuleInterface_t gInterfaces[] = {
	{ &gRenderer, IRENDER_NAME, IRENDER_VER }
};

extern "C"
{
	DLL_EXPORT ModuleInterface_t* ch_get_interfaces( u8& srCount )
	{
		srCount = 1;
		return gInterfaces;
	}
}


#if 0
CONCMD( r_reload_textures )
{
	VK_WaitForGraphicsQueue();
	VK_WaitForPresentQueue();

	for ( auto& [ path, handle ] : gTexturePaths )
	{
		gRenderer.LoadTexture( handle, path, gTextureInfo[ handle ] );
	}
}
#endif

