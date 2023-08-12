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


#include <unordered_set>


LOG_CHANNEL2( Render );
LOG_CHANNEL2( Vulkan );
LOG_CHANNEL2( Validation );


extern int             gWidth;
extern int             gHeight;

extern VkFormat        gColorFormat;
extern VkColorSpaceKHR gColorSpace;

extern ConVar          r_msaa;
extern ConVar          r_msaa_samples;

enum class GraphicsFmt;
using ShaderStage = char;
using EImageUsage = unsigned short;
enum EPipelineBindPoint;
enum EAttachmentLoadOp;

struct PipelineLayoutCreate_t;
struct GraphicsPipelineCreate_t;
struct TextureCreateInfo_t;
struct RenderPassCreate_t;



#if _DEBUG
extern PFN_vkSetDebugUtilsObjectNameEXT    pfnSetDebugUtilsObjectName;
extern PFN_vkSetDebugUtilsObjectTagEXT     pfnSetDebugUtilsObjectTag;

extern PFN_vkQueueBeginDebugUtilsLabelEXT  pfnQueueBeginDebugUtilsLabel;
extern PFN_vkQueueEndDebugUtilsLabelEXT    pfnQueueEndDebugUtilsLabel;
extern PFN_vkQueueInsertDebugUtilsLabelEXT pfnQueueInsertDebugUtilsLabel;

extern PFN_vkCmdBeginDebugUtilsLabelEXT    pfnCmdBeginDebugUtilsLabel;
extern PFN_vkCmdEndDebugUtilsLabelEXT      pfnCmdEndDebugUtilsLabel;
extern PFN_vkCmdInsertDebugUtilsLabelEXT   pfnCmdInsertDebugUtilsLabel;

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
};


// TODO: split this up, most of the time we don't like half of the info here, only just filling up the cpu cache more !!
struct TextureVK
{
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

	// Texture Information
	const char*          apName          = nullptr;
	int                  aIndex          = 0;  // texture sampler index (MOVE ELSEWHERE!!)
	glm::uvec2           aSize{};
	u8                   aMipLevels    = 0;
	int                  aFrames       = 0;
	bool                 aRenderTarget = false;
	bool                 aSwapChain    = false;  // swapchain managed texture (wtf)
};


struct RenderPassInfoVK
{
	bool aUsesMSAA = false;
};


struct CreateFramebufferVK
{
	VkRenderPass               aRenderPass = VK_NULL_HANDLE;
	glm::uvec2                 aSize{};

	std::vector< VkImageView > aAttachColors;
	// std::vector< VkImageView > aAttachInput;
	std::vector< VkImageView > aAttachResolve;
	// std::vector< VkImageView > aAttachPreserve;
	VkImageView                aAttachDepth = 0;
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
	std::vector< ChHandle_t >    aColors;
	// std::vector< TextureVK* > aInput;
	std::vector< ChHandle_t >    aResolve;
	// std::vector< TextureVK* > aPreserve;
	ChHandle_t                   aDepth = 0;

	// std::vector< VkImageView >   aColors;
	// std::vector< VkImageView >   aResolve;
	// VkImageView                  apDepth = 0;

	std::vector< FramebufferVK > aFrameBuffers;
};


// TODO: migrate all the global data here
struct GraphicsAPI_t
{
	std::unordered_set< ChHandle_t > aSampledTextures;
};


extern GraphicsAPI_t                  gGraphicsAPIData;


// --------------------------------------------------------------------------------------
// General

const char*                           VKString( VkResult sResult );

void                                  VK_CheckResultF( VkResult sResult, char const* spArgs, ... );
void                                  VK_CheckResult( VkResult sResult, char const* spMsg );
void                                  VK_CheckResult( VkResult sResult );

// Non Fatal Version of it, is just an error
void                                  VK_CheckResultE( VkResult sResult, char const* spMsg );
void                                  VK_CheckResultE( VkResult sResult );

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

void                                  VK_memcpy( VkDeviceMemory sBufferMemory, VkDeviceSize sSize, const void* spData );
void                                  VK_memread( VkDeviceMemory sBufferMemory, VkDeviceSize sSize, void* spData );

void                                  VK_Reset( ERenderResetFlags sFlags = ERenderResetFlags_None );

bool                                  VK_UseMSAA();
VkSampleCountFlagBits                 VK_GetMSAASamples();

// --------------------------------------------------------------------------------------
// Vulkan Instance

bool                                  VK_CreateInstance();
void                                  VK_DestroyInstance();

void                                  VK_CreateSurface( void* spWindow );

void                                  VK_SetupPhysicalDevice();
void                                  VK_CreateDevice();

VkInstance                            VK_GetInstance();
VkSurfaceKHR                          VK_GetSurface();

VkDevice                              VK_GetDevice();
VkPhysicalDevice                      VK_GetPhysicalDevice();

VkQueue                               VK_GetGraphicsQueue();
VkQueue                               VK_GetPresentQueue();

bool                                  VK_CheckValidationLayerSupport();
VkSampleCountFlags                    VK_GetMaxMSAASamples();
VkSampleCountFlagBits                 VK_FindMaxMSAASamples();

uint32_t                              VK_GetMemoryType( uint32_t sTypeFilter, VkMemoryPropertyFlags sProperties );
void                                  VK_FindQueueFamilies( VkPhysicalDevice sDevice, u32* spGraphics, u32* spPresent );
bool                                  VK_ValidQueueFamilies( u32& srPresent, u32& srGraphics );

void                                  VK_UpdateSwapchainInfo();
VkSurfaceCapabilitiesKHR              VK_GetSwapCapabilities();
VkSurfaceFormatKHR                    VK_ChooseSwapSurfaceFormat();
VkPresentModeKHR                      VK_ChooseSwapPresentMode();
VkExtent2D                            VK_ChooseSwapExtent();

const VkPhysicalDeviceProperties&     VK_GetPhysicalDeviceProperties();
const VkPhysicalDeviceLimits&         VK_GetPhysicalDeviceLimits();

// --------------------------------------------------------------------------------------
// Swapchain

void                                  VK_CreateSwapchain( VkSwapchainKHR spOldSwapchain = nullptr );
void                                  VK_DestroySwapchain();
void                                  VK_RecreateSwapchain();

u32                                   VK_GetSwapImageCount();
// const std::vector< TextureVK* >&      VK_GetSwapImages();
const std::vector< VkImage >&         VK_GetSwapImages();
const std::vector< VkImageView >&     VK_GetSwapImageViews();
const VkExtent2D&                     VK_GetSwapExtent();
VkSurfaceFormatKHR                    VK_GetSwapSurfaceFormat();
VkFormat                              VK_GetSwapFormat();
VkColorSpaceKHR                       VK_GetSwapColorSpace();
VkSwapchainKHR                        VK_GetSwapchain();
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

// void                                  VK_AddUniformBuffer( BufferVK* spTexture );
// void                                  VK_WriteUniformBuffer( BufferVK* spTexture );
// void                                  VK_RemoveUniformBuffer( BufferVK* spTexture );
// void                                  VK_UpdateUniformBuffers();

void                                  VK_AddImageStorage( TextureVK* spTexture );
void                                  VK_RemoveImageStorage( TextureVK* spTexture );
void                                  VK_UpdateImageStorage();

// --------------------------------------------------------------------------------------
// Command Pool

void                                  VK_CreateCommandPool( VkCommandPool& srPool, VkCommandPoolCreateFlags sFlags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT );
void                                  VK_DestroyCommandPool( VkCommandPool& srPool );
void                                  VK_ResetCommandPool( VkCommandPool& srPool, VkCommandPoolResetFlags sFlags = 0 );

VkCommandPool&                        VK_GetSingleTimeCommandPool();
VkCommandPool&                        VK_GetPrimaryCommandPool();

// --------------------------------------------------------------------------------------
// Render Pass

VkRenderPass                          VK_CreateMainRenderPass();
void                                  VK_DestroyMainRenderPass();

Handle                                VK_CreateRenderPass( const RenderPassCreate_t& srCreate );
void                                  VK_DestroyRenderPass( Handle shHandle );

void                                  VK_DestroyRenderPasses();
VkRenderPass                          VK_GetRenderPass();
VkRenderPass                          VK_GetRenderPass( Handle shHandle );
RenderPassInfoVK*                     VK_GetRenderPassInfo( VkRenderPass renderPass );

// --------------------------------------------------------------------------------------
// Present

VkCommandBuffer                       VK_GetCommandBuffer();
VkCommandBuffer                       VK_GetCommandBuffer( Handle cmd );
u32                                   VK_GetCommandIndex();

void                                  VK_CreateFences();
void                                  VK_DestroyFences();

void                                  VK_CreateSemaphores();
void                                  VK_DestroySemaphores();

void                                  VK_AllocateCommands();
void                                  VK_FreeCommands();

VkCommandBuffer                       VK_BeginOneTimeCommand();
void                                  VK_EndOneTimeCommand( VkCommandBuffer c );
void                                  VK_OneTimeCommand( std::function< void( VkCommandBuffer ) > sFunc );

void                                  VK_WaitForPresentQueue();
void                                  VK_WaitForGraphicsQueue();

void                                  VK_RecordCommands();
void                                  VK_Present();

void                                  VK_CheckFenceStatus();

// --------------------------------------------------------------------------------------
// Shader System

VkShaderModule                        VK_CreateShaderModule( u32* spByteCode, u32 sSize );
void                                  VK_DestroyShaderModule( VkShaderModule shaderModule );

bool                                  VK_CreatePipelineLayout( Handle& sHandle, PipelineLayoutCreate_t& srPipelineCreate );
bool                                  VK_CreateGraphicsPipeline( Handle& sHandle, GraphicsPipelineCreate_t& srGraphicsCreate );

void                                  VK_DestroyPipeline( Handle sPipeline );
void                                  VK_DestroyPipelineLayout( Handle sPipeline );

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
bool                                  VK_LoadTexture( ChHandle_t& srHandle, TextureVK* spTexture, const std::string& srPath, const TextureCreateData_t& srCreateData );
TextureVK*                            VK_CreateTexture( ChHandle_t& srHandle, const TextureCreateInfo_t& srTextureCreateInfo, const TextureCreateData_t& srCreateData );
void                                  VK_DestroyTexture( ChHandle_t sTexture );
void                                  VK_DestroyAllTextures();
TextureVK*                            VK_GetTexture( ChHandle_t sTexture );
TextureVK*                            VK_GetTextureNoMissing( ChHandle_t sTexture );  // Same as above, but does not return the missing texture as a fallback

Handle                                VK_CreateFramebuffer( VkRenderPass sRenderPass, u16 sWidth, u16 sHeight, const VkImageView* spAttachments, u32 sCount );
Handle                                VK_CreateFramebuffer( const VkFramebufferCreateInfo& sCreateInfo );
Handle                                VK_CreateFramebuffer( const CreateFramebuffer_t& srCreate );
void                                  VK_DestroyFramebuffer( Handle shHandle );
VkFramebuffer                         VK_GetFramebuffer( Handle shHandle );
glm::uvec2                            VK_GetFramebufferSize( Handle shHandle );
Handle                                VK_GetFramebufferHandle( VkFramebuffer sFrameBuffer );

RenderTarget*                         VK_CreateRenderTarget( const std::vector< TextureVK* >& srImages, u16 sWidth, u16 sHeight, const std::vector< VkImageView >& srSwapImages = {} );
void                                  VK_DestroyRenderTarget( RenderTarget* spTarget );
void                                  VK_DestroyRenderTargets();

RenderTarget*                         VK_GetBackBuffer();
RenderTarget*                         VK_GetBackBufferHandles();

void                                  VK_SetImageLayout( VkImage sImage, VkImageLayout sOldLayout, VkImageLayout sNewLayout, VkImageSubresourceRange& sSubresourceRange );
void                                  VK_SetImageLayout( VkImage sImage, VkImageLayout sOldLayout, VkImageLayout sNewLayout, u32 sMipLevels );

void                                  VK_CreateMissingTexture();

// void                                  VK_AllocateAndBindTexture( TextureVK* spTexture );
void                                  VK_CreateImage( VkImageCreateInfo& srCreateInfo, TextureVK* spTexture );

// --------------------------------------------------------------------------------------
// KTX Texture Support

bool                                  KTX_LoadTexture( TextureVK* spTexture, const char* spPath );


