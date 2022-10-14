#pragma once

#include <vulkan/vulkan.hpp>


LOG_CHANNEL2( Render );
LOG_CHANNEL( Vulkan );
LOG_CHANNEL( Validation );


extern int             gWidth;
extern int             gHeight;

extern VkFormat        gColorFormat;
extern VkColorSpaceKHR gColorSpace;

enum class GraphicsFmt;
using ShaderStage = int;

struct PipelineLayoutCreate_t;
struct GraphicsPipelineCreate_t;


#if _DEBUG
extern PFN_vkDebugMarkerSetObjectTagEXT  pfnDebugMarkerSetObjectTag;
extern PFN_vkDebugMarkerSetObjectNameEXT pfnDebugMarkerSetObjectName;
extern PFN_vkCmdDebugMarkerBeginEXT      pfnCmdDebugMarkerBegin;
extern PFN_vkCmdDebugMarkerEndEXT        pfnCmdDebugMarkerEnd;
extern PFN_vkCmdDebugMarkerInsertEXT     pfnCmdDebugMarkerInsert;
#endif


struct BufferVK
{
	VkBuffer       aBuffer;
	VkDeviceMemory aMemory;
	size_t         aSize;
};


struct TextureVK
{
	// Vulkan Info
	VkImage         aImage;
	VkImageView     aImageView;
	VkDeviceMemory  aMemory;
	VkImageViewType aViewType;
	VkFormat        aFormat = VK_FORMAT_UNDEFINED;

	// Texture Information
	int             aIndex;
	glm::ivec2      aSize;
	u8              aMipLevels    = 0;
	int             aFrames       = 0;
	bool            aRenderTarget = false;
};


struct RenderTarget
{
	std::vector< TextureVK* >    aImages;
	std::vector< VkFramebuffer > aFrameBuffers;
};


// --------------------------------------------------------------------------------------
// General

char const*                           VKString( VkResult sResult );

VkFormat                              VK_ToVkFormat( GraphicsFmt colorFmt );
VkShaderStageFlags                    VK_ToVkShaderStage( ShaderStage stage );

void                                  VK_CheckResult( VkResult sResult, char const* spMsg );
void                                  VK_CheckResult( VkResult sResult );

void                                  VK_memcpy( VkDeviceMemory sBufferMemory, VkDeviceSize sSize, const void* spData );

void                                  VK_Reset();

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
VkSampleCountFlagBits                 VK_FindMaxMSAASamples();

uint32_t                              VK_GetMemoryType( uint32_t sTypeFilter, VkMemoryPropertyFlags sProperties );
void                                  VK_FindQueueFamilies( VkPhysicalDevice sDevice, u32* spGraphics, u32* spPresent );
bool                                  VK_ValidQueueFamilies( u32& srPresent, u32& srGraphics );

void                                  VK_UpdateSwapchainInfo();
VkSurfaceCapabilitiesKHR              VK_GetSwapCapabilities();
VkSurfaceFormatKHR                    VK_ChooseSwapSurfaceFormat();
VkPresentModeKHR                      VK_ChooseSwapPresentMode();
VkExtent2D                            VK_ChooseSwapExtent();

// --------------------------------------------------------------------------------------
// Swapchain

void                                  VK_CreateSwapchain( VkSwapchainKHR spOldSwapchain = nullptr );
void                                  VK_DestroySwapchain();
void                                  VK_RecreateSwapchain();

u32                                   VK_GetSwapImageCount();
const std::vector< VkImage >&         VK_GetSwapImages();
const std::vector< VkImageView >&     VK_GetSwapImageViews();
const VkExtent2D&                     VK_GetSwapExtent();
VkSurfaceFormatKHR                    VK_GetSwapSurfaceFormat();
VkFormat                              VK_GetSwapFormat();
VkColorSpaceKHR                       VK_GetSwapColorSpace();
VkSwapchainKHR                        VK_GetSwapchain();

// --------------------------------------------------------------------------------------
// Descriptor Pool

void                                  VK_CreateDescSets();
void                                  VK_DestroyDescSets();

VkDescriptorPool                      VK_GetDescPool();
// VkDescriptorSetLayout                 VK_GetDescImageSet();
// VkDescriptorSetLayout                 VK_GetDescBufferSet();

VkDescriptorSetLayout                 VK_GetImageLayout();
VkDescriptorSetLayout                 VK_GetImageStorageLayout();

const std::vector< VkDescriptorSet >& VK_GetImageSets();
const std::vector< VkDescriptorSet >& VK_GetImageStorage();
VkDescriptorSet                       VK_GetImageSet( size_t sIndex );
void                                  VK_UpdateImageSets();

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

void                                  VK_DestroyRenderPasses();
VkRenderPass                          VK_GetRenderPass();
VkRenderPass                          VK_GetRenderPass( Handle shHandle );

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

VkCommandBuffer                       VK_BeginSingleCommand();
void                                  VK_EndSingleCommand();
void                                  VK_SingleCommand( std::function< void( VkCommandBuffer ) > sFunc );

void                                  VK_WaitForPresentQueue();
void                                  VK_WaitForGraphicsQueue();

void                                  VK_RecordCommands();
void                                  VK_Present();

// --------------------------------------------------------------------------------------
// Shader System

VkShaderModule                        VK_CreateShaderModule( u32* spByteCode, u32 sSize );
void                                  VK_DestroyShaderModule( VkShaderModule shaderModule );

Handle                                VK_CreatePipelineLayout( PipelineLayoutCreate_t& srPipelineCreate );
Handle                                VK_CreateGraphicsPipeline( GraphicsPipelineCreate_t& srGraphicsCreate );

void                                  VK_BindShader( VkCommandBuffer c, Handle handle );
VkPipelineLayout                      VK_GetPipelineLayout( Handle handle );

// --------------------------------------------------------------------------------------
// Buffers

void                                  VK_CreateBuffer( VkBuffer& srBuffer, VkDeviceMemory& srBufferMem, u32 sBufferSize, VkBufferUsageFlags sUsage, VkMemoryPropertyFlags sMemBits );
void                                  VK_DestroyBuffer( VkBuffer& srBuffer, VkDeviceMemory& srBufferMem );

// --------------------------------------------------------------------------------------
// Textures and Render Targets

VkSampler                             VK_GetSampler();

TextureVK*                            VK_NewTexture();
TextureVK*                            VK_LoadTexture( const std::string& srPath );
TextureVK*                            VK_CreateTexture( const glm::ivec2& srSize, VkFormat sFormat );
void                                  VK_DestroyTexture( TextureVK* spTexture );
void                                  VK_DestroyAllTextures();
TextureVK*                            VK_GetTexture( Handle texture );

RenderTarget*                         VK_CreateRenderTarget( const std::vector< TextureVK* >& srImages, u16 sWidth, u16 sHeight, const std::vector< VkImageView >& srSwapImages = {} );
void                                  VK_DestroyRenderTarget( RenderTarget* spTarget );
void                                  VK_DestroyRenderTargets();
void                                  VK_RebuildRenderTargets();

Handle                                VK_GetFrameBufferHandle( VkFramebuffer sFrameBuffer );
VkFramebuffer                         VK_GetFrameBuffer( Handle shFrameBuffer );

RenderTarget*                         VK_GetBackBuffer();

void                                  VK_SetImageLayout( VkImage sImage, VkImageLayout sOldLayout, VkImageLayout sNewLayout, VkImageSubresourceRange& sSubresourceRange );
void                                  VK_SetImageLayout( VkImage sImage, VkImageLayout sOldLayout, VkImageLayout sNewLayout, u32 sMipLevels );

// --------------------------------------------------------------------------------------
// KTX Texture Support

TextureVK*                            KTX_LoadTexture( const std::string srPath );


