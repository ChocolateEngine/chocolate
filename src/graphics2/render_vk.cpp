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

LOG_REGISTER_CHANNEL2( Render, LogColor::Cyan );
LOG_REGISTER_CHANNEL2( Vulkan, LogColor::DarkYellow );
LOG_REGISTER_CHANNEL2( Validation, LogColor::DarkYellow );

int                                                      gWidth     = Args_RegisterF( 1280, "Width of the main window", 2, "-width", "-w" );
int                                                      gHeight    = Args_RegisterF( 720, "Height of the main window", 2, "-height", "-h" );

#ifndef _WIN32
static bool                                              gMaxWindow = Args_Register( "Maximize the main window", "-max" );
#endif

SDL_Window*                                              gpWindow = nullptr;

static VkCommandPool                                     gSingleTime;
static VkCommandPool                                     gPrimary;
static VkCommandPool                                     gCmdPoolTransfer;

static std::unordered_map< ChHandle_t, VkDescriptorSet > gImGuiTextures;
static std::vector< std::vector< char > >                gFontData;

static std::vector< VkBuffer >                           gBuffers;

ResourceList< BufferVK >                                 gBufferHandles;
ResourceList< TextureVK* >                               gTextureHandles;
// static ResourceManager< BufferVK >        gBufferHandles;

static std::unordered_map< std::string, Handle >         gTexturePaths;
static std::unordered_map< Handle, TextureCreateData_t > gTextureInfo;

// Static Memory Pools for Vulkan Commands
static VkViewport*                                       gpViewports                = nullptr;
static u32                                               gMaxViewports              = 0;

static Render_OnReset_t                                  gpOnResetFunc              = nullptr;
Render_OnTextureIndexUpdate*                             gpOnTextureIndexUpdateFunc = nullptr;

bool                                                     gNeedTextureUpdate         = false;

GraphicsAPI_t                                            gGraphicsAPIData;

// tracy contexts
#if TRACY_ENABLE
static std::unordered_map< VkCommandBuffer, TracyVkCtx > gTracyCtx;
#endif


CONVAR_CMD_EX( r_msaa, 1, CVARF_ARCHIVE, "Enable/Disable MSAA Globally" )
{
	VK_Reset( ERenderResetFlags_MSAA );
}


CONVAR_CMD_EX( r_msaa_samples, 8, CVARF_ARCHIVE, "Set the Default Amount of MSAA Samples Globally" )
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
		Log_FatalF( gLC_Render, "Vulkan Error: %s: %s", spArgs, VKString( sResult ) );
		return;
	}

	std::string argString;
	argString.resize( std::size_t( len ) + 1, '\0' );
	std::vsnprintf( argString.data(), argString.size(), spArgs, args );
	argString.resize( len );

	va_end( args );

	VK_CheckResult( sResult, argString.data() );
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
void VK_CheckResultE( VkResult sResult, char const* spMsg )
{
	if ( sResult == VK_SUCCESS )
		return;

	Log_ErrorF( gLC_Render, "Vulkan Error: %s: %s", spMsg, VKString( sResult ) );
}


void VK_CheckResultE( VkResult sResult )
{
	if ( sResult == VK_SUCCESS )
		return;

	Log_ErrorF( gLC_Render, "Vulkan Error: %s", VKString( sResult ) );
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

		case VK_FORMAT_BC1_RGB_SRGB_BLOCK:
			return GraphicsFmt::BC1_RGB_SRGB_BLOCK;

		case VK_FORMAT_BC3_SRGB_BLOCK:
			return GraphicsFmt::BC3_SRGB_BLOCK;

		case VK_FORMAT_BC7_SRGB_BLOCK:
			return GraphicsFmt::BC7_SRGB_BLOCK;

		// ------------------------------------------

		case VK_FORMAT_B8G8R8A8_SRGB:
			return GraphicsFmt::BGRA8888_SRGB;

		case VK_FORMAT_B8G8R8A8_SNORM:
			return GraphicsFmt::BGRA8888_SNORM;

		case VK_FORMAT_B8G8R8A8_UNORM:
			return GraphicsFmt::BGRA8888_UNORM;

		// ------------------------------------------

		case VK_FORMAT_R8G8B8A8_SRGB:
			return GraphicsFmt::RGBA8888_SRGB;

		case VK_FORMAT_R8G8B8A8_SNORM:
			return GraphicsFmt::RGBA8888_SNORM;

		case VK_FORMAT_R8G8B8A8_UNORM:
			return GraphicsFmt::RGBA8888_UNORM;

		// ------------------------------------------

		case VK_FORMAT_D16_UNORM:
			return GraphicsFmt::D16_UNORM;

		case VK_FORMAT_D32_SFLOAT:
			return GraphicsFmt::D32_SFLOAT;

		case VK_FORMAT_D32_SFLOAT_S8_UINT:
			return GraphicsFmt::D32_SFLOAT_S8_UINT;
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

		case GraphicsFmt::D32_SFLOAT_S8_UINT:
			return VK_FORMAT_D32_SFLOAT_S8_UINT;
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


EImageUsage VK_ToImageUsage( VkImageUsageFlags usage )
{
	EImageUsage flags = 0;

	if ( usage & VK_IMAGE_USAGE_TRANSFER_SRC_BIT )
		flags |= EImageUsage_TransferSrc;

	if ( usage & VK_IMAGE_USAGE_TRANSFER_DST_BIT )
		flags |= EImageUsage_TransferDst;

	if ( usage & VK_IMAGE_USAGE_SAMPLED_BIT )
		flags |= EImageUsage_Sampled;

	if ( usage & VK_IMAGE_USAGE_STORAGE_BIT )
		flags |= EImageUsage_Storage;

	if ( usage & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT )
		flags |= EImageUsage_AttachColor;

	if ( usage & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT )
		flags |= EImageUsage_AttachDepthStencil;

	if ( usage & VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT )
		flags |= EImageUsage_AttachTransient;

	if ( usage & VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT )
		flags |= EImageUsage_AttachInput;

	return flags;
}


VkAttachmentLoadOp VK_ToVkLoadOp( EAttachmentLoadOp loadOp )
{
	switch ( loadOp )
	{
		default:
			return VK_ATTACHMENT_LOAD_OP_MAX_ENUM;

		case EAttachmentLoadOp_Load:
			return VK_ATTACHMENT_LOAD_OP_LOAD;

		case EAttachmentLoadOp_Clear:
			return VK_ATTACHMENT_LOAD_OP_CLEAR;

		case EAttachmentLoadOp_DontCare:
			return VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	}
}


VkAttachmentStoreOp VK_ToVkStoreOp( EAttachmentStoreOp storeOp )
{
	switch ( storeOp )
	{
		default:
			return VK_ATTACHMENT_STORE_OP_MAX_ENUM;

		case EAttachmentStoreOp_Store:
			return VK_ATTACHMENT_STORE_OP_STORE;

		case EAttachmentStoreOp_DontCare:
			return VK_ATTACHMENT_STORE_OP_DONT_CARE;
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


VkSamplerAddressMode VK_ToVkSamplerAddress( ESamplerAddressMode mode )
{
	switch ( mode )
	{
		default:
		case ESamplerAddressMode_Repeat:
			return VK_SAMPLER_ADDRESS_MODE_REPEAT;

		case ESamplerAddressMode_MirroredRepeast:
			return VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;

		case ESamplerAddressMode_ClampToEdge:
			return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;

		case ESamplerAddressMode_ClampToBorder:
			return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;

		case ESamplerAddressMode_MirrorClampToEdge:
			return VK_SAMPLER_ADDRESS_MODE_MIRROR_CLAMP_TO_EDGE;
	}
}


// blech
VkPipelineStageFlags VK_ToPipelineStageFlags( EPipelineStageFlags sFlags )
{
	VkPipelineStageFlags flags = VK_PIPELINE_STAGE_NONE;

	if ( sFlags & EPipelineStage_TopOfPipe )
		flags |= VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;

	if ( sFlags & EPipelineStage_DrawIndirect )
		flags |= VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT;

	if ( sFlags & EPipelineStage_VertexInput )
		flags |= VK_PIPELINE_STAGE_VERTEX_INPUT_BIT;

	if ( sFlags & EPipelineStage_VertexShader )
		flags |= VK_PIPELINE_STAGE_VERTEX_SHADER_BIT;

	if ( sFlags & EPipelineStage_TessellationControlShader )
		flags |= VK_PIPELINE_STAGE_TESSELLATION_CONTROL_SHADER_BIT;

	if ( sFlags & EPipelineStage_TessellationEvaluationShader )
		flags |= VK_PIPELINE_STAGE_TESSELLATION_EVALUATION_SHADER_BIT;

	if ( sFlags & EPipelineStage_GeometryShader )
		flags |= VK_PIPELINE_STAGE_GEOMETRY_SHADER_BIT;

	if ( sFlags & EPipelineStage_FragmentShader )
		flags |= VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;

	if ( sFlags & EPipelineStage_EarlyFragmentTests )
		flags |= VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;

	if ( sFlags & EPipelineStage_LateFragmentTests )
		flags |= VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;

	if ( sFlags & EPipelineStage_ColorAttachmentOutput )
		flags |= VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

	if ( sFlags & EPipelineStage_ComputeShader )
		flags |= VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;

	if ( sFlags & EPipelineStage_Transfer )
		flags |= VK_PIPELINE_STAGE_TRANSFER_BIT;

	if ( sFlags & EPipelineStage_BottomOfPipe )
		flags |= VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;

	if ( sFlags & EPipelineStage_Host )
		flags |= VK_PIPELINE_STAGE_HOST_BIT;

	if ( sFlags & EPipelineStage_AllGraphics )
		flags |= VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT;

	if ( sFlags & EPipelineStage_AllCommands )
		flags |= VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;

	return flags;
}


VkAccessFlags VK_ToAccessFlags( EGraphicsAccessFlags sFlags )
{
	VkAccessFlags flags = VK_ACCESS_NONE;

	if ( sFlags & EGraphicsAccess_IndirectCommandRead )
		flags |= VK_ACCESS_INDIRECT_COMMAND_READ_BIT;

	if ( sFlags & EGraphicsAccess_IndexRead )
		flags |= VK_ACCESS_INDEX_READ_BIT;

	if ( sFlags & EGraphicsAccess_VertexAttributeRead )
		flags |= VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT;

	if ( sFlags & EGraphicsAccess_UniformRead )
		flags |= VK_ACCESS_UNIFORM_READ_BIT;

	if ( sFlags & EGraphicsAccess_InputAttachemntRead )
		flags |= VK_ACCESS_INPUT_ATTACHMENT_READ_BIT;

	if ( sFlags & EGraphicsAccess_ShaderRead )
		flags |= VK_ACCESS_SHADER_READ_BIT;

	if ( sFlags & EGraphicsAccess_ShaderWrite )
		flags |= VK_ACCESS_SHADER_WRITE_BIT;

	if ( sFlags & EGraphicsAccess_ColorAttachmentRead )
		flags |= VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;

	if ( sFlags & EGraphicsAccess_ColorAttachmentWrite )
		flags |= VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

	if ( sFlags & EGraphicsAccess_DepthStencilRead )
		flags |= VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;

	if ( sFlags & EGraphicsAccess_DepthStencilWrite )
		flags |= VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

	if ( sFlags & EGraphicsAccess_TransferRead )
		flags |= VK_ACCESS_TRANSFER_READ_BIT;

	if ( sFlags & EGraphicsAccess_TransferWrite )
		flags |= VK_ACCESS_TRANSFER_WRITE_BIT;

	if ( sFlags & EGraphicsAccess_HostRead )
		flags |= VK_ACCESS_HOST_READ_BIT;

	if ( sFlags & EGraphicsAccess_HostWrite )
		flags |= VK_ACCESS_HOST_WRITE_BIT;

	if ( sFlags & EGraphicsAccess_MemoryRead )
		flags |= VK_ACCESS_MEMORY_READ_BIT;

	if ( sFlags & EGraphicsAccess_MemoryWrite )
		flags |= VK_ACCESS_MEMORY_WRITE_BIT;

	return flags;
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

	// create a vertex buffer
	VkBufferCreateInfo aBufferInfo{ VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
	aBufferInfo.size        = spBuffer->aSize;
	aBufferInfo.usage       = sUsage;
	aBufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	VK_CheckResult( vkCreateBuffer( VK_GetDevice(), &aBufferInfo, nullptr, &spBuffer->aBuffer ), "Failed to create buffer" );

	// allocate memory for the vertex buffer
	VkMemoryRequirements aMemReqs;
	vkGetBufferMemoryRequirements( VK_GetDevice(), spBuffer->aBuffer, &aMemReqs );

	VkMemoryAllocateInfo aMemAllocInfo{ VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
	aMemAllocInfo.allocationSize  = aMemReqs.size;
	aMemAllocInfo.memoryTypeIndex = VK_GetMemoryType( aMemReqs.memoryTypeBits, sMemBits );

	VK_CheckResult( vkAllocateMemory( VK_GetDevice(), &aMemAllocInfo, nullptr, &spBuffer->aMemory ), "Failed to allocate buffer memory" );

	// bind the vertex buffer to the device memory
	VK_CheckResult( vkBindBufferMemory( VK_GetDevice(), spBuffer->aBuffer, spBuffer->aMemory, 0 ), "Failed to bind buffer" );

	gBuffers.push_back( spBuffer->aBuffer );
}


void VK_CreateBuffer( const char* spName, BufferVK* spBuffer, VkBufferUsageFlags sUsage, VkMemoryPropertyFlags sMemBits )
{
	VK_CreateBuffer( spBuffer, sUsage, sMemBits );

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
			(uint64_t)spBuffer->aBuffer,                         // objectHandle
			spName,                                              // pObjectName
		};

		VK_CheckResultE( pfnSetDebugUtilsObjectName( VK_GetDevice(), &nameInfo ), "Failed to set VkBuffer Debug Name" );
	}
#endif
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

	VK_CreateCommandPool( VK_GetSingleTimeCommandPool(), gGraphicsAPIData.aQueueFamilyGraphics );
	VK_CreateCommandPool( VK_GetPrimaryCommandPool(), gGraphicsAPIData.aQueueFamilyGraphics );
	VK_CreateCommandPool( VK_GetTransferCommandPool(), gGraphicsAPIData.aQueueFamilyTransfer );

	VK_CreateSwapchain();
	VK_CreateFences();
	VK_CreateSemaphores();
	VK_CreateDescSets();

	VK_AllocateCommands();

	const auto& deviceProps = VK_GetPhysicalDeviceProperties();

	gpViewports             = new VkViewport[deviceProps.limits.maxViewports];
	gMaxViewports           = deviceProps.limits.maxViewports;

	VK_CreateTextureSamplers();
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
	VK_DestroyCommandPool( VK_GetTransferCommandPool() );
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


class RenderVK : public IRender
{
public:
	// --------------------------------------------------------------------------------------------
	// ISystem Functions
	// --------------------------------------------------------------------------------------------

	bool Init() override
	{
		// this really should not be here of all things
		if ( SDL_Init( SDL_INIT_VIDEO | SDL_INIT_EVENTS | SDL_INIT_AUDIO ) != 0 )
			Log_Fatal( "Unable to initialize SDL2!" );

		std::string windowName;

		windowName = ( Core_GetAppInfo().apWindowTitle ) ? Core_GetAppInfo().apWindowTitle : "Chocolate Engine";
		windowName += vstring( " - Build %zd - Compiled On - %s %s", Core_GetBuildNumber(), Core_GetBuildDate(), Core_GetBuildTime() );

#ifdef _WIN32
		void* window = Sys_CreateWindow( windowName.c_str(), gWidth, gHeight );

		if ( !window )
		{
			Log_Error( gLC_Render, "Failed to create window\n" );
			return false;
		}

		gpWindow = SDL_CreateWindowFrom( window );
#else
		int flags = SDL_WINDOW_VULKAN | SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI;

		if ( gMaxWindow )
			flags |= SDL_WINDOW_MAXIMIZED;

		gpWindow = SDL_CreateWindow( windowName.c_str(), SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
		                             gWidth, gHeight, flags );
#endif

		if ( !gpWindow )
		{
			Log_Error( gLC_Render, "Failed to create SDL2 Window\n" );
			return false;
		}

#ifdef _WIN32
		if ( !Render_Init( window ) )
			return false;
#else
		if ( !Render_Init( gpWindow ) )
			return false;
#endif

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
		for ( u32 i = 0; i < gGraphicsAPIData.aBufferCopies.aCapacity; i++ )
		{
			QueuedBufferCopy_t& bufferCopy = gGraphicsAPIData.aBufferCopies.apData[ i ];

			if ( bufferCopy.apRegions )
				free( bufferCopy.apRegions );

			bufferCopy.apRegions = nullptr;
		}

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

		bool ret = VK_CreateImGuiFonts();
		gFontData.clear();
		return ret;
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

	ImTextureID AddTextureToImGui( ChHandle_t sHandle ) override
	{
		if ( sHandle == CH_INVALID_HANDLE )
		{
			Log_ErrorF( gLC_Render, "AddTextureToImGui(): Invalid Image Handle!\n" );
			return nullptr;
		}

		// Is this already loaded?
		auto it = gImGuiTextures.find( sHandle );
		if ( it != gImGuiTextures.end() )
		{
			return it->second;
		}

		TextureVK* tex = VK_GetTexture( sHandle );
		if ( tex == nullptr )
		{
			Log_ErrorF( gLC_Render, "AddTextureToImGui(): No Vulkan Texture created for Image!\n" );
			return nullptr;
		}

		// imgui can't handle 2d array textures
		if ( tex->aRenderTarget || !( tex->aUsage & VK_IMAGE_USAGE_SAMPLED_BIT ) || tex->aViewType != VK_IMAGE_VIEW_TYPE_2D )
		{
			return nullptr;
		}

		// VK_SetImageLayout( tex->aImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 1 );

		auto desc = ImGui_ImplVulkan_AddTexture( VK_GetSampler( tex->aFilter, VK_SAMPLER_ADDRESS_MODE_REPEAT, VK_FALSE ), tex->aImageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL );

		if ( desc )
		{
			gImGuiTextures[ sHandle ] = desc;
			return desc;
		}

		printf( "Render_AddTextureToImGui(): Failed to add texture to ImGui\n" );
		return nullptr;
	}

	// bool BuildImGuiFonts()
	// {
	// 	return true;
	// }

	// --------------------------------------------------------------------------------------------
	// Buffers
	// --------------------------------------------------------------------------------------------
	
	Handle CreateBuffer( const char* spName, u32 sSize, EBufferFlags sBufferFlags, EBufferMemory sBufferMem ) override
	{
		PROF_SCOPE();

		BufferVK* buffer = nullptr;
		Handle    handle = gBufferHandles.Create( &buffer );

		buffer->aBuffer  = VK_NULL_HANDLE;
		buffer->aMemory  = VK_NULL_HANDLE;
		buffer->aSize    = sSize;

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
	
	ChHandle_t CreateBuffer( u32 sSize, EBufferFlags sBufferFlags, EBufferMemory sBufferMem ) override
	{
		return CreateBuffer( nullptr, sSize, sBufferFlags, sBufferMem );
	}

	void DestroyBuffer( ChHandle_t buffer ) override
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

	void DestroyBuffers( ChHandle_t* spBuffers, u32 sCount ) override
	{
		for ( u32 i = 0; i < sCount; i++ )
			DestroyBuffer( spBuffers[ i ] );
	}

	virtual u32 BufferWrite( Handle buffer, u32 sSize, void* spData ) override
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

	virtual u32 BufferRead( Handle buffer, u32 sSize, void* spData ) override
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

	virtual bool BufferCopy( Handle shSrc, Handle shDst, BufferRegionCopy_t* spRegions, u32 sRegionCount ) override
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
		}

		// TODO: PERF: only use the transfer queue if you need to copy from host to device local memory
		// this is slower than the graphics queue when copying from device local to device local
		VkCommandBuffer c = VK_BeginOneTimeTransferCommand();
		vkCmdCopyBuffer( c, bufSrc->aBuffer, bufDst->aBuffer, sRegionCount, regions );
		VK_EndOneTimeTransferCommand( c );

		CH_STACK_FREE( regions );

		return true;
	}

	virtual bool BufferCopyQueued( Handle shSrc, Handle shDst, BufferRegionCopy_t* spRegions, u32 sRegionCount ) override
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
			bufferCopy.apRegions = ch_malloc_count< VkBufferCopy >( sRegionCount );

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

	// --------------------------------------------------------------------------------------------
	// Materials and Textures
	// --------------------------------------------------------------------------------------------

	ChHandle_t LoadTexture( ChHandle_t& srHandle, const std::string& srTexturePath, const TextureCreateData_t& srCreateData ) override
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
			// TODO: We should probably allocate a handle and a new texture anyway,
			// so if it appears on the disk, we can use it in hotloading later
			gTexturePaths[ srTexturePath ] = InvalidHandle;
			Log_ErrorF( gLC_Render, "Failed to find Texture: \"%s\"\n", srTexturePath.c_str() );
			return InvalidHandle;
		}

		if ( srHandle )
		{
#pragma message( "TODO: HANDLE UPDATING IMGUI TEXTURES WHEN REPLACING THE DATA IN THE HANDLE" )
			Log_Dev( 1, "TODO:  HANDLE UPDATING IMGUI TEXTURES WHEN REPLACING THE DATA IN THE HANDLE\n" );

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

			if ( !VK_LoadTexture( srHandle, tex, fullPath, srCreateData ) )
			{
				VK_DestroyTexture( srHandle );
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
			TextureVK* tex = VK_NewTexture( srHandle );
			if ( !VK_LoadTexture( srHandle, tex, fullPath, srCreateData ) )
			{
				VK_DestroyTexture( srHandle );
				return InvalidHandle;
			}

			gTexturePaths[ srTexturePath ] = srHandle;
			gTextureInfo[ srHandle ]       = srCreateData;
		}

		return srHandle;
	}

	ChHandle_t CreateTexture( const TextureCreateInfo_t& srTextureCreateInfo, const TextureCreateData_t& srCreateData ) override
	{
		ChHandle_t handle = CH_INVALID_HANDLE;
		TextureVK* tex    = VK_CreateTexture( handle, srTextureCreateInfo, srCreateData );
		if ( tex == nullptr )
			return InvalidHandle;

		gTextureInfo[ handle ] = srCreateData;
		return handle;
	}

	void FreeTexture( ChHandle_t sTexture ) override
	{
		VK_DestroyTexture( sTexture );
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

	// EImageUsage GetTextureUsage( ChHandle_t shTexture ) override
	// {
	// 	VK_ToImageUsage();
	// }

	int GetTextureIndex( ChHandle_t shTexture ) override
	{
		PROF_SCOPE();

		if ( shTexture == InvalidHandle )
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

	GraphicsFmt GetTextureFormat( ChHandle_t shTexture ) override
	{
		PROF_SCOPE();

		TextureVK* tex = VK_GetTexture( shTexture );

		if ( tex )
			return VK_ToGraphicsFmt( tex->aFormat );

		Log_ErrorF( gLC_Render, "GetTextureFormat: Failed to find texture %zd\n", shTexture );
		return GraphicsFmt::INVALID;
	}
	
	glm::uvec2 GetTextureSize( ChHandle_t shTexture ) override
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
	
	const ChVector< ChHandle_t >& GetTextureList() override
	{
		return gTextureHandles.aHandles;
	}
	
	TextureInfo_t GetTextureInfo( ChHandle_t sTexture ) override
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

		if ( tex->apName )
			info.aName = tex->apName;

		info.aSize     = tex->aSize;
		info.aGpuIndex = tex->aIndex;
		// info.aViewType = tex->aViewType

		for ( auto& [ path, handle ] : gTexturePaths )
		{
			if ( handle == sTexture )
			{
				info.aPath = path;

				if ( info.aName.empty() )
					info.aName = FileSys_GetFileName( path );

				break;
			}
		}

		return info;
	}

	Handle CreateFramebuffer( const CreateFramebuffer_t& srCreate ) override
	{
		return VK_CreateFramebuffer( srCreate );
	}

	void DestroyFramebuffer( Handle shTarget ) override
	{
		if ( shTarget == InvalidHandle )
			return;

		VK_DestroyFramebuffer( shTarget );
	}

	glm::uvec2 GetFramebufferSize( Handle shFramebuffer ) override
	{
		if ( shFramebuffer == InvalidHandle )
			return {};

		return VK_GetFramebufferSize( shFramebuffer );
	}

	void SetTextureDescSet( ChHandle_t* spDescSets, int sCount, u32 sBinding ) override
	{
		VK_SetImageSets( spDescSets, sCount, sBinding );
	}

	// --------------------------------------------------------------------------------------------
	// Shader System
	// --------------------------------------------------------------------------------------------

	bool CreatePipelineLayout( ChHandle_t& sHandle, PipelineLayoutCreate_t& srPipelineCreate ) override
	{
		return VK_CreatePipelineLayout( sHandle, srPipelineCreate );
	}

	bool CreateGraphicsPipeline( ChHandle_t& sHandle, GraphicsPipelineCreate_t& srGraphicsCreate ) override
	{
		return VK_CreateGraphicsPipeline( sHandle, srGraphicsCreate );
	}

	bool CreateComputePipeline( ChHandle_t& sHandle, ComputePipelineCreate_t& srComputeCreate ) override
	{
		return VK_CreateComputePipeline( sHandle, srComputeCreate );
	}

	void DestroyPipeline( ChHandle_t sPipeline ) override
	{
		VK_DestroyPipeline( sPipeline );
	}

	void DestroyPipelineLayout( Handle sPipeline ) override
	{
		VK_DestroyPipelineLayout( sPipeline );
	}

	// --------------------------------------------------------------------------------------------
	// Descriptor Pool
	// --------------------------------------------------------------------------------------------

	Handle CreateDescLayout( const CreateDescLayout_t& srCreate ) override
	{
		return VK_CreateDescLayout( srCreate );
	}

	void UpdateDescSets( WriteDescSet_t* spUpdate, u32 sCount ) override
	{
		VK_UpdateDescSets( spUpdate, sCount );
	}
	
	bool AllocateVariableDescLayout( const AllocVariableDescLayout_t& srCreate, Handle* handles ) override
	{
		return VK_AllocateVariableDescLayout( srCreate, handles );
	}
	
	bool AllocateDescLayout( const AllocDescLayout_t& srCreate, Handle* handles ) override
	{
		return VK_AllocateDescLayout( srCreate, handles );
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

	void SetTextureIndexUpdateCallback( Render_OnTextureIndexUpdate* spFunc ) override
	{
		gpOnTextureIndexUpdateFunc = spFunc;
	}

	void NewFrame() override
	{
		ImGui_ImplVulkan_NewFrame();
	}

	void Reset() override
	{
		VK_Reset();
	}

	void PreRenderPass() override
	{
	}

	void CopyQueuedBuffers() override
	{
		VkCommandBuffer c = VK_BeginOneTimeTransferCommand();

		for ( QueuedBufferCopy_t& bufferCopy : gGraphicsAPIData.aBufferCopies )
		{
			vkCmdCopyBuffer( c, bufferCopy.aSource, bufferCopy.aDest, bufferCopy.aRegionCount, bufferCopy.apRegions );
		}

		VK_EndOneTimeTransferCommand( c );

		gGraphicsAPIData.aBufferCopies.clear();
	}

	u32 GetFlightImageIndex() override
	{
		return VK_GetNextImage();
	}

	void Present( u32 sImageIndex ) override
	{
		VK_Present( sImageIndex );
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
	u32 GetCommandBufferHandles( Handle* spHandles ) override
	{
		extern ResourceList< VkCommandBuffer > gCommandBufferHandles;  // wtf

		if ( spHandles != nullptr )
		{
			for ( size_t i = 0; i < gCommandBufferHandles.size(); i++ )
			{
				spHandles[ i ] = gCommandBufferHandles.GetHandleByIndex( i );
			}
		}

		return gCommandBufferHandles.size();
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
		PROF_SCOPE();

		VkCommandBuffer c = VK_GetCommandBuffer( cmd );

		// TODO: expose this
		VkCommandBufferBeginInfo begin{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
		begin.pNext            = nullptr;
		begin.flags            = 0;
		begin.pInheritanceInfo = nullptr;

		VK_CheckResult( vkBeginCommandBuffer( c, &begin ), "Failed to begin command buffer" );

		// gTracyCtx[ c ] = TracyVkContext( VK_GetPhysicalDevice(), VK_GetDevice(), VK_GetGraphicsQueue(), c );
	}

	void EndCommandBuffer( Handle cmd ) override
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
					clearValues[ i ].depthStencil.depth   = srBegin.aClear[ i ].aColor.w;
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
		PROF_SCOPE();

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
		PROF_SCOPE();

		VkCommandBuffer c = VK_GetCommandBuffer( cmd );
		ImGui_ImplVulkan_RenderDrawData( spDrawData, c );
	}

	// --------------------------------------------------------------------------------------------
	// Vulkan Commands
	// --------------------------------------------------------------------------------------------

	void CmdSetViewport( Handle cmd, u32 sOffset, const Viewport_t* spViewports, u32 sCount ) override
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

	void CmdSetScissor( Handle cmd, u32 sOffset, const Rect2D_t* spScissors, u32 sCount ) override
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

	void CmdSetDepthBias( Handle cmd, float sConstantFactor, float sClamp, float sSlopeFactor ) override
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

	void CmdSetLineWidth( Handle cmd, float sLineWidth ) override
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

	bool CmdBindPipeline( Handle cmd, Handle shader ) override
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

	void CmdPushConstants( Handle      cmd,
	                       Handle      shPipelineLayout,
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

	void CmdBindVertexBuffers( Handle cmd, u32 sFirstBinding, u32 sCount, const Handle* spBuffers, const uint64_t* spOffsets ) override
	{
		PROF_SCOPE();

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

			if ( !bufVK )
			{
				Log_Error( gLC_Render, "CmdBindVertexBuffers: Failed to find Buffer\n" );
				CH_STACK_FREE( vkBuffers );
				return;
			}

			vkBuffers[ i ]  = bufVK->aBuffer;
		}

		vkCmdBindVertexBuffers( c, sFirstBinding, sCount, vkBuffers, spOffsets );
		CH_STACK_FREE( vkBuffers );
	}

	void CmdBindIndexBuffer( Handle cmd, Handle shBuffer, size_t offset, EIndexType indexType ) override
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
	
	void CmdDraw( Handle cmd, u32 sVertCount, u32 sInstanceCount, u32 sFirstVert, u32 sFirstInstance ) override
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

	void CmdDrawIndexed( Handle cmd, u32 sIndexCount, u32 sInstanceCount, u32 sFirstIndex, int sVertOffset, u32 sFirstInstance ) override
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

	void CmdDispatch( ChHandle_t sCmd, u32 sGroupCountX, u32 sGroupCountY, u32 sGroupCountZ ) override
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

	void CmdPipelineBarrier( ChHandle_t sCmd, PipelineBarrier_t& srBarrier ) override
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
	DLL_EXPORT ModuleInterface_t* cframework_GetInterfaces( size_t& srCount )
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

