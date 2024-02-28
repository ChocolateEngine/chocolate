#include "render_vk.h"


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


GraphicsFmt VK_ToGraphicsFmt( VkFormat colorFmt )
{
	switch ( colorFmt )
	{
		default:
			Log_Error( gLC_Render, "Unspecified VkFormat to Color Format Conversion!\n" );

		case VK_FORMAT_UNDEFINED:
			return GraphicsFmt::INVALID;

			// ------------------------------------------

		case VK_FORMAT_BC1_RGB_UNORM_BLOCK:
			return GraphicsFmt::BC1_RGB_UNORM_BLOCK;

		case VK_FORMAT_BC1_RGB_SRGB_BLOCK:
			return GraphicsFmt::BC1_RGB_SRGB_BLOCK;

		case VK_FORMAT_BC1_RGBA_UNORM_BLOCK:
			return GraphicsFmt::BC1_RGBA_UNORM_BLOCK;

		case VK_FORMAT_BC1_RGBA_SRGB_BLOCK:
			return GraphicsFmt::BC1_RGBA_SRGB_BLOCK;

		case VK_FORMAT_BC3_UNORM_BLOCK:
			return GraphicsFmt::BC3_UNORM_BLOCK;

		case VK_FORMAT_BC3_SRGB_BLOCK:
			return GraphicsFmt::BC3_SRGB_BLOCK;

		case VK_FORMAT_BC4_UNORM_BLOCK:
			return GraphicsFmt::BC4_UNORM_BLOCK;

		case VK_FORMAT_BC4_SNORM_BLOCK:
			return GraphicsFmt::BC4_SNORM_BLOCK;

		case VK_FORMAT_BC5_UNORM_BLOCK:
			return GraphicsFmt::BC5_UNORM_BLOCK;

		case VK_FORMAT_BC5_SNORM_BLOCK:
			return GraphicsFmt::BC5_SNORM_BLOCK;

		case VK_FORMAT_BC6H_UFLOAT_BLOCK:
			return GraphicsFmt::BC6H_UFLOAT_BLOCK;

		case VK_FORMAT_BC6H_SFLOAT_BLOCK:
			return GraphicsFmt::BC6H_SFLOAT_BLOCK;

		case VK_FORMAT_BC7_UNORM_BLOCK:
			return GraphicsFmt::BC7_UNORM_BLOCK;

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

		case GraphicsFmt::RGBA8888_SNORM:
			return VK_FORMAT_R8G8B8A8_SNORM;

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


const char* VK_ObjectTypeStr( VkObjectType type )
{
	switch ( type )
	{
		case VK_OBJECT_TYPE_UNKNOWN:
			return "Unknown";

		case VK_OBJECT_TYPE_INSTANCE:
			return "Instance";

		case VK_OBJECT_TYPE_PHYSICAL_DEVICE:
			return "Physical Device";

		case VK_OBJECT_TYPE_DEVICE:
			return "Device";

		case VK_OBJECT_TYPE_QUEUE:
			return "Queue";

		case VK_OBJECT_TYPE_SEMAPHORE:
			return "Semaphore";

		case VK_OBJECT_TYPE_COMMAND_BUFFER:
			return "Command Buffer";

		case VK_OBJECT_TYPE_FENCE:
			return "Fence";

		case VK_OBJECT_TYPE_DEVICE_MEMORY:
			return "Device Memory";

		case VK_OBJECT_TYPE_BUFFER:
			return "Buffer";

		case VK_OBJECT_TYPE_IMAGE:
			return "Image";

		case VK_OBJECT_TYPE_EVENT:
			return "Event";

		case VK_OBJECT_TYPE_QUERY_POOL:
			return "Query Pool";

		case VK_OBJECT_TYPE_BUFFER_VIEW:
			return "Buffer View";

		case VK_OBJECT_TYPE_IMAGE_VIEW:
			return "Image View";

		case VK_OBJECT_TYPE_SHADER_MODULE:
			return "Shader Module";

		case VK_OBJECT_TYPE_PIPELINE_CACHE:
			return "Pipeline Cache";

		case VK_OBJECT_TYPE_PIPELINE_LAYOUT:
			return "Pipeline Layout";

		case VK_OBJECT_TYPE_RENDER_PASS:
			return "Render Pass";

		case VK_OBJECT_TYPE_PIPELINE:
			return "Pipeline";

		case VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT:
			return "Descriptor Set Layout";

		case VK_OBJECT_TYPE_SAMPLER:
			return "Sampler";

		case VK_OBJECT_TYPE_DESCRIPTOR_POOL:
			return "Descriptor Pool";

		case VK_OBJECT_TYPE_DESCRIPTOR_SET:
			return "Descriptor Set";

		case VK_OBJECT_TYPE_FRAMEBUFFER:
			return "Framebuffer";

		case VK_OBJECT_TYPE_COMMAND_POOL:
			return "Command Pool";

		case VK_OBJECT_TYPE_SAMPLER_YCBCR_CONVERSION:
			return "Sampler YCbCr Conversion";

		case VK_OBJECT_TYPE_DESCRIPTOR_UPDATE_TEMPLATE:
			return "Descriptor Update Template";

		case VK_OBJECT_TYPE_PRIVATE_DATA_SLOT:
			return "Private Data Slot";

		case VK_OBJECT_TYPE_SURFACE_KHR:
			return "KHR - Surface";

		case VK_OBJECT_TYPE_SWAPCHAIN_KHR:
			return "KHR - Swapchain";

		case VK_OBJECT_TYPE_DISPLAY_KHR:
			return "KHR - Display";

		case VK_OBJECT_TYPE_DISPLAY_MODE_KHR:
			return "KHR - Display Mode";

		case VK_OBJECT_TYPE_DEBUG_REPORT_CALLBACK_EXT:
			return "EXT - Debug Report Callback";

		case VK_OBJECT_TYPE_VIDEO_SESSION_KHR:
			return "KHR - Video Session";

		case VK_OBJECT_TYPE_VIDEO_SESSION_PARAMETERS_KHR:
			return "KHR - Video Session Paremeters";

		case VK_OBJECT_TYPE_CU_MODULE_NVX:
			return "NVX - CU Module";

		case VK_OBJECT_TYPE_CU_FUNCTION_NVX:
			return "NVX - CU Function";

		case VK_OBJECT_TYPE_DEBUG_UTILS_MESSENGER_EXT:
			return "EXT - Debug Utils Messenger";

		case VK_OBJECT_TYPE_ACCELERATION_STRUCTURE_KHR:
			return "KHR - Acceleration Structure";

		case VK_OBJECT_TYPE_VALIDATION_CACHE_EXT:
			return "EXT - Validation Cache";

		case VK_OBJECT_TYPE_ACCELERATION_STRUCTURE_NV:
			return "NVIDIA EXT - Acceleration Structure";

		case VK_OBJECT_TYPE_PERFORMANCE_CONFIGURATION_INTEL:
			return "INTEL - Performance Configuration";

		case VK_OBJECT_TYPE_DEFERRED_OPERATION_KHR:
			return "KHR - Deferred Operation";

		case VK_OBJECT_TYPE_INDIRECT_COMMANDS_LAYOUT_NV:
			return "NVIDIA EXT - Indirect Commands Layout";

		case VK_OBJECT_TYPE_CUDA_MODULE_NV:
			return "NVIDIA EXT - CUDA Module";

		case VK_OBJECT_TYPE_CUDA_FUNCTION_NV:
			return "NVIDIA EXT - CUDA Function";

		case VK_OBJECT_TYPE_BUFFER_COLLECTION_FUCHSIA:
			return "Buffer Collection Fuchsia";

		case VK_OBJECT_TYPE_MICROMAP_EXT:
			return "EXT - Micromap";

		case VK_OBJECT_TYPE_OPTICAL_FLOW_SESSION_NV:
			return "NVIDIA EXT - Optical Flow Session";

		case VK_OBJECT_TYPE_SHADER_EXT:
			return "EXT - Shader";
	}
}

