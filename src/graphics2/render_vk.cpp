// #include "../render.h"
#include "core/platform.h"
#include "core/filesystem.h"
#include "core/log.h"
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

int                  gWidth  = 1280;
int                  gHeight = 720;

SDL_Window*          gpWindow = nullptr;

static VkCommandPool gSingleTime;
static VkCommandPool gPrimary;

// static std::unordered_map< ImageInfo*, VkDescriptorSet > gImGuiTextures;
static std::vector< std::vector< char > > gFontData;

static std::vector< VkBuffer >            gBuffers;

static ResourceList< BufferVK >           gBufferHandles;
static ResourceList< TextureVK* >         gTextureHandles;
// static ResourceManager< BufferVK >        gBufferHandles;


char const* VKString( VkResult sResult )
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


VkFormat VK_ToVkFormat( GraphicsFmt colorFmt )
{
	switch ( colorFmt )
	{
		case GraphicsFmt::INVALID:
		default:
			Log_Error( gLC_Render, "Unspecified Color Format to VkFormat Conversion!\n" );
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


void VK_CheckResult( VkResult sResult, char const* spMsg )
{
	if ( sResult == VK_SUCCESS )
		return;

	char pBuf[ 1024 ];
	snprintf( pBuf, sizeof( pBuf ), "Vulkan Error %s: %s", spMsg, VKString( sResult ) );

	// SDL_ShowSimpleMessageBox( SDL_MESSAGEBOX_ERROR, "Vulkan Error", pBuf, nullptr );
	Log_Fatal( gLC_Render, pBuf );
}


void VK_CheckResult( VkResult sResult )
{
	if ( sResult == VK_SUCCESS )
		return;

	char pBuf[ 1024 ];
	snprintf( pBuf, sizeof( pBuf ), "Vulkan Error: %s", VKString( sResult ) );

	// SDL_ShowSimpleMessageBox( SDL_MESSAGEBOX_ERROR, "Vulkan Error", pBuf, nullptr );
	Log_Fatal( gLC_Render, pBuf );
}


// Copies memory to the GPU.
void VK_memcpy( VkDeviceMemory sBufferMemory, VkDeviceSize sSize, const void* spData )
{
	void* pData;
	VK_CheckResult( vkMapMemory( VK_GetDevice(), sBufferMemory, 0, sSize, 0, &pData ), "Vulkan: Failed to map memory" );
	memcpy( pData, spData, (size_t)sSize );
	vkUnmapMemory( VK_GetDevice(), sBufferMemory );
}


VkSampleCountFlagBits VK_GetMSAASamples()
{
	return VK_SAMPLE_COUNT_1_BIT;
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


void VK_ClearDrawQueue()
{
}


bool VK_CreateImGuiFonts()
{
	VkCommandBuffer c = VK_BeginSingleCommand();

	if ( !ImGui_ImplVulkan_CreateFontsTexture( c ) )
	{
		Log_Error( gLC_Render, "VK_CreateImGuiFonts(): Failed to create ImGui Fonts Texture!\n" );
		return false;
	}

	VK_EndSingleCommand();

	ImGui_ImplVulkan_DestroyFontUploadObjects();

	// TODO: manually free FontData in ImGui fonts, why is it holding onto it and trying to free() it on shutdown?
	return true;
}


bool VK_InitImGui()
{
	ImGui_ImplSDL2_InitForVulkan( gpWindow );

	ImGui_ImplVulkan_InitInfo init_info{};
	init_info.Instance        = VK_GetInstance();
	init_info.PhysicalDevice  = VK_GetPhysicalDevice();
	init_info.Device          = VK_GetDevice();
	init_info.Queue           = VK_GetGraphicsQueue();
	init_info.DescriptorPool  = VK_GetDescPool();
	init_info.MinImageCount   = VK_GetSwapImageCount();
	init_info.ImageCount      = VK_GetSwapImageCount();
	init_info.MSAASamples     = VK_GetMSAASamples();
	init_info.CheckVkResultFn = VK_CheckResult;

	if ( !ImGui_ImplVulkan_Init( &init_info, VK_GetRenderPass() ) )
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
	
	if ( !VK_InitImGui() )
	{
		Log_Error( gLC_Render, "Failed to init ImGui for Vulkan\n" );
		Render_Shutdown();
		return false;
	}

	// Load up image shader and create buffer for image mesh
	// VK_CreateImageLayout();
	// VK_CreateImageStorageLayout();

	// VK_CreateImageShader();
	// VK_CreateFilterShader();

	Log_Msg( gLC_Render, "Loaded Vulkan Renderer\n" );

	return true;
}


void Render_Shutdown()
{
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


void VK_Reset()
{
	VK_RecreateSwapchain();

	// recreate backbuffer
	VK_DestroyRenderTargets();
	VK_GetBackBuffer();
}


void Render_NewFrame()
{
	ImGui_ImplVulkan_NewFrame();
	VK_ClearDrawQueue();
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

	auto desc = ImGui_ImplVulkan_AddTexture( VK_GetSampler(), tex->aImageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL );

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
// Render Pipeline Ideas

void Render_BindShader();
void Render_DrawModel();


// ----------------------------------------------------------------------------------
// Chocolate Render Abstraction
// need to get rid of this cause this is really stupid


class RenderVK : public IRender
{
public:
	// --------------------------------------------------------------------------------------------
	// BaseSystem Functions
	// --------------------------------------------------------------------------------------------

	void Init() override
	{
		gWidth = Args_GetInt( "-w", gWidth ); 
		gHeight = Args_GetInt( "-h", gHeight ); 

		// this really should not be here of all things
		if ( SDL_Init( SDL_INIT_VIDEO | SDL_INIT_EVENTS | SDL_INIT_AUDIO ) != 0 )
			Log_Fatal( "Unable to initialize SDL2!" );

		gpWindow = SDL_CreateWindow( "Chocolate Engine - Compiled on " __DATE__, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
		                             gWidth, gHeight, SDL_WINDOW_VULKAN | SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE );

		Render_Init( gpWindow );
		Render_BuildFonts();
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

	SDL_Window* GetWindow() override
	{
		return gpWindow;
	}

	void GetSurfaceSize( int& srWidth, int& srHeight ) override
	{
		srWidth  = gWidth;
		srHeight = gHeight;
	}

	// --------------------------------------------------------------------------------------------
	// Testing
	// --------------------------------------------------------------------------------------------
	
	Handle CreateBuffer( u32 sSize, EBufferFlags sBufferFlags, EBufferMemory sBufferMem ) override
	{
		BufferVK* buffer = nullptr;
		Handle   handle = gBufferHandles.Create( &buffer );

		// BufferVK* buffer = new BufferVK;
		// Handle   handle = gBufferHandles.Add( buffer );

		buffer->aBuffer  = nullptr;
		buffer->aMemory  = nullptr;
		buffer->aSize    = sSize;

		int flagBits = 0;

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

		VK_CreateBuffer( buffer->aBuffer, buffer->aMemory, sSize, flagBits, memBits );

		return handle;
	}

	virtual void MemWriteBuffer( Handle buffer, u32 sSize, void* spData ) override
	{
		BufferVK* bufVK = gBufferHandles.Get( buffer );

		if ( !bufVK )
		{
			Log_Error( gLC_Render, "MemWriteBuffer: Failed to find Buffer\n" );
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

		VK_SingleCommand( [ & ]( VkCommandBuffer c )
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

	Handle LoadTexture( const std::string& srTexturePath ) override
	{
		TextureVK* tex = VK_LoadTexture( srTexturePath );
		if ( tex == nullptr )
			return InvalidHandle;

		return gTextureHandles.Add( tex );
	}

	Handle CreateTexture( const TextureCreateInfo_t& srTextureCreateInfo ) override
	{
		return InvalidHandle;
	}

	void FreeTexture( Handle sTexture ) override
	{
		TextureVK* tex = nullptr;
		if ( !gTextureHandles.Get( sTexture, &tex ) )
		{
			Log_Error( gLC_Render, "FreeTexture: Failed to find texture\n" );
			return;
		}

		gTextureHandles.Remove( sTexture );
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

	// --------------------------------------------------------------------------------------------
	// Shader System
	// --------------------------------------------------------------------------------------------

	Handle CreatePipelineLayout( PipelineLayoutCreate_t& srPipelineCreate ) override
	{
		return VK_CreatePipelineLayout( srPipelineCreate );
	}

	Handle CreateGraphicsPipeline( GraphicsPipelineCreate_t& srGraphicsCreate ) override
	{
		// TODO: add a bool to the struct here so we know if we need to rebuild it or not if the swapchain resizes
		// ...or just do it in game code somehow?
		return VK_CreateGraphicsPipeline( srGraphicsCreate );
	}

	// --------------------------------------------------------------------------------------------
	// Render List
	// --------------------------------------------------------------------------------------------

	void NewFrame() override
	{
		Render_NewFrame();
	}

	void Reset() override
	{
		VK_Reset();
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

	Handle GetBackBufferColor() override
	{
		RenderTarget* backBuf = VK_GetBackBuffer();
		if ( !backBuf )
		{
			Log_Error( gLC_Render, "No Backbuffer????\n" );
			return InvalidHandle;
		}

		return VK_GetFrameBufferHandle( backBuf->aFrameBuffers[ 0 ] );
	}

	Handle GetBackBufferDepth() override
	{
		RenderTarget* backBuf = VK_GetBackBuffer();
		if ( !backBuf )
		{
			Log_Error( gLC_Render, "No Backbuffer????\n" );
			return InvalidHandle;
		}

		return VK_GetFrameBufferHandle( backBuf->aFrameBuffers[ 1 ] );
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

		Log_Fatal( gLC_Render, "Implement CreateRenderPass!!!\n" );
		VK_GetRenderPass();
		// VK_CreateRenderPass();

		return InvalidHandle;
	}

	void FreeRenderPass( Handle shPass ) override
	{
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
		renderPassBeginInfo.framebuffer       = VK_GetFrameBuffer( srBegin.aFrameBuffer );
		renderPassBeginInfo.renderArea.offset = { 0, 0 };
		renderPassBeginInfo.renderArea.extent = VK_GetSwapExtent();

		VkClearValue clearValues[ 2 ];
		clearValues[ 0 ].color              = { srBegin.aClearColor.x, srBegin.aClearColor.y, srBegin.aClearColor.z, srBegin.aClearColor.w };
		clearValues[ 1 ].depthStencil       = { 1.0f, 0 };

		renderPassBeginInfo.clearValueCount = ARR_SIZE( clearValues );
		renderPassBeginInfo.pClearValues    = clearValues;

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
		VkCommandBuffer c = VK_GetCommandBuffer( cmd );

		if ( c == nullptr )
		{
			Log_Error( gLC_Render, "CmdSetViewport: Invalid Command Buffer\n" );
			return;
		}

#if 0
		std::vector< VkViewport > vkViewports;
		for ( u32 i = 0; i < sCount; i++ )
		{
			VkViewport& vkView = vkViewports.emplace_back();
			vkView.x           = spViewports[ i ].x;
			vkView.y           = spViewports[ i ].y;
			vkView.width       = spViewports[ i ].width;
			vkView.height      = spViewports[ i ].height;
			vkView.minDepth    = spViewports[ i ].minDepth;
			vkView.maxDepth    = spViewports[ i ].maxDepth;
		}

		vkCmdSetViewport( c, sOffset, sCount, vkViewports.data() );
#else
		VkViewport* vkViewports = (VkViewport*)CH_STACK_ALLOC( sizeof( VkViewport ) * sCount );

		for ( u32 i = 0; i < sCount; i++ )
		{
			vkViewports[ i ].x           = spViewports[ i ].x;
			vkViewports[ i ].y           = spViewports[ i ].y;
			vkViewports[ i ].width       = spViewports[ i ].width;
			vkViewports[ i ].height      = spViewports[ i ].height;
			vkViewports[ i ].minDepth    = spViewports[ i ].minDepth;
			vkViewports[ i ].maxDepth    = spViewports[ i ].maxDepth;
		}

		vkCmdSetViewport( c, sOffset, sCount, vkViewports );
#endif
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

	void CmdBindPipeline( Handle cmd, Handle shader ) override
	{
		VkCommandBuffer c = VK_GetCommandBuffer( cmd );

		if ( c == nullptr )
		{
			Log_Error( gLC_Render, "CmdBindPipeline: Invalid Command Buffer\n" );
			return;
		}

		VK_BindShader( c, shader );
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

	// will most likely change
	void CmdBindDescriptorSets( Handle             cmd,
	                            size_t             sCmdIndex,
	                            EPipelineBindPoint sBindPoint,
	                            Handle             shPipelineLayout,
	                            EDescriptorLayout   sSets ) override
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

		VkPipelineBindPoint bindPoint{};

		if ( sBindPoint == EPipelineBindPoint_Graphics )
			bindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;

		else if ( sBindPoint == EPipelineBindPoint_Graphics )
			bindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;

		else if ( sBindPoint == EPipelineBindPoint_Graphics )
		{
			Log_Error( gLC_Render, "CmdBindDescriptorSets: Invalid Pipeline Bind Point\n" );
			return;
		}

		std::vector< VkDescriptorSet > sets;

		if ( sSets & EDescriptorLayout_Image )
			sets.push_back( VK_GetImageSets()[ sCmdIndex ] );

		if ( sSets & EDescriptorLayout_ImageStorage )
			sets.push_back( VK_GetImageStorage()[ sCmdIndex ] );

		vkCmdBindDescriptorSets( c, bindPoint, layout, 0, sets.size(), sets.data(), 0, nullptr );
	}

	void CmdBindVertexBuffers( Handle cmd, u32 sFirstBinding, const std::vector< Handle >& srBuffers, const std::vector< size_t >& srOffsets ) override
	{
		VkCommandBuffer c = VK_GetCommandBuffer( cmd );

		if ( c == nullptr )
		{
			Log_Error( gLC_Render, "CmdBindVertexBuffers: Invalid Command Buffer\n" );
			return;
		}

		std::vector< VkBuffer > buffers;

		for ( auto& handle : srBuffers )
		{
			BufferVK* bufVK = gBufferHandles.Get( handle );

			if ( !bufVK )
			{
				Log_Error( gLC_Render, "CmdBindVertexBuffers: Failed to find Buffer\n" );
				return;
			}

			buffers.push_back( bufVK->aBuffer );
		}

		vkCmdBindVertexBuffers( c, sFirstBinding, srBuffers.size(), buffers.data(), srOffsets.data() );
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


extern "C"
{
	DLL_EXPORT void* cframework_get()
	{
		static RenderVK renderer;
		return &renderer;
	}
}

