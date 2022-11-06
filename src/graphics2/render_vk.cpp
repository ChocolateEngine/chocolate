// #include "../render.h"
#include "core/platform.h"
#include "core/filesystem.h"
#include "core/log.h"
#include "core/systemmanager.h"
#include "util.h"

#include "render/irender.h"
#include "render_vk.h"

 #include "imgui_impl_sdl.h"
 #include "imgui_impl_vulkan.h"

#include <unordered_map>
#include <filesystem>

LOG_REGISTER_CHANNEL2( Render, LogColor::Cyan );
LOG_REGISTER_CHANNEL( Vulkan, LogColor::DarkYellow );
LOG_REGISTER_CHANNEL( Validation, LogColor::DarkYellow );

int                                                      gWidth   = 1280;
int                                                      gHeight  = 720;

SDL_Window*                                              gpWindow = nullptr;

static VkCommandPool                                     gSingleTime;
static VkCommandPool                                     gPrimary;

// static std::unordered_map< ImageInfo*, VkDescriptorSet > gImGuiTextures;
static std::vector< std::vector< char > >                gFontData;

static std::vector< VkBuffer >                           gBuffers;

ResourceList< BufferVK >                                 gBufferHandles;
ResourceList< TextureVK* >                               gTextureHandles;
// static ResourceManager< BufferVK >        gBufferHandles;

static std::unordered_map< std::string, Handle >         gTexturePaths;
static std::unordered_map< Handle, TextureCreateData_t > gTextureInfo;

// Static Memory Pools for Vulkan Commands
static VkViewport*                                       gpViewports        = nullptr;
static u32                                               gMaxViewports      = 0;

static Render_OnReset_t                                  gpOnResetFunc      = nullptr;

bool                                                     gNeedTextureUpdate = false;
extern std::vector< TextureVK* >                         gTextures;


CONVAR_CMD( r_msaa, 1 )
{
	VK_Reset( ERenderResetFlags_MSAA );
}


CONVAR_CMD( r_msaa_samples, 8 )
{
	if ( !r_msaa )
		return;

	VK_Reset( ERenderResetFlags_MSAA );
}


const char* VKString( VkResult sResult )
{
	switch ( sResult )
	{
		default:
			return "Unknown";
		case VK_SUCCESS:
			return "VK_SUCCESS";
		case VK_ERROR_OUT_OF_HOST_MEMORY:
			return "VK_ERROR_OUT_OF_HOST_MEMORY";
		case VK_ERROR_OUT_OF_DEVICE_MEMORY:
			return "VK_ERROR_OUT_OF_DEVICE_MEMORY";
		case VK_ERROR_INITIALIZATION_FAILED:
			return "VK_ERROR_INITIALIZATION_FAILED";
		case VK_ERROR_DEVICE_LOST:
			return "VK_ERROR_DEVICE_LOST";
		case VK_ERROR_MEMORY_MAP_FAILED:
			return "VK_ERROR_MEMORY_MAP_FAILED";
		case VK_ERROR_LAYER_NOT_PRESENT:
			return "VK_ERROR_LAYER_NOT_PRESENT";
		case VK_ERROR_EXTENSION_NOT_PRESENT:
			return "VK_ERROR_EXTENSION_NOT_PRESENT";
		case VK_ERROR_FEATURE_NOT_PRESENT:
			return "VK_ERROR_FEATURE_NOT_PRESENT";
		case VK_ERROR_INCOMPATIBLE_DRIVER:
			return "VK_ERROR_INCOMPATIBLE_DRIVER";
		case VK_ERROR_TOO_MANY_OBJECTS:
			return "VK_ERROR_TOO_MANY_OBJECTS";
		case VK_ERROR_FORMAT_NOT_SUPPORTED:
			return "VK_ERROR_FORMAT_NOT_SUPPORTED";
		case VK_ERROR_FRAGMENTED_POOL:
			return "VK_ERROR_FRAGMENTED_POOL";
		case VK_ERROR_UNKNOWN:
			return "VK_ERROR_UNKNOWN";
		case VK_ERROR_OUT_OF_POOL_MEMORY:
			return "VK_ERROR_OUT_OF_POOL_MEMORY";
		case VK_ERROR_INVALID_EXTERNAL_HANDLE:
			return "VK_ERROR_INVALID_EXTERNAL_HANDLE";
		case VK_ERROR_FRAGMENTATION:
			return "VK_ERROR_FRAGMENTATION";
		case VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS:
			return "VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS";
		case VK_ERROR_SURFACE_LOST_KHR:
			return "VK_ERROR_SURFACE_LOST_KHR";
		case VK_ERROR_NATIVE_WINDOW_IN_USE_KHR:
			return "VK_ERROR_NATIVE_WINDOW_IN_USE_KHR";
		case VK_SUBOPTIMAL_KHR:
			return "VK_SUBOPTIMAL_KHR";
		case VK_ERROR_OUT_OF_DATE_KHR:
			return "VK_ERROR_OUT_OF_DATE_KHR";
		case VK_ERROR_INCOMPATIBLE_DISPLAY_KHR:
			return "VK_ERROR_INCOMPATIBLE_DISPLAY_KHR";
		case VK_ERROR_VALIDATION_FAILED_EXT:
			return "VK_ERROR_VALIDATION_FAILED_EXT";
		case VK_ERROR_INVALID_SHADER_NV:
			return "VK_ERROR_INVALID_SHADER_NV";
		case VK_ERROR_INVALID_DRM_FORMAT_MODIFIER_PLANE_LAYOUT_EXT:
			return "VK_ERROR_INVALID_DRM_FORMAT_MODIFIER_PLANE_LAYOUT_EXT";
		case VK_ERROR_NOT_PERMITTED_EXT:
			return "VK_ERROR_NOT_PERMITTED_EXT";
		case VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT:
			return "VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT";
		case VK_THREAD_IDLE_KHR:
			return "VK_THREAD_IDLE_KHR";
		case VK_THREAD_DONE_KHR:
			return "VK_THREAD_DONE_KHR";
		case VK_OPERATION_DEFERRED_KHR:
			return "VK_OPERATION_DEFERRED_KHR";
		case VK_OPERATION_NOT_DEFERRED_KHR:
			return "VK_OPERATION_NOT_DEFERRED_KHR";
		case VK_PIPELINE_COMPILE_REQUIRED_EXT:
			return "VK_PIPELINE_COMPILE_REQUIRED_EXT";
	}
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


GraphicsFmt VK_ToGraphicsFmt( VkFormat colorFmt )
{
	switch ( colorFmt )
	{
		default:
			Log_Error( gLC_Render, "Unspecified VkFormat to Color Format Conversion!\n" );

		case VK_FORMAT_UNDEFINED:
			return GraphicsFmt::INVALID;

		// ------------------------------------------

		case VK_FORMAT_B8G8R8A8_SRGB:
			return GraphicsFmt::BGRA8888_SRGB;

		case VK_FORMAT_B8G8R8A8_SNORM:
			return GraphicsFmt::BGRA8888_SNORM;

		case VK_FORMAT_B8G8R8A8_UNORM:
			return GraphicsFmt::BGRA8888_UNORM;

		// ------------------------------------------

		case VK_FORMAT_D16_UNORM:
			return GraphicsFmt::D16_UNORM;

		case VK_FORMAT_D32_SFLOAT:
			return GraphicsFmt::D32_SFLOAT;
	}
}


VkFormat VK_ToVkFormat( GraphicsFmt colorFmt )
{
	switch ( colorFmt )
	{
		default:
			Log_Error( gLC_Render, "Unspecified Color Format to VkFormat Conversion!\n" );

		case GraphicsFmt::INVALID:
			return VK_FORMAT_UNDEFINED;

		// ------------------------------------------

		case GraphicsFmt::R8_SINT:
			return VK_FORMAT_R8_SINT;

		case GraphicsFmt::R8_UINT:
			return VK_FORMAT_R8_UINT;

		case GraphicsFmt::R8_SRGB:
			return VK_FORMAT_R8_SRGB;

		case GraphicsFmt::RG88_SRGB:
			return VK_FORMAT_R8G8_SRGB;

		case GraphicsFmt::RG88_SINT:
			return VK_FORMAT_R8G8_SINT;

		case GraphicsFmt::RG88_UINT:
			return VK_FORMAT_R8G8_UINT;

		case GraphicsFmt::RGBA8888_SRGB:
			return VK_FORMAT_R8G8B8A8_SRGB;

		case GraphicsFmt::RGBA8888_UNORM:
			return VK_FORMAT_R8G8B8A8_UNORM;

		// ------------------------------------------

		case GraphicsFmt::BGRA8888_SRGB:
			return VK_FORMAT_B8G8R8A8_SRGB;

		case GraphicsFmt::BGRA8888_SNORM:
			return VK_FORMAT_B8G8R8A8_SNORM;

		case GraphicsFmt::BGRA8888_UNORM:
			return VK_FORMAT_B8G8R8A8_UNORM;

		// ------------------------------------------

		case GraphicsFmt::R16_SFLOAT:
			return VK_FORMAT_R16_SFLOAT;

		case GraphicsFmt::R16_SINT:
			return VK_FORMAT_R16_SINT;

		case GraphicsFmt::R16_UINT:
			return VK_FORMAT_R16_UINT;

		case GraphicsFmt::RG1616_SFLOAT:
			return VK_FORMAT_R16G16_SFLOAT;

		case GraphicsFmt::RG1616_SINT:
			return VK_FORMAT_R16G16_SINT;

		case GraphicsFmt::RG1616_UINT:
			return VK_FORMAT_R16G16_UINT;

		case GraphicsFmt::RGBA16161616_SFLOAT:
			return VK_FORMAT_R16G16B16A16_SFLOAT;

		// ------------------------------------------

		case GraphicsFmt::R32_SFLOAT:
			return VK_FORMAT_R32_SFLOAT;

		case GraphicsFmt::R32_SINT:
			return VK_FORMAT_R32_SINT;

		case GraphicsFmt::R32_UINT:
			return VK_FORMAT_R32_UINT;

		case GraphicsFmt::RG3232_SFLOAT:
			return VK_FORMAT_R32G32_SFLOAT;

		case GraphicsFmt::RG3232_SINT:
			return VK_FORMAT_R32G32_SINT;

		case GraphicsFmt::RG3232_UINT:
			return VK_FORMAT_R32G32_UINT;

		case GraphicsFmt::RGB323232_SFLOAT:
			return VK_FORMAT_R32G32B32_SFLOAT;

		case GraphicsFmt::RGB323232_SINT:
			return VK_FORMAT_R32G32B32_SINT;

		case GraphicsFmt::RGB323232_UINT:
			return VK_FORMAT_R32G32B32_UINT;

		case GraphicsFmt::RGBA32323232_SFLOAT:
			return VK_FORMAT_R32G32B32A32_SFLOAT;

		case GraphicsFmt::RGBA32323232_SINT:
			return VK_FORMAT_R32G32B32A32_SINT;

		case GraphicsFmt::RGBA32323232_UINT:
			return VK_FORMAT_R32G32B32A32_UINT;

		// ------------------------------------------

		case GraphicsFmt::BC1_RGB_UNORM_BLOCK:
			return VK_FORMAT_BC1_RGB_UNORM_BLOCK;

		case GraphicsFmt::BC1_RGB_SRGB_BLOCK:
			return VK_FORMAT_BC1_RGB_SRGB_BLOCK;

		case GraphicsFmt::BC1_RGBA_UNORM_BLOCK:
			return VK_FORMAT_BC1_RGBA_UNORM_BLOCK;

		case GraphicsFmt::BC1_RGBA_SRGB_BLOCK:
			return VK_FORMAT_BC1_RGBA_SRGB_BLOCK;

		case GraphicsFmt::BC2_UNORM_BLOCK:
			return VK_FORMAT_BC2_UNORM_BLOCK;

		case GraphicsFmt::BC2_SRGB_BLOCK:
			return VK_FORMAT_BC2_SRGB_BLOCK;

		case GraphicsFmt::BC3_UNORM_BLOCK:
			return VK_FORMAT_BC3_UNORM_BLOCK;

		case GraphicsFmt::BC3_SRGB_BLOCK:
			return VK_FORMAT_BC3_SRGB_BLOCK;

		case GraphicsFmt::BC4_UNORM_BLOCK:
			return VK_FORMAT_BC4_UNORM_BLOCK;

		case GraphicsFmt::BC4_SNORM_BLOCK:
			return VK_FORMAT_BC4_SNORM_BLOCK;

		case GraphicsFmt::BC5_UNORM_BLOCK:
			return VK_FORMAT_BC5_UNORM_BLOCK;

		case GraphicsFmt::BC5_SNORM_BLOCK:
			return VK_FORMAT_BC5_SNORM_BLOCK;

		case GraphicsFmt::BC6H_UFLOAT_BLOCK:
			return VK_FORMAT_BC6H_UFLOAT_BLOCK;

		case GraphicsFmt::BC6H_SFLOAT_BLOCK:
			return VK_FORMAT_BC6H_SFLOAT_BLOCK;

		case GraphicsFmt::BC7_SRGB_BLOCK:
			return VK_FORMAT_BC7_SRGB_BLOCK;

		case GraphicsFmt::BC7_UNORM_BLOCK:
			return VK_FORMAT_BC7_UNORM_BLOCK;

		// ------------------------------------------
		// Other Formats

		case GraphicsFmt::D16_UNORM:
			return VK_FORMAT_D16_UNORM;

		case GraphicsFmt::D32_SFLOAT:
			return VK_FORMAT_D32_SFLOAT;
	}
}


VkShaderStageFlags VK_ToVkShaderStage( ShaderStage stage )
{
	VkShaderStageFlags flags = 0;

	if ( stage & ShaderStage_Vertex )
		flags |= VK_SHADER_STAGE_VERTEX_BIT;

	if ( stage & ShaderStage_Fragment )
		flags |= VK_SHADER_STAGE_FRAGMENT_BIT;

	if ( stage & ShaderStage_Compute )
		flags |= VK_SHADER_STAGE_COMPUTE_BIT;

	return flags;
}


VkPipelineBindPoint VK_ToPipelineBindPoint( EPipelineBindPoint bindPoint )
{
	switch ( bindPoint )
	{
		default:
			return VK_PIPELINE_BIND_POINT_MAX_ENUM;

		case EPipelineBindPoint_Graphics:
			return VK_PIPELINE_BIND_POINT_GRAPHICS;

		case EPipelineBindPoint_Compute:
			return VK_PIPELINE_BIND_POINT_COMPUTE;
	}
}


VkImageUsageFlags VK_ToVkImageUsage( EImageUsage usage )
{
	VkImageUsageFlags flags = 0;

	if ( usage & EImageUsage_TransferSrc )
		flags |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;

	if ( usage & EImageUsage_TransferDst )
		flags |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;

	if ( usage & EImageUsage_Sampled )
		flags |= VK_IMAGE_USAGE_SAMPLED_BIT;

	if ( usage & EImageUsage_Storage )
		flags |= VK_IMAGE_USAGE_STORAGE_BIT;

	if ( usage & EImageUsage_AttachColor )
		flags |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

	if ( usage & EImageUsage_AttachDepthStencil )
		flags |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;

	if ( usage & EImageUsage_AttachTransient )
		flags |= VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT;

	if ( usage & EImageUsage_AttachInput )
		flags |= VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT;

	return flags;
}


VkAttachmentLoadOp VK_ToVkLoadOp( EAttachmentLoadOp loadOp )
{
	switch ( loadOp )
	{
		default:
			return VK_ATTACHMENT_LOAD_OP_MAX_ENUM;

		case EAttachmentLoadOp_Load:
			return VkAttachmentLoadOp::VK_ATTACHMENT_LOAD_OP_LOAD;

		case EAttachmentLoadOp_Clear:
			return VkAttachmentLoadOp::VK_ATTACHMENT_LOAD_OP_CLEAR;

		case EAttachmentLoadOp_DontCare:
			return VkAttachmentLoadOp::VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	}
}


VkFilter VK_ToVkFilter( EImageFilter filter )
{
	switch ( filter )
	{
		default:
		case EImageFilter_Linear:
			return VK_FILTER_LINEAR;

		case EImageFilter_Nearest:
			return VK_FILTER_NEAREST;
	}
}


// Copies memory to the GPU.
void VK_memcpy( VkDeviceMemory sBufferMemory, VkDeviceSize sSize, const void* spData )
{
	void* pData;
	VK_CheckResult( vkMapMemory( VK_GetDevice(), sBufferMemory, 0, sSize, 0, &pData ), "Vulkan: Failed to map memory" );
	memcpy( pData, spData, (size_t)sSize );
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

	auto maxSamples = VK_GetMaxMSAASamples();

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


void VK_CreateCommandPool( VkCommandPool& sCmdPool, VkCommandPoolCreateFlags sFlags )
{
	u32 graphics;
	VK_FindQueueFamilies( VK_GetPhysicalDevice(), &graphics, nullptr );

	VkCommandPoolCreateInfo aCommandPoolInfo{ VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };
	aCommandPoolInfo.pNext            = nullptr;
	aCommandPoolInfo.flags            = sFlags;
	aCommandPoolInfo.queueFamilyIndex = graphics;

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


void VK_CreateBuffer( VkBuffer& srBuffer, VkDeviceMemory& srBufferMem, u32 sBufferSize, VkBufferUsageFlags sUsage, VkMemoryPropertyFlags sMemBits )
{
	// create a vertex buffer
	VkBufferCreateInfo aBufferInfo{ VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
	aBufferInfo.size        = sBufferSize;
	aBufferInfo.usage       = sUsage;
	aBufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	VK_CheckResult( vkCreateBuffer( VK_GetDevice(), &aBufferInfo, nullptr, &srBuffer ), "Failed to create buffer" );

	// allocate memory for the vertex buffer
	VkMemoryRequirements aMemReqs;
	vkGetBufferMemoryRequirements( VK_GetDevice(), srBuffer, &aMemReqs );

	VkMemoryAllocateInfo aMemAllocInfo{ VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
	aMemAllocInfo.allocationSize  = aMemReqs.size;
	aMemAllocInfo.memoryTypeIndex = VK_GetMemoryType( aMemReqs.memoryTypeBits, sMemBits );

	VK_CheckResult( vkAllocateMemory( VK_GetDevice(), &aMemAllocInfo, nullptr, &srBufferMem ), "Failed to allocate buffer memory" );

	// bind the vertex buffer to the device memory
	VK_CheckResult( vkBindBufferMemory( VK_GetDevice(), srBuffer, srBufferMem, 0 ), "Failed to bind buffer" );

	gBuffers.push_back( srBuffer );
}


void VK_CreateBuffer( const char* spName, VkBuffer& srBuffer, VkDeviceMemory& srBufferMem, u32 sBufferSize, VkBufferUsageFlags sUsage, VkMemoryPropertyFlags sMemBits )
{
	VK_CreateBuffer( srBuffer, srBufferMem, sBufferSize, sUsage, sMemBits );

#ifdef _DEBUG
	if ( spName == nullptr )
		return;

	// add a debug label onto it
	if ( pfnSetDebugUtilsObjectName )
	{
		const VkDebugUtilsObjectNameInfoEXT nameInfo = {
			VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,  // sType
			NULL,                                                // pNext
			VK_OBJECT_TYPE_BUFFER,                               // objectType
			(uint64_t)srBuffer,                                  // objectHandle
			spName,                                              // pObjectName
		};

		pfnSetDebugUtilsObjectName( VK_GetDevice(), &nameInfo );
	}
#endif
}


void VK_DestroyBuffer( VkBuffer& srBuffer, VkDeviceMemory& srBufferMem )
{
	if ( srBuffer )
	{
		vec_remove( gBuffers, srBuffer );
		vkDestroyBuffer( VK_GetDevice(), srBuffer, nullptr );
	}

	if ( srBufferMem )
		vkFreeMemory( VK_GetDevice(), srBufferMem, nullptr );
}


bool VK_CreateImGuiFonts()
{
	VkCommandBuffer c = VK_BeginOneTimeCommand();

	if ( !ImGui_ImplVulkan_CreateFontsTexture( c ) )
	{
		Log_Error( gLC_Render, "VK_CreateImGuiFonts(): Failed to create ImGui Fonts Texture!\n" );
		return false;
	}

	VK_EndOneTimeCommand( c );

	ImGui_ImplVulkan_DestroyFontUploadObjects();

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
	initInfo.MinImageCount   = VK_GetSwapImageCount();
	initInfo.ImageCount      = VK_GetSwapImageCount();
	initInfo.CheckVkResultFn = VK_CheckResult;

	RenderPassInfoVK* info   = VK_GetRenderPassInfo( sRenderPass );
	if ( info == nullptr )
	{
		Log_Error( gLC_Render, "Failed to find RenderPass info\n" );
		return false;
	}

	initInfo.MSAASamples = info->aUsesMSAA ? VK_GetMSAASamples() : VK_SAMPLE_COUNT_1_BIT;

	if ( !ImGui_ImplVulkan_Init( &initInfo, sRenderPass ) )
		return false;

	return true;
}


// ----------------------------------------------------------------------------------
// Render Abstraction

void Render_Shutdown();

bool Render_Init( void* spWindow )
{
	if ( !VK_CreateInstance() )
	{
		Log_Error( gLC_Render, "Failed to create Vulkan Instance\n" );
		return false;
	}

	VK_CreateSurface( spWindow );
	VK_SetupPhysicalDevice();
	VK_CreateDevice();

	VK_CreateCommandPool( VK_GetSingleTimeCommandPool() );
	VK_CreateCommandPool( VK_GetPrimaryCommandPool() );

	VK_CreateSwapchain();
	VK_CreateFences();
	VK_CreateSemaphores();
	VK_CreateDescSets();

	VK_AllocateCommands();

	const auto& deviceProps = VK_GetPhysicalDeviceProperties();

	gpViewports             = new VkViewport[deviceProps.limits.maxViewports];
	gMaxViewports           = deviceProps.limits.maxViewports;

	VK_CreateMissingTexture();

	Log_Msg( gLC_Render, "Loaded Vulkan Renderer\n" );

	return true;
}


void Render_Shutdown()
{
	if ( gpViewports )
		delete gpViewports;

	ImGui_ImplVulkan_Shutdown();

	VK_DestroySwapchain();
	VK_DestroyRenderTargets();
	VK_DestroyRenderPasses();

	VK_DestroyAllTextures();

	// VK_DestroyFilterShader();
	// VK_DestroyImageShader();

	VK_FreeCommands();

	VK_DestroyFences();
	VK_DestroySemaphores();
	VK_DestroyCommandPool( VK_GetSingleTimeCommandPool() );
	VK_DestroyCommandPool( VK_GetPrimaryCommandPool() );
	VK_DestroyDescSets();

	VK_DestroyInstance();
}


void VK_Reset( ERenderResetFlags sFlags )
{
	VK_RecreateSwapchain();

	// recreate backbuffer
	VK_DestroyRenderTargets();

	if ( sFlags & ERenderResetFlags_MSAA )
	{
		VK_DestroyMainRenderPass();
		VK_CreateMainRenderPass();
	}

	VK_GetBackBuffer();

	if ( gpOnResetFunc )
		gpOnResetFunc( sFlags );
}


void Render_NewFrame()
{
	ImGui_ImplVulkan_NewFrame();
}


void Render_Reset()
{
	VK_Reset();
}


void Render_Present()
{
	VK_RecordCommands();
	VK_Present();
}


void Render_SetResolution( int sWidth, int sHeight )
{
	gWidth  = sWidth;
	gHeight = sHeight;
}


#if 0
ImTextureID Render_AddTextureToImGui( Handle texture )
{
	if ( spInfo == nullptr )
	{
		Log_Error( gLC_Render, "Render_AddTextureToImGui(): ImageInfo* is nullptr!\n" );
		return nullptr;
	}

	TextureVK* tex = VK_GetTexture( spInfo );
	if ( tex == nullptr )
	{
		Log_Error( gLC_Render, "Render_AddTextureToImGui(): No Vulkan Texture created for Image!\n" );
		return nullptr;
	}

	VK_SetImageLayout( tex->aImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 1 );

	auto desc = ImGui_ImplVulkan_AddTexture( VK_GetSampler( tex->aFilter ), tex->aImageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL );

	if ( desc )
	{
		gImGuiTextures[ spInfo ] = desc;
		return desc;
	}

	Log_Error( gLC_Render, "Render_AddTextureToImGui(): Failed to add texture to ImGui\n" );
	return nullptr;
}

static ImWchar ranges[] = { 0x1, 0x1FFFF, 0 };

ImFont* Render_AddFont( const std::filesystem::path& srPath, float sSizePixels, const ImFontConfig* spFontConfig )
{
	// if ( !FileSys_IsFile( srPath.c_str() ) )
	// {
	// 	wprintf( L"Render_BuildFont(): Font does not exist: %ws\n", srPath.c_str() );
	// 	return nullptr;
	// }
	// 
	// auto& fileData = gFontData.emplace_back();
	// 
	// if ( !fs_read_file( srPath, fileData ) )
	// {
	// 	wprintf( L"Render_BuildFont(): Font is empty file: %ws\n", srPath.c_str() );
	// 	gFontData.pop_back();
	// 	return nullptr;
	// }
	// 
	// auto& io = ImGui::GetIO();
	// return io.Fonts->AddFontFromMemoryTTF( fileData.data(), fileData.size(), sSizePixels, spFontConfig, ranges );
	return nullptr;
}
#endif

bool Render_BuildFonts()
{
	bool ret = VK_CreateImGuiFonts();
	gFontData.clear();
	return ret;
}


// ----------------------------------------------------------------------------------
// Chocolate Engine Render Abstraction


class RenderVK : public IRender
{
public:
	// --------------------------------------------------------------------------------------------
	// BaseSystem Functions
	// --------------------------------------------------------------------------------------------

	bool Init() override
	{
		gWidth = Args_GetInt( "-w", gWidth ); 
		gHeight = Args_GetInt( "-h", gHeight ); 

		// this really should not be here of all things
		if ( SDL_Init( SDL_INIT_VIDEO | SDL_INIT_EVENTS | SDL_INIT_AUDIO ) != 0 )
			Log_Fatal( "Unable to initialize SDL2!" );

		int flags = SDL_WINDOW_VULKAN | SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE;

		if ( Args_Find( "-max" ) )
			flags |= SDL_WINDOW_MAXIMIZED;

		gpWindow = SDL_CreateWindow( "Chocolate Engine - Compiled on " __DATE__, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
		                             gWidth, gHeight, flags );

		if ( !Render_Init( gpWindow ) )
			return false;

		if ( !ImGui_ImplSDL2_InitForVulkan( gpWindow ) )
		{
			Log_Error( gLC_Render, "Failed to init ImGui SDL2 for Vulkan\n" );
			Render_Shutdown();
			return false;
		}

		return true;
	}

	void Shutdown()
	{
		Render_Shutdown();
	}

	void Update( float sDT ) override
	{
	}

	// --------------------------------------------------------------------------------------------
	// General Purpose Functions
	// --------------------------------------------------------------------------------------------

	// very odd
	bool InitImGui( Handle shRenderPass ) override
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

		return Render_BuildFonts();
	}

	void ShutdownImGui() override
	{
		ImGui_ImplVulkan_Shutdown();
	}

	SDL_Window* GetWindow() override
	{
		return gpWindow;
	}

	void GetSurfaceSize( int& srWidth, int& srHeight ) override
	{
		SDL_GetWindowSize( gpWindow, &srWidth, &srHeight );
	}

	// --------------------------------------------------------------------------------------------
	// Testing
	// --------------------------------------------------------------------------------------------
	
	Handle CreateBuffer( const char* spName, u32 sSize, EBufferFlags sBufferFlags, EBufferMemory sBufferMem ) override
	{
		BufferVK* buffer = nullptr;
		Handle    handle = gBufferHandles.Create( &buffer );

		// BufferVK* buffer = new BufferVK;
		// Handle   handle = gBufferHandles.Add( buffer );

		buffer->aBuffer  = nullptr;
		buffer->aMemory  = nullptr;
		buffer->aSize    = sSize;

		int flagBits     = 0;

		if ( sBufferFlags & EBufferFlags_TransferSrc )
			flagBits |= VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

		if ( sBufferFlags & EBufferFlags_TransferDst )
			flagBits |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;

		if ( sBufferFlags & EBufferFlags_Uniform )
			flagBits |= VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;

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

		VK_CreateBuffer( spName, buffer->aBuffer, buffer->aMemory, sSize, flagBits, memBits );

		return handle;
	}
	
	Handle CreateBuffer( u32 sSize, EBufferFlags sBufferFlags, EBufferMemory sBufferMem ) override
	{
		return CreateBuffer( nullptr, sSize, sBufferFlags, sBufferMem );
	}

	virtual void MemWriteBuffer( Handle buffer, u32 sSize, void* spData ) override
	{
		BufferVK* bufVK = gBufferHandles.Get( buffer );

		if ( !bufVK )
		{
			Log_ErrorF( gLC_Render, "MemWriteBuffer: Failed to find Buffer (Handle: %zd)\n", buffer );
			return;
		}

		VK_memcpy( bufVK->aMemory, sSize, spData );
	}

	virtual void MemCopyBuffer( Handle shSrc, Handle shDst, u32 sSize ) override
	{
		BufferVK* bufSrc = gBufferHandles.Get( shSrc );
		if ( !bufSrc )
		{
			Log_Error( gLC_Render, "MemCopyBuffer: Failed to find Source Buffer\n" );
			return;
		}

		BufferVK* bufDst = gBufferHandles.Get( shDst );
		if ( !bufDst )
		{
			Log_Error( gLC_Render, "MemCopyBuffer: Failed to find Dest Buffer\n" );
			return;
		}
		
		VkBufferCopy copyRegion{};
		copyRegion.srcOffset = 0;  // Optional
		copyRegion.dstOffset = 0;  // Optional
		copyRegion.size      = sSize;

		VK_OneTimeCommand( [ & ]( VkCommandBuffer c )
		                  { vkCmdCopyBuffer( c, bufSrc->aBuffer, bufDst->aBuffer, 1, &copyRegion ); } );
	}

	void DestroyBuffer( Handle buffer ) override
	{
		BufferVK* bufVK = gBufferHandles.Get( buffer );

		if ( !bufVK )
		{
			Log_Error( gLC_Render, "MemCopy: Failed to find Buffer\n" );
			return;
		}

		VK_DestroyBuffer( bufVK->aBuffer, bufVK->aMemory );

		gBufferHandles.Remove( buffer );
	}

	// --------------------------------------------------------------------------------------------
	// Materials and Textures
	// --------------------------------------------------------------------------------------------

	Handle LoadTexture( Handle& srHandle, const std::string& srTexturePath, const TextureCreateData_t& srCreateData ) override
	{
		if ( srHandle == InvalidHandle )
		{
			auto it = gTexturePaths.find( srTexturePath );
			if ( it != gTexturePaths.end() && it->second != InvalidHandle )
			{
				srHandle = it->second;
				return srHandle;
			}
		}

		std::string fullPath;

		// we only support ktx right now
		if ( srTexturePath.ends_with( ".ktx" ) )
		{
			fullPath = FileSys_FindFile( srTexturePath );
		}
		else
		{
			fullPath = FileSys_FindFile( srTexturePath + ".ktx" );
		}

		if ( fullPath.empty() )
		{
			// add it to the paths anyway, if you do a texture reload, then maybe the texture will have been added
			gTexturePaths[ srTexturePath ] = InvalidHandle;
			Log_ErrorF( gLC_Render, "Failed to find Texture: \"%s\"\n", srTexturePath.c_str() );
			return InvalidHandle;
		}

		if ( srHandle )
		{
			// free old texture data
			TextureVK* tex = nullptr;
			if ( !gTextureHandles.Get( srHandle, &tex ) )
			{
				Log_Error( gLC_Render, "Failed to find old texture\n" );
				return InvalidHandle;
			}

			if ( tex->aImageView )
				vkDestroyImageView( VK_GetDevice(), tex->aImageView, nullptr );

			if ( tex->aMemory )
			{
				if ( tex->aImage )
					vkDestroyImage( VK_GetDevice(), tex->aImage, nullptr );

				vkFreeMemory( VK_GetDevice(), tex->aMemory, nullptr );
			}

			if ( !VK_LoadTexture( tex, fullPath, srCreateData ) )
			{
				VK_DestroyTexture( tex );
				return InvalidHandle;
			}

			// write new texture data
			if ( !gTextureHandles.Update( srHandle, tex ) )
			{
				Log_ErrorF( gLC_Render, "Failed to Update Texture Handle: \"%s\"\n", srTexturePath.c_str() );
				return InvalidHandle;
			}
		}
		else
		{
			TextureVK* tex = VK_NewTexture();
			if ( !VK_LoadTexture( tex, fullPath, srCreateData ) )
			{
				VK_DestroyTexture( tex );
				return InvalidHandle;
			}

			srHandle                       = gTextureHandles.Add( tex );
			gTexturePaths[ srTexturePath ] = srHandle;
			gTextureInfo[ srHandle ]       = srCreateData;
		}

		return srHandle;
	}

	Handle CreateTexture( const TextureCreateInfo_t& srTextureCreateInfo, const TextureCreateData_t& srCreateData ) override
	{
		TextureVK* tex = VK_CreateTexture( srTextureCreateInfo, srCreateData );
		if ( tex == nullptr )
			return InvalidHandle;

		Handle handle = gTextureHandles.Add( tex );
		gTextureInfo[ handle ] = srCreateData;
		return handle;
	}

	void FreeTexture( Handle sTexture ) override
	{
		TextureVK* tex = nullptr;
		if ( !gTextureHandles.Get( sTexture, &tex ) )
		{
			Log_Error( gLC_Render, "FreeTexture: Failed to find texture\n" );
			return;
		}

		VK_DestroyTexture( tex );
		gTextureHandles.Remove( sTexture );
		gTextureInfo.erase( sTexture );

		for ( auto& [ path, handle ] : gTexturePaths )
		{
			if ( sTexture == handle )
			{
				gTexturePaths.erase( path );
				break;
			}
		}
	}

	int GetTextureIndex( Handle shTexture ) override
	{
		if ( shTexture == InvalidHandle )
			return 0;

		TextureVK* tex = nullptr;
		if ( !gTextureHandles.Get( shTexture, &tex ) )
		{
			Log_Error( gLC_Render, "GetTextureIndex: Failed to find texture\n" );
			return 0;
		}

		return tex->aIndex;
	}

	void ReloadTextures() override
	{
		VK_WaitForGraphicsQueue();
		VK_WaitForPresentQueue();

		for ( auto& [ path, handle ] : gTexturePaths )
		{
			LoadTexture( handle, path, gTextureInfo[ handle ] );
		}
	}

	Handle CreateFramebuffer( const CreateFramebuffer_t& srCreate ) override
	{
		VkRenderPass renderPass = VK_GetRenderPass( srCreate.aRenderPass );
		if ( renderPass == nullptr )
		{
			Log_Error( gLC_Render, "Failed to create Framebuffer: RenderPass not found!\n" );
			return InvalidHandle;
		}

		size_t count = srCreate.aPass.aAttachColors.size();

		if ( srCreate.aPass.aAttachDepth )
			count++;

		VkImageView* attachments = (VkImageView*)CH_STACK_ALLOC( sizeof( VkImageView ) * count );

		if ( attachments == nullptr )
		{
			Log_ErrorF( gLC_Render, "STACK OVERFLOW: Failed to allocate stack for Framebuffer attachments (%zu bytes)\n", sizeof( VkImageView ) * count );
			return InvalidHandle;
		}

		count = 0;
		for ( size_t i = 0; i < srCreate.aPass.aAttachColors.size(); i++ )
		{
			TextureVK* tex = nullptr;
			if ( !gTextureHandles.Get( srCreate.aPass.aAttachColors[ i ], &tex ) )
			{
				Log_ErrorF( gLC_Render, "Failed to find texture %u while creating Framebuffer\n", i );
				return InvalidHandle;
			}
			
			attachments[ count++ ] = tex->aImageView;
		}

		if ( srCreate.aPass.aAttachDepth != InvalidHandle )
		{
			TextureVK* tex = nullptr;
			if ( !gTextureHandles.Get( srCreate.aPass.aAttachDepth, &tex ) )
			{
				Log_Error( gLC_Render, "Failed to find depth texture while creating Framebuffer\n" );
				return InvalidHandle;
			}
			
			attachments[ count++ ] = tex->aImageView;
		}

		Handle handle = VK_CreateFramebuffer( renderPass, srCreate.aSize.x, srCreate.aSize.y, attachments, count );
		CH_STACK_FREE( attachments );
		return handle;
	}

	void DestroyFramebuffer( Handle shTarget ) override
	{
		if ( shTarget == InvalidHandle )
			return;

		VK_DestroyFramebuffer( shTarget );
	}

	// --------------------------------------------------------------------------------------------
	// Shader System
	// --------------------------------------------------------------------------------------------

	bool CreatePipelineLayout( Handle& sHandle, PipelineLayoutCreate_t& srPipelineCreate ) override
	{
		return VK_CreatePipelineLayout( sHandle, srPipelineCreate );
	}

	bool CreateGraphicsPipeline( Handle& sHandle, GraphicsPipelineCreate_t& srGraphicsCreate ) override
	{
		return VK_CreateGraphicsPipeline( sHandle, srGraphicsCreate );
	}

	void DestroyPipeline( Handle sPipeline ) override
	{
		VK_DestroyPipeline( sPipeline );
	}

	void DestroyPipelineLayout( Handle sPipeline ) override
	{
		VK_DestroyPipelineLayout( sPipeline );
	}

	Handle GetSamplerLayout() override
	{
		return VK_GetSamplerLayoutHandle();
	}

	void GetSamplerSets( Handle* spHandles ) override
	{
		const std::vector< Handle >& handles = VK_GetSamplerSetsHandles();
		spHandles[ 0 ]                       = handles[ 0 ];
		spHandles[ 1 ]                       = handles[ 1 ];
	}

	Handle CreateVariableDescLayout( const CreateVariableDescLayout_t& srCreate ) override
	{
		return VK_CreateVariableDescLayout( srCreate );
	}
	
	bool AllocateVariableDescLayout( const AllocVariableDescLayout_t& srCreate, Handle* handles ) override
	{
		return VK_AllocateVariableDescLayout( srCreate, handles );
	}

	void UpdateVariableDescSet( const UpdateVariableDescSet_t& srUpdate ) override
	{
		VK_UpdateVariableDescSet( srUpdate );
	}

	// --------------------------------------------------------------------------------------------
	// Back Buffer Info
	// --------------------------------------------------------------------------------------------

	GraphicsFmt GetSwapFormatColor() override
	{
		return VK_ToGraphicsFmt( VK_GetSwapFormat() );
	}

	GraphicsFmt GetSwapFormatDepth() override
	{
		return VK_ToGraphicsFmt( VK_FORMAT_D32_SFLOAT );
	}

	void GetBackBufferTextures( Handle* spColor, Handle* spDepth, Handle* spResolve ) override
	{
#if 0
		extern std::unordered_map< TextureVK*, Handle > gBackbufferHandles;

		RenderTarget* backBuf = VK_GetBackBuffer();
		if ( !backBuf )
		{
			Log_Error( gLC_Render, "No Backbuffer????\n" );
			return;
		}

		if ( spColor && backBuf->aColors.size() )
		{
			*spColor = gBackbufferHandles[ backBuf->aColors[ 0 ] ];
		}

		if ( spDepth && backBuf->apDepth )
		{
			*spDepth = gBackbufferHandles[ backBuf->apDepth ];
		}

		if ( spResolve && backBuf->aResolve.size() && VK_UseMSAA() )
		{
			*spResolve = gBackbufferHandles[ backBuf->aResolve[ 0 ] ];
		}
#endif
	}

	Handle GetBackBufferColor() override
	{
		RenderTarget* backBuf = VK_GetBackBuffer();
		if ( !backBuf )
		{
			Log_Error( gLC_Render, "No Backbuffer????\n" );
			return InvalidHandle;
		}

		return VK_GetFramebufferHandle( backBuf->aFrameBuffers[ 0 ].aBuffer );
	}

	Handle GetBackBufferDepth() override
	{
		RenderTarget* backBuf = VK_GetBackBuffer();
		if ( !backBuf )
		{
			Log_Error( gLC_Render, "No Backbuffer????\n" );
			return InvalidHandle;
		}

		return VK_GetFramebufferHandle( backBuf->aFrameBuffers[ 1 ].aBuffer );
	}

	// --------------------------------------------------------------------------------------------
	// Rendering
	// --------------------------------------------------------------------------------------------

	void SetResetCallback( Render_OnReset_t sFunc ) override
	{
		gpOnResetFunc = sFunc;
	}

	void NewFrame() override
	{
		Render_NewFrame();
	}

	void Reset() override
	{
		VK_Reset();
	}

	void PreRenderPass() override
	{
		if ( gNeedTextureUpdate )
			VK_UpdateImageSets();

		gNeedTextureUpdate = false;
	}

	void Present() override
	{
		VK_Present();
	}

	void WaitForQueues() override
	{
		VK_WaitForGraphicsQueue();
		VK_WaitForPresentQueue();
	}

	void ResetCommandPool() override
	{
		VK_ResetCommandPool( VK_GetPrimaryCommandPool() );
	}

	// blech again
	void GetCommandBufferHandles( std::vector< Handle >& srHandles ) override
	{
		extern ResourceList< VkCommandBuffer > gCommandBufferHandles;  // wtf

		for ( size_t i = 0; i < gCommandBufferHandles.size(); i++ )
		{
			srHandles.push_back( gCommandBufferHandles.GetHandleByIndex( i ) );
		}
	}

#if 0
	Handle AllocCommandBuffer() override
	{
		return InvalidHandle;
	}

	void FreeCommandBuffer( Handle cmd ) override
	{
	}
#endif

	void BeginCommandBuffer( Handle cmd ) override
	{
		VkCommandBuffer c = VK_GetCommandBuffer( cmd );

		// TODO: expose this
		VkCommandBufferBeginInfo begin{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
		begin.pNext            = nullptr;
		begin.flags            = 0;
		begin.pInheritanceInfo = nullptr;

		VK_CheckResult( vkBeginCommandBuffer( c, &begin ), "Failed to begin command buffer" );
	}

	void EndCommandBuffer( Handle cmd ) override
	{
		VkCommandBuffer c = VK_GetCommandBuffer( cmd );
		VK_CheckResult( vkEndCommandBuffer( c ), "Failed to end command buffer" );
	}

	
	Handle CreateRenderPass( const RenderPassCreate_t& srCreate ) override
	{
		return VK_CreateRenderPass( srCreate );
	}

	void DestroyRenderPass( Handle shPass ) override
	{
		VK_DestroyRenderPass( shPass );
	}

	void BeginRenderPass( Handle cmd, const RenderPassBegin_t& srBegin ) override
	{
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
		renderPassBeginInfo.renderArea.offset = { 0, 0 };
		renderPassBeginInfo.renderArea.extent = VK_GetSwapExtent();  // TODO: ADD renderArea THIS TO RenderPassBegin_t

		std::vector< VkClearValue > clearValues( srBegin.aClear.size() );
		if ( srBegin.aClear.size() )
		{
			for ( size_t i = 0; i < srBegin.aClear.size(); i++ )
			{
				if ( srBegin.aClear[ i ].aIsDepth )
				{
					clearValues[ i ].depthStencil.depth   = srBegin.aClear[ i ].aDepth;
					clearValues[ i ].depthStencil.stencil = srBegin.aClear[ i ].aStencil;
				}
				else
				{
					clearValues[ i ].color = {
						srBegin.aClear[ i ].aColor.x,
						srBegin.aClear[ i ].aColor.y,
						srBegin.aClear[ i ].aColor.z,
						srBegin.aClear[ i ].aColor.w
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

	void EndRenderPass( Handle cmd ) override
	{
		VkCommandBuffer c = VK_GetCommandBuffer( cmd );

		if ( c == nullptr )
		{
			Log_Error( gLC_Render, "EndRenderPass: Invalid Command Buffer\n" );
			return;
		}

		vkCmdEndRenderPass( c );
	}

	void DrawImGui( ImDrawData* spDrawData, Handle cmd ) override
	{
		VkCommandBuffer c = VK_GetCommandBuffer( cmd );
		ImGui_ImplVulkan_RenderDrawData( spDrawData, c );
	}

	// --------------------------------------------------------------------------------------------
	// Vulkan Commands
	// --------------------------------------------------------------------------------------------

	void CmdSetViewport( Handle cmd, u32 sOffset, const Viewport_t* spViewports, u32 sCount ) override
	{
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

	void CmdSetScissor( Handle cmd, u32 sOffset, const Rect2D_t* spScissors, u32 sCount ) override
	{
		VkCommandBuffer c = VK_GetCommandBuffer( cmd );

		if ( c == nullptr )
		{
			Log_Error( gLC_Render, "CmdSetScissor: Invalid Command Buffer\n" );
			return;
		}

		std::vector< VkRect2D > vkScissors;
		for ( u32 i = 0; i < sCount; i++ )
		{
			VkRect2D& rect     = vkScissors.emplace_back();
			rect.offset.x      = spScissors[ i ].aOffset.x;
			rect.offset.y      = spScissors[ i ].aOffset.y;
			rect.extent.width  = spScissors[ i ].aExtent.x;
			rect.extent.height = spScissors[ i ].aExtent.y;
		}

		vkCmdSetScissor( c, sOffset, vkScissors.size(), vkScissors.data() );
	}

	void CmdSetDepthBias( Handle cmd, float sConstantFactor, float sClamp, float sSlopeFactor ) override
	{
		VkCommandBuffer c = VK_GetCommandBuffer( cmd );

		if ( c == nullptr )
		{
			Log_Error( gLC_Render, "CmdSetDepthBias: Invalid Command Buffer\n" );
			return;
		}

		return vkCmdSetDepthBias( c, sConstantFactor, sClamp, sSlopeFactor );
	}

	bool CmdBindPipeline( Handle cmd, Handle shader ) override
	{
		VkCommandBuffer c = VK_GetCommandBuffer( cmd );

		if ( c == nullptr )
		{
			Log_Error( gLC_Render, "CmdBindPipeline: Invalid Command Buffer\n" );
			return false;
		}

		return VK_BindShader( c, shader );
	}

	void CmdPushConstants( Handle      cmd,
	                       Handle      shPipelineLayout,
	                       ShaderStage sShaderStage,
	                       u32         sOffset,
	                       u32         sSize,
	                       void*       spValues ) override
	{
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
	void CmdPushConstants( Handle      sDrawList,
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
	void CmdBindDescriptorSets( Handle             cmd,
	                            size_t             sCmdIndex,
	                            EPipelineBindPoint sBindPoint,
	                            Handle             shPipelineLayout,
	                            Handle*            spSets,
	                            u32                sSetCount ) override
	{
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

	void CmdBindVertexBuffers( Handle cmd, u32 sFirstBinding, u32 sCount, const Handle* spBuffers, const size_t* spOffsets ) override
	{
		VkCommandBuffer c = VK_GetCommandBuffer( cmd );

		if ( c == nullptr )
		{
			Log_Error( gLC_Render, "CmdBindVertexBuffers: Invalid Command Buffer\n" );
			return;
		}

		VkBuffer* vkBuffers = (VkBuffer*)CH_STACK_ALLOC( sizeof( VkBuffer ) * sCount );

		for ( u32 i = 0; i < sCount; i++ )
		{
			BufferVK* bufVK = gBufferHandles.Get( spBuffers[ i ] );
			vkBuffers[ i ]  = bufVK->aBuffer;
		}

		vkCmdBindVertexBuffers( c, sFirstBinding, sCount, vkBuffers, spOffsets );
		CH_STACK_FREE( vkBuffers );
	}

	void CmdBindIndexBuffer( Handle cmd, Handle shBuffer, size_t offset, EIndexType indexType ) override
	{
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
	
	void CmdDraw( Handle cmd, u32 sVertCount, u32 sInstanceCount, u32 sFirstVert, u32 sFirstInstance ) override
	{
		VkCommandBuffer c = VK_GetCommandBuffer( cmd );

		if ( c == nullptr )
		{
			Log_Error( gLC_Render, "CmdDrawIndexed: Invalid Command Buffer\n" );
			return;
		}

		vkCmdDraw( c, sVertCount, sInstanceCount, sFirstVert, sFirstInstance );
	}

	void CmdDrawIndexed( Handle cmd, u32 sIndexCount, u32 sInstanceCount, u32 sFirstIndex, int sVertOffset, u32 sFirstInstance ) override
	{
		VkCommandBuffer c = VK_GetCommandBuffer( cmd );

		if ( c == nullptr )
		{
			Log_Error( gLC_Render, "CmdDrawIndexed: Invalid Command Buffer\n" );
			return;
		}

		vkCmdDrawIndexed( c, sIndexCount, sInstanceCount, sFirstIndex, sVertOffset, sFirstInstance );
	}
};

static RenderVK gRenderer;

static ModuleInterface_t gInterfaces[] = {
	{ &gRenderer, IRENDER_NAME, IRENDER_HASH }
};

extern "C"
{
	DLL_EXPORT ModuleInterface_t* cframework_GetInterfaces( size_t& srCount )
	{
		srCount = 1;
		return gInterfaces;
	}
}


CONCMD( r_reload_textures )
{
	VK_WaitForGraphicsQueue();
	VK_WaitForPresentQueue();

	for ( auto& [ path, handle ] : gTexturePaths )
	{
		gRenderer.LoadTexture( handle, path, gTextureInfo[ handle ] );
	}
}

