#pragma once

#include "render/irender.h"

#if NDEBUG
  #define VMA_STATS_STRING_ENABLED 0
#endif

#ifdef _WIN32
  #define VK_USE_PLATFORM_WIN32_KHR 1
#endif

#include <vulkan/vulkan.h>

#if TRACY_ENABLE
  #include <TracyVulkan.hpp>
#endif


#include <set>


LOG_CHANNEL2( GraphicsAPI );
LOG_CHANNEL2( Render );
LOG_CHANNEL2( Vulkan );
LOG_CHANNEL2( Validation );


constexpr const char*  CH_DEFAULT_WINDOW_NAME = "App Window";


extern VkFormat        gColorFormat;
extern VkColorSpaceKHR gColorSpace;

CONVAR_BOOL_EXT( r_msaa );
CONVAR_INT_EXT( r_msaa_samples );

extern ChHandle_t      gMissingTexHandle;

enum class GraphicsFmt;
using ShaderStage = char;
using EImageUsage = unsigned short;
enum EPipelineBindPoint : u8;
enum EAttachmentLoadOp;

struct PipelineLayoutCreate_t;
struct GraphicsPipelineCreate_t;
struct TextureCreateInfo_t;
struct RenderPassCreate_t;
class ImGuiContext;


extern PFN_vkSetDebugUtilsObjectNameEXT    pfnSetDebugUtilsObjectName;
extern PFN_vkSetDebugUtilsObjectTagEXT     pfnSetDebugUtilsObjectTag;

extern PFN_vkQueueBeginDebugUtilsLabelEXT  pfnQueueBeginDebugUtilsLabel;
extern PFN_vkQueueEndDebugUtilsLabelEXT    pfnQueueEndDebugUtilsLabel;
extern PFN_vkQueueInsertDebugUtilsLabelEXT pfnQueueInsertDebugUtilsLabel;

extern PFN_vkCmdBeginDebugUtilsLabelEXT    pfnCmdBeginDebugUtilsLabel;
extern PFN_vkCmdEndDebugUtilsLabelEXT      pfnCmdEndDebugUtilsLabel;
extern PFN_vkCmdInsertDebugUtilsLabelEXT   pfnCmdInsertDebugUtilsLabel;

#if _DEBUG
#define NV_CHECKPOINTS 0
#if NV_CHECKPOINTS
extern PFN_vkCmdSetCheckpointNV       pfnCmdSetCheckpointNV;
extern PFN_vkGetQueueCheckpointDataNV pfnGetQueueCheckpointDataNV;
#endif
#endif


struct BufferVK
{
	VkBuffer       aBuffer;
	VkDeviceMemory aMemory;
	size_t         aSize;
	const char*    apName;
};


// TODO: split this up, most of the time we don't like half of the info here, only just filling up the cpu cache more !!
struct TextureVK
{
	// TODO: move to separate array
	ch_string            aName;

	// Vulkan Info
	VkImage              aImage          = VK_NULL_HANDLE;
	VkImageView          aImageView      = VK_NULL_HANDLE;
	VkDeviceMemory       aMemory         = VK_NULL_HANDLE;
	VkImageViewType      aViewType       = VK_IMAGE_VIEW_TYPE_2D;
	VkImageUsageFlags    aUsage          = 0;
	VkFormat             aFormat         = VK_FORMAT_UNDEFINED;
	VkFilter             aFilter         = VK_FILTER_NEAREST;
	VkSamplerAddressMode aSamplerAddress = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	VkBool32             aDepthCompare   = VK_FALSE;
	u32                  aDataSize       = 0;

	// Texture Information
	int                  aIndex          = 0;  // texture sampler index (MOVE ELSEWHERE!!)

	glm::uvec2           aSize{};

	// How much memory this uses
	u32                  aMemorySize   = 0;

	int                  aFrames       = 0;
	u8                   aMipLevels    = 0;
	bool                 aRenderTarget = false;
	bool                 aSwapChain    = false;  // swapchain managed texture (wtf)
};


// Whenever you create a texture, you get a handle to one of these
// PAST ME: this saves texture memory by not having to make multiple copies of the texture for different sampler settings
// CURRENT ME: THE ABOVE MAKES NO SENSE - you already have samplers separate from textures, this solves nothing!

// if you load in multiple of the same texture, but all with different sampler settings, then each handle will be different
// but all will point to the same texture internally

// actually what do you gain with this again?
struct TextureView
{
	TextureVK*           texture        = nullptr;
	VkImageView          imageView      = VK_NULL_HANDLE;
	VkSampler            sampler        = VK_NULL_HANDLE;

	// apparently you can also change the format in VkImageView? no thanks
	// NOTE: how does this work if the image loaded is a 2D image, but you view it as a CUBE? does it just crash? probably
	VkImageViewType      viewType       = VK_IMAGE_VIEW_TYPE_2D;

	// Sampler creation settings here
	VkFilter             filter         = VK_FILTER_NEAREST;
	VkSamplerAddressMode samplerAddress = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	VkBool32             depthCompare   = VK_FALSE;
};

// VK_GetTextureView()
// render->GetTexture( texHandle, viewSettings );


struct RenderPassInfoVK
{
	bool aUsesMSAA = false;
};


struct CreateFramebufferVK
{
	const char*             apName      = nullptr;
	VkRenderPass            aRenderPass = VK_NULL_HANDLE;
	glm::uvec2              aSize{};

	ChVector< VkImageView > aAttachColors;
	// ChVector< VkImageView > aAttachInput;
	ChVector< VkImageView > aAttachResolve;
	// ChVector< VkImageView > aAttachPreserve;
	VkImageView             aAttachDepth = 0;
};


struct FramebufferVK
{
	VkFramebuffer   aBuffer;
	EAttachmentType aType;
};


// struct RenderTarget
// {
// 	std::vector< TextureVK* >    aImages;
// 	std::vector< VkFramebuffer > aFrameBuffers;
// };


struct RenderTarget
{
	ChVector< ChHandle_t >    aColors;
	// std::vector< TextureVK* > aInput;
	ChVector< ChHandle_t >    aResolve;
	// std::vector< TextureVK* > aPreserve;
	ChHandle_t                   aDepth = 0;

	// std::vector< VkImageView >   aColors;
	// std::vector< VkImageView >   aResolve;
	// VkImageView                  apDepth = 0;

	ChVector< FramebufferVK > aFrameBuffers;
};


struct QueuedBufferCopy_t
{
	VkBuffer      aSource      = VK_NULL_HANDLE;
	VkBuffer      aDest        = VK_NULL_HANDLE;
	u32           aRegionCount = 0;
	VkBufferCopy* apRegions    = nullptr;
};


struct CommandBufferGroup_t
{
	u32              aCountPerFrame;
	u32              aUsedThisFrame;
	VkCommandBuffer* apBuffers;
	VkCommandPool    aPool;
};


// idk
struct SemaphoreGroup_t
{
	VkSemaphore aImageAvailable;
	VkSemaphore aRenderFinished;
	VkSemaphore aTransferFinished;
};


// data for each window
struct WindowVK
{
	SDL_Window*                     window    = nullptr;
	ImGuiContext*                   context   = nullptr; 
	VkSurfaceKHR                    surface   = VK_NULL_HANDLE;
	VkSwapchainKHR                  swapchain = VK_NULL_HANDLE;

	// amount allocated is swapImageCount
	VkSemaphore*                    imageAvailableSemaphores;  // signals that we have finished presenting the last image
	VkSemaphore*                    renderFinishedSemaphores;  // signals that we have finished executing the last command buffer

	// only allocates to swapImageCount for the main window buffers
	VkCommandBuffer*                commandBuffers;
	ChHandle_t*                     commandBufferHandles;

	// amount allocated is swapImageCount
	VkFence*                        fences;
	VkFence*                        fencesInFlight;  // useless?

	// swapchain info
	VkSurfaceFormatKHR              swapSurfaceFormat;
	VkExtent2D                      swapExtent;
	VkImage*                        swapImages;
	VkImageView*                    swapImageViews;

	// Contains the framebuffers which are to be drawn to during command buffer recording.
	RenderTarget*                   backbuffer;

	Render_OnReset_t                onResetFunc = nullptr;

	// here because of mem alignment, 2 unused bytes at the end
	VkPresentModeKHR                swapPresentMode;
	u8                              frameIndex = 0;
	u8                              swapImageCount;
};


// TODO: migrate all the global data here
// no dont do that, that's dumb
struct GraphicsAPI_t
{
	u32                                   aQueueFamilyGraphics = UINT32_MAX;
	u32                                   aQueueFamilyTransfer = UINT32_MAX;

	CommandBufferGroup_t                  aCommandGroups[ ECommandBufferType_Count ];

	ChVector< ChHandle_t >                aSampledTextures;

	ChVector< QueuedBufferCopy_t >        aBufferCopies;

	std::unordered_map< ChHandle_t, u32 > aTextureRefs;

	// WindowVK*                             aWindows;
	// u8                                    aWindowCount;

	ResourceList< WindowVK >              windows;
	ChHandle_t                            mainWindow;
};


extern GraphicsAPI_t                  gGraphicsAPIData;


// --------------------------------------------------------------------------------------
// General

const char*                           VKString( VkResult sResult );

void                                  VK_CheckResultF( VkResult sResult, char const* spArgs, ... );
void                                  VK_CheckResult( VkResult sResult, char const* spMsg );
void                                  VK_CheckResult( VkResult sResult );

// Non Fatal Version of it, is just an error
bool                                  VK_CheckResultEF( VkResult sResult, char const* spArgs, ... );
bool                                  VK_CheckResultE( VkResult sResult, char const* spMsg );
bool                                  VK_CheckResultE( VkResult sResult );

// General Conversion Functions
GraphicsFmt                           VK_ToGraphicsFmt( VkFormat colorFmt );
VkFormat                              VK_ToVkFormat( GraphicsFmt colorFmt );
VkShaderStageFlags                    VK_ToVkShaderStage( ShaderStage stage );
VkPipelineBindPoint                   VK_ToPipelineBindPoint( EPipelineBindPoint bindPoint );
VkImageUsageFlags                     VK_ToVkImageUsage( EImageUsage usage );
VkAttachmentLoadOp                    VK_ToVkLoadOp( EAttachmentLoadOp loadOp );
VkAttachmentStoreOp                   VK_ToVkStoreOp( EAttachmentStoreOp storeOp );
VkFilter                              VK_ToVkFilter( EImageFilter filter );
VkSamplerAddressMode                  VK_ToVkSamplerAddress( ESamplerAddressMode mode );
VkPipelineStageFlags                  VK_ToPipelineStageFlags( EPipelineStageFlags sFlags );
VkAccessFlags                         VK_ToAccessFlags( EGraphicsAccessFlags sFlags );

void                                  VK_memcpy( VkDeviceMemory sBufferMemory, VkDeviceSize sSize, const void* spData );
void                                  VK_memread( VkDeviceMemory sBufferMemory, VkDeviceSize sSize, void* spData );

void                                  VK_Reset( ChHandle_t windowHandle, WindowVK* window, ERenderResetFlags sFlags = ERenderResetFlags_None );
void                                  VK_ResetAll( ERenderResetFlags sFlags = ERenderResetFlags_None );

bool                                  VK_UseMSAA();
VkSampleCountFlagBits                 VK_GetMSAASamples();

const char*                           VK_ObjectTypeStr( VkObjectType type );

// --------------------------------------------------------------------------------------
// Debug Helpers

void                                  VK_SetObjectName( VkObjectType type, u64 handle, const char* name );
void                                  VK_SetObjectNameEx( VkObjectType type, u64 handle, const char* name, const char* typeName );

// --------------------------------------------------------------------------------------
// Vulkan Instance

bool                                  VK_CreateInstance();
void                                  VK_DestroyInstance();

VkSurfaceKHR                          VK_CreateSurface( void* sSysWindow, SDL_Window* sSDLWindow );
void                                  VK_DestroySurface( VkSurfaceKHR surface );

bool                                  VK_SetupPhysicalDevice( VkSurfaceKHR surface );
void                                  VK_CreateDevice( VkSurfaceKHR surface );

VkInstance                            VK_GetInstance();

VkDevice                              VK_GetDevice();
VkPhysicalDevice                      VK_GetPhysicalDevice();

VkQueue                               VK_GetGraphicsQueue();
VkQueue                               VK_GetTransferQueue();

bool                                  VK_CheckValidationLayerSupport();
VkSampleCountFlags                    VK_GetMaxMSAASamples();
VkSampleCountFlagBits                 VK_FindMaxMSAASamples();

uint32_t                              VK_GetMemoryType( uint32_t sTypeFilter, VkMemoryPropertyFlags sProperties );
void                                  VK_FindQueueFamilies( VkSurfaceKHR surface, const VkPhysicalDeviceProperties& srDeviceProps, VkPhysicalDevice sDevice, u32& srGraphics, u32& srTransfer );

void                                  VK_UpdateSwapchainInfo( VkSurfaceKHR surface );
VkSurfaceCapabilitiesKHR              VK_GetSurfaceCapabilities();
u32                                   VK_GetSurfaceImageCount();
VkSurfaceFormatKHR                    VK_ChooseSwapSurfaceFormat();
VkPresentModeKHR                      VK_ChooseSwapPresentMode();
VkExtent2D                            VK_ChooseSwapExtent( WindowVK* window );

const VkPhysicalDeviceProperties&     VK_GetPhysicalDeviceProperties();
const VkPhysicalDeviceLimits&         VK_GetPhysicalDeviceLimits();

// --------------------------------------------------------------------------------------
// Swapchain

bool                                  VK_CreateSwapchain( WindowVK* window, VkSwapchainKHR spOldSwapchain = VK_NULL_HANDLE );
void                                  VK_DestroySwapchain( WindowVK* window );
void                                  VK_RecreateSwapchain( WindowVK* window );

VkFormat                              VK_GetDepthFormat();

// --------------------------------------------------------------------------------------
// Descriptor Pool

void                                  VK_CreateDescSets();
void                                  VK_DestroyDescSets();

VkDescriptorPool                      VK_GetDescPool();
// VkDescriptorSetLayout                 VK_GetDescImageSet();
// VkDescriptorSetLayout                 VK_GetDescBufferSet();

// blech
Handle                                VK_GetSamplerLayoutHandle();
const std::vector< Handle >&          VK_GetSamplerSetsHandles();

// VkDescriptorSetLayout                 VK_GetImageLayout();
// VkDescriptorSetLayout                 VK_GetImageStorageLayout();

// const std::vector< VkDescriptorSet >& VK_GetImageSets();
// const std::vector< VkDescriptorSet >& VK_GetImageStorage();
// VkDescriptorSet                       VK_GetImageSet( size_t sIndex );
void                                  VK_UpdateImageSets();
void                                  VK_CalcTextureIndices();
void                                  VK_SetImageSets( ChHandle_t* spDescSets, int sCount, u32 sBinding );

Handle                                VK_CreateDescLayout( const CreateDescLayout_t& srCreate );
void                                  VK_UpdateDescSets( WriteDescSet_t* spUpdate, u32 sCount );

bool                                  VK_AllocateDescLayout( const AllocDescLayout_t& srCreate, Handle* handles );
bool                                  VK_AllocateVariableDescLayout( const AllocVariableDescLayout_t& srCreate, Handle* handles );


VkDescriptorSetLayout                 VK_GetDescLayout( Handle sHandle );
VkDescriptorSet                       VK_GetDescSet( Handle sHandle );

// --------------------------------------------------------------------------------------
// Command Pool

void                                  VK_CreateCommandPool( VkCommandPool& srPool, u32 sQueueFamily, VkCommandPoolCreateFlags sFlags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT );
void                                  VK_DestroyCommandPool( VkCommandPool& srPool );
void                                  VK_ResetCommandPool( VkCommandPool& srPool, VkCommandPoolResetFlags sFlags = 0 );

VkCommandPool&                        VK_GetSingleTimeCommandPool();
VkCommandPool&                        VK_GetPrimaryCommandPool();
VkCommandPool&                        VK_GetTransferCommandPool();

// --------------------------------------------------------------------------------------
// Render Pass

VkRenderPass                          VK_CreateMainRenderPass();
void                                  VK_DestroyMainRenderPass();

Handle                                VK_CreateRenderPass( const RenderPassCreate_t& srCreate );
void                                  VK_DestroyRenderPass( Handle shHandle );

void                                  VK_DestroyRenderPasses();
VkRenderPass                          VK_GetRenderPass( ChHandle_t shHandle = CH_INVALID_HANDLE );
RenderPassInfoVK*                     VK_GetRenderPassInfo( VkRenderPass renderPass );

// --------------------------------------------------------------------------------------
// Present

VkCommandBuffer                       VK_GetCommandBuffer( Handle cmd );

bool                                  VK_CreateFences( WindowVK* window );
void                                  VK_DestroyFences( WindowVK* window );

bool                                  VK_CreateSemaphores( WindowVK* window );
void                                  VK_DestroySemaphores( WindowVK* window );

bool                                  VK_AllocateCommands( WindowVK* window );
void                                  VK_FreeCommands( WindowVK* window );

void                                  VK_AllocateOneTimeCommands();
void                                  VK_FreeOneTimeCommands();

// kinda ugly
VkCommandBuffer                       VK_BeginOneTimeTransferCommand();
void                                  VK_EndOneTimeTransferCommand( VkCommandBuffer c );

VkCommandBuffer                       VK_BeginOneTimeCommand();
void                                  VK_EndOneTimeCommand( VkCommandBuffer c );
void                                  VK_OneTimeCommand( std::function< void( VkCommandBuffer ) > sFunc );

void                                  VK_WaitForTransferQueue();
void                                  VK_WaitForGraphicsQueue();

u32                                   VK_GetNextImage( ChHandle_t windowHandle, WindowVK* window );
void                                  VK_Present( ChHandle_t windowHandle, WindowVK* window, u32 sImageIndex );

void                                  VK_CheckFenceStatus( WindowVK* window );

// --------------------------------------------------------------------------------------
// Shader System

bool                                  VK_CreatePipelineLayout( ChHandle_t& sHandle, PipelineLayoutCreate_t& srPipelineCreate );
bool                                  VK_CreateGraphicsPipeline( ChHandle_t& sHandle, GraphicsPipelineCreate_t& srGraphicsCreate );
bool                                  VK_CreateComputePipeline( ChHandle_t& srHandle, ComputePipelineCreate_t& srPipelineCreate );

void                                  VK_DestroyPipeline( Handle sPipeline );
void                                  VK_DestroyPipelineLayout( Handle sPipeline );
void                                  VK_DestroyShaders();

bool                                  VK_BindShader( VkCommandBuffer c, Handle handle );
VkPipelineLayout                      VK_GetPipelineLayout( Handle handle );

// --------------------------------------------------------------------------------------
// Buffers

void                                  VK_CreateBuffer( const char* spName, BufferVK* spBuffer, VkBufferUsageFlags sUsage, VkMemoryPropertyFlags sMemBits );
void                                  VK_CreateBuffer( BufferVK* spBuffer, VkBufferUsageFlags sUsage, VkMemoryPropertyFlags sMemBits );
void                                  VK_DestroyBuffer( BufferVK* spBuffer );

// --------------------------------------------------------------------------------------
// Textures and Render Targets

void                                  VK_CreateTextureSamplers();
void                                  VK_DestroyTextureSamplers();
VkSampler                             VK_GetSampler( VkFilter sFilter, VkSamplerAddressMode addressMode, VkBool32 sDepthCompare );

TextureVK*                            VK_NewTexture( ChHandle_t& srHandle );
bool                                  VK_LoadTexture( ChHandle_t& srHandle, TextureVK* spTexture, const ch_string& srPath, const TextureCreateData_t& srCreateData );
TextureVK*                            VK_CreateTexture( ChHandle_t& srHandle, const TextureCreateInfo_t& srTextureCreateInfo, const TextureCreateData_t& srCreateData );
void                                  VK_DestroyTexture( ChHandle_t sTexture );
void                                  VK_DestroyAllTextures();
TextureVK*                            VK_GetTexture( ChHandle_t sTexture );
TextureVK*                            VK_GetTextureNoMissing( ChHandle_t sTexture );  // Same as above, but does not return the missing texture as a fallback

Handle                                VK_CreateFramebuffer( const char* name, VkRenderPass sRenderPass, u16 sWidth, u16 sHeight, const VkImageView* spAttachments, u32 sCount );
Handle                                VK_CreateFramebuffer( const char* name, const VkFramebufferCreateInfo& sCreateInfo );
Handle                                VK_CreateFramebuffer( const CreateFramebuffer_t& srCreate );
void                                  VK_DestroyFramebuffer( Handle shHandle );
VkFramebuffer                         VK_GetFramebuffer( Handle shHandle );
glm::uvec2                            VK_GetFramebufferSize( Handle shHandle );
Handle                                VK_GetFramebufferHandle( VkFramebuffer sFrameBuffer );

RenderTarget*                         VK_CreateRenderTarget( const std::vector< TextureVK* >& srImages, u16 sWidth, u16 sHeight, const std::vector< VkImageView >& srSwapImages = {} );
void                                  VK_DestroyRenderTarget( RenderTarget* spTarget );
void                                  VK_DestroyRenderTargets();

void                                  VK_CreateBackBuffer( WindowVK* window );

void                                  VK_SetImageLayout( VkImage sImage, VkImageLayout sOldLayout, VkImageLayout sNewLayout, VkImageSubresourceRange& sSubresourceRange );
void                                  VK_SetImageLayout( VkImage sImage, VkImageLayout sOldLayout, VkImageLayout sNewLayout, u32 sMipLevels );

void                                  VK_CreateMissingTexture();

// void                                  VK_AllocateAndBindTexture( TextureVK* spTexture );
void                                  VK_CreateImage( VkImageCreateInfo& srCreateInfo, TextureVK* spTexture );

// --------------------------------------------------------------------------------------
// KTX Texture Support

bool                                  KTX_Init();
void                                  KTX_Shutdown();
bool                                  KTX_LoadTexture( TextureVK* spTexture, const char* spPath );


