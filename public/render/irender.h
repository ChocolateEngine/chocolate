#pragma once

#include "system.h"
#include <stdio.h>

#include <glm/vec3.hpp>
#include <glm/mat4x4.hpp>

using Handle = size_t;

struct SDL_Window;
struct ImDrawData;


// -----------------------------------------------------------------------------
// Data Types
// -----------------------------------------------------------------------------


enum class GraphicsFmt
{
	INVALID,

	// -------------------------
	// 8 bit depth

	R8_SRGB,
	R8_SINT,
	R8_UINT,

	RG88_SRGB,
	RG88_SINT,
	RG88_UINT,

	RGB888_SRGB,
	RGB888_SINT,
	RGB888_UINT,

	RGBA8888_SRGB,
	RGBA8888_SINT,
	RGBA8888_UINT,

	RGBA8888_UNORM,

	// -------------------------
	
	BGRA8888_SRGB,
	BGRA8888_SNORM,
	BGRA8888_UNORM,

	// -------------------------
	// 16 bit depth

	R16_SFLOAT,
	R16_SINT,
	R16_UINT,

	RG1616_SFLOAT,
	RG1616_SINT,
	RG1616_UINT,

	RGB161616_SFLOAT,
	RGB161616_SINT,
	RGB161616_UINT,

	RGBA16161616_SFLOAT,
	RGBA16161616_SINT,
	RGBA16161616_UINT,

	// -------------------------
	// 32 bit depth

	R32_SFLOAT,
	R32_SINT,
	R32_UINT,

	RG3232_SFLOAT,
	RG3232_SINT,
	RG3232_UINT,

	RGB323232_SFLOAT,
	RGB323232_SINT,
	RGB323232_UINT,

	RGBA32323232_SFLOAT,
	RGBA32323232_SINT,
	RGBA32323232_UINT,

	// -------------------------
	// GPU Compressed Formats

	BC1_RGB_UNORM_BLOCK,
	BC1_RGB_SRGB_BLOCK,
	BC1_RGBA_UNORM_BLOCK,
	BC1_RGBA_SRGB_BLOCK,

	BC2_UNORM_BLOCK,
	BC2_SRGB_BLOCK,

	BC3_UNORM_BLOCK,
	BC3_SRGB_BLOCK,

	BC4_UNORM_BLOCK,
	BC4_SNORM_BLOCK,

	BC5_UNORM_BLOCK,
	BC5_SNORM_BLOCK,

	BC6H_UFLOAT_BLOCK,
	BC6H_SFLOAT_BLOCK,

	BC7_UNORM_BLOCK,
	BC7_SRGB_BLOCK,

	// -------------------------
	// Other Formats

	D16_UNORM,
	D32_SFLOAT,
};


enum TextureType
{
	TextureType_1D,
	TextureType_2D,
	TextureType_3D,
	TextureType_Cube,
	TextureType_1DArray,
	TextureType_2DArray,
	TextureType_CubeArray,
	TextureType_Max,
};


enum TextureSamples : unsigned char
{
	TextureSamples_1,
	TextureSamples_2,
	TextureSamples_4,
	TextureSamples_8,
	TextureSamples_16,
	TextureSamples_32,
	TextureSamples_64,
	TextureSamples_Max,
};


using EImageUsage = unsigned short;
enum : EImageUsage
{
	EImageUsage_None               = 0,
	EImageUsage_TransferSrc        = ( 1 << 0 ),
	EImageUsage_TransferDst        = ( 1 << 1 ),
	EImageUsage_Sampled            = ( 1 << 2 ),
	EImageUsage_Storage            = ( 1 << 3 ),
	EImageUsage_AttachColor        = ( 1 << 4 ),
	EImageUsage_AttachDepthStencil = ( 1 << 5 ),
	EImageUsage_AttachTransient    = ( 1 << 6 ),
	EImageUsage_AttachInput        = ( 1 << 7 ),
};


enum EImageFilter : char
{
	EImageFilter_Nearest,
	EImageFilter_Linear,
	// EImageFilter_Cubic,
};


using EDynamicState = unsigned short;
enum : EDynamicState
{
	EDynamicState_None               = 0,
	EDynamicState_Viewport           = ( 1 << 0 ),
	EDynamicState_Scissor            = ( 1 << 1 ),
	EDynamicState_LineWidth          = ( 1 << 2 ),
	EDynamicState_DepthBias          = ( 1 << 3 ),
	EDynamicState_DepthBounds        = ( 1 << 4 ),
	EDynamicState_BlendConstants     = ( 1 << 5 ),
	EDynamicState_StencilCompareMask = ( 1 << 6 ),
	EDynamicState_StencilWriteMask   = ( 1 << 7 ),
	EDynamicState_StencilReference   = ( 1 << 8 ),
};


using ShaderStage = char;
enum : ShaderStage
{
	ShaderStage_None     = 0,
	ShaderStage_Vertex   = ( 1 << 0 ),
	ShaderStage_Fragment = ( 1 << 1 ),
	ShaderStage_Compute  = ( 1 << 2 ),
};


enum EPrimTopology
{
	EPrimTopology_Point,
	EPrimTopology_Line,
	EPrimTopology_Tri
};


enum ECullMode
{
	ECullMode_None,
	ECullMode_Front,
	ECullMode_Back
};


enum EIndexType : char
{
	EIndexType_U16,
	EIndexType_U32,
};


using EBufferFlags = int;
enum : EBufferFlags
{
	EBufferFlags_None        = 0,
	EBufferFlags_TransferSrc = ( 1 << 0 ),
	EBufferFlags_TransferDst = ( 1 << 1 ),
	EBufferFlags_Uniform     = ( 1 << 2 ),
	EBufferFlags_Index       = ( 1 << 3 ),
	EBufferFlags_Vertex      = ( 1 << 4 ),
	EBufferFlags_Indirect    = ( 1 << 5 ),
};


using EBufferMemory = char;
enum : EBufferMemory
{
	EBufferMemory_None      = 0,
	EBufferMemory_Device = ( 1 << 0 ),
	EBufferMemory_Host   = ( 1 << 1 ),
};


enum EPipelineBindPoint
{
	EPipelineBindPoint_Graphics,
	EPipelineBindPoint_Compute,
	// EPipelineBindPoint_RayTracing,  // one day
};


enum ECommandBufferLevel
{
	ECommandBufferLevel_Primary,
	ECommandBufferLevel_Secondary,
};


enum EImageView
{
	EImageView_1D,
	EImageView_2D,
	EImageView_3D,
	EImageView_Cube,
	EImageView_1D_Array,
	EImageView_2D_Array,
	EImageView_CubeArray
};


enum EAttachmentType
{
	EAttachmentType_Color,
	// EAttachmentType_Input,
	EAttachmentType_Resolve,
	// EAttachmentType_Preserve,
	EAttachmentType_Depth,
};


enum EBlendState
{
	EBlendState_None,
	EBlendState_Additive
};


enum EAttachmentLoadOp
{
	EAttachmentLoadOp_Load,
	EAttachmentLoadOp_Clear,
	EAttachmentLoadOp_DontCare
};


using ERenderResetFlags = int;
enum : ERenderResetFlags
{
	ERenderResetFlags_None = 0,
	ERenderResetFlags_MSAA = ( 1 << 0 ),  // MSAA was toggled, Recreate RenderPass and shaders
};


enum EDescriptorType : char
{
	EDescriptorType_Sampler,
	EDescriptorType_CombinedImageSampler,
	EDescriptorType_SampledImage,
	EDescriptorType_StorageImage,
	EDescriptorType_UniformTexelBuffer,
	EDescriptorType_StorageTexelBuffer,
	EDescriptorType_UniformBuffer,
	EDescriptorType_StorageBuffer,
	EDescriptorType_UniformBufferDynamic,
	EDescriptorType_StorageBufferDynamic,
	EDescriptorType_InputAttachment,
	EDescriptorType_Max,
};

// -----------------------------------------------------------------------------
// Structs
// -----------------------------------------------------------------------------


struct TextureCreateInfo_t
{
	const char* apName    = nullptr;  // Only Used for debugging
	void*       apData    = nullptr;
	u32         aDataSize = 0;
	glm::uvec2  aSize;
	GraphicsFmt aFormat;
	EImageView  aViewType;
};


// used for both loading textures and creating textures
struct TextureCreateData_t
{
	bool         aUseMSAA = false;
	EImageUsage  aUsage   = EImageUsage_None;
	EImageFilter aFilter  = EImageFilter_Linear;
};



struct Viewport_t
{
	float x;
	float y;
	float width;
	float height;
	float minDepth;
	float maxDepth;
};


struct Rect2D_t
{
	glm::ivec2 aOffset;
	glm::uvec2 aExtent;
};


struct CreateRenderTarget_t
{
	glm::uvec2  aSize;
	GraphicsFmt aFormat;
	bool        aUseMSAA;
	bool        aDepth;
};


struct FramebufferPass_t
{
	std::vector< Handle > aAttachColors;
	// std::vector< Handle > aAttachInput;
	std::vector< Handle > aAttachResolve;
	// std::vector< Handle > aAttachPreserve;
	Handle                aAttachDepth = 0;
};


struct CreateFramebuffer_t
{
	Handle           aRenderPass = InvalidHandle;
	glm::uvec2       aSize{};
	// const Handle*    aTextures = nullptr;
	// u8               aTextureCount = 0;

	FramebufferPass_t aPass{};
};


struct VertexInputBinding_t
{
	u32  aBinding;
	u32  aStride;
	bool aIsInstanced;
};


struct VertexInputAttribute_t
{
	u32         aLocation;
	u32         aBinding;
	GraphicsFmt aFormat;
	u32         aOfset;
};


struct ShaderModule_t
{
	ShaderStage aStage;
	const char* aModulePath;
	const char* apEntry;
};


struct PushConstantRange_t
{
	ShaderStage aShaderStages;
	u32         aOffset;
	u32         aSize;
};


struct ColorBlendAttachment_t
{
	bool aBlendEnable = true;
};


struct PipelineLayoutCreate_t
{
	std::vector< Handle >              aLayouts;
	std::vector< PushConstantRange_t > aPushConstants;
};


struct GraphicsPipelineCreate_t
{
	const char*                           apName = nullptr;

	std::vector< ShaderModule_t >         aShaderModules;
	std::vector< VertexInputBinding_t >   aVertexBindings;
	std::vector< VertexInputAttribute_t > aVertexAttributes;
	std::vector< ColorBlendAttachment_t > aColorBlendAttachments;

	EPrimTopology                         aPrimTopology;
	EDynamicState                         aDynamicState;
	ECullMode                             aCullMode;

	Handle                                aPipelineLayout;
	Handle                                aRenderPass;
};


// potential idea for the future for adding a software renderer
using GraphicsSupportFlags = u32;
enum : GraphicsSupportFlags
{
	GraphicsSupport_None = 0,
	// GraphicsSupport_Shaders = (1 << 0),
	// GraphicsSupport_Buffers = (1 << 1),
	// GraphicsSupport_Textures = (1 << 2),
	// GraphicsSupport_RenderTargets = (1 << 3),
	// GraphicsSupport_Viewports = (1 << 4),
	// GraphicsSupport_ViewportsCamera = (1 << 5),
	// GraphicsSupport_RenderGraph = (1 << 6),
	// GraphicsSupport_All = 0xFFFFFFFF,
};


// use this function to check:
// bool ret = graphics->Supports( GraphicsSupport_Something | GraphicsSupport_Something2 );


struct RenderPassAttachment_t
{
	GraphicsFmt       aFormat;
	bool              aUseMSAA;
	EAttachmentType   aType;
	EAttachmentLoadOp aLoadOp;
};


struct RenderPassSubpass_t
{
	EPipelineBindPoint        aBindPoint;
};


// TODO: add subpass dependencies?
struct RenderPassCreate_t
{
	std::vector< RenderPassAttachment_t > aAttachments;  // TODO: use FramebufferPass_t instead (separate argument to avoid copying to this struct)
	std::vector< RenderPassSubpass_t >    aSubpasses;
};


struct RenderPassClear_t
{
	bool      aIsDepth = false;
	glm::vec4 aColor{};
	float     aDepth   = 1.f;
	u32       aStencil = 0;
};


struct RenderPassBegin_t
{
	Handle    aRenderPass  = InvalidHandle;
	Handle    aFrameBuffer = InvalidHandle;

	std::vector< RenderPassClear_t > aClear;
};


struct CreateVariableDescLayout_t
{
	const char*     apName    = nullptr;
	EDescriptorType aType     = EDescriptorType_Max;
	ShaderStage     aStages   = ShaderStage_None;
	u32             aBinding  = 0;
	u32             aCount    = 0;
};


struct AllocVariableDescLayout_t
{
	const char* apName    = nullptr;
	Handle      aLayout   = InvalidHandle;
	u32         aCount    = 0;
	u32         aSetCount = 0;
};


struct UpdateVariableDescSet_t
{
	std::vector< Handle > aDescSets;
	EDescriptorType       aType;

	std::vector< Handle > aBuffers;
	std::vector< Handle > aImages;
};


#if 0
//resources are used to comunicate to a renderpass what and how a resource is used
//the shader dictates what type of resource must be bound where
//resources may be changed from frame to frame
struct ResourceStorageBuffer_t
{
	Handle aBuffer;
	bool                readOnly;
	uint32_t            binding;
};

struct ResourceUniformBuffer_t
{
	Handle   buffer;
	uint32_t            binding;
};

//if used as a storage image it is considered to be written to, causing additional barriers
struct ResourceImage_t
{
	Handle      aImage;
	uint32_t    aMipLevel;
	uint32_t    aBinding;
};

struct ResourceSampler_t
{
	Handle aSampler;
	u32    aBinding;
};

struct RenderPassResources_t
{
	std::vector< ResourceSampler_t >       aSamplers;
	std::vector< ResourceStorageBuffer_t > aStorageBuffers;
	std::vector< ResourceUniformBuffer_t > aUniformBuffers;
	std::vector< ResourceImage_t >         aSampledImages;
	std::vector< ResourceImage_t >         aStorageImages;

	// maybe?
	std::vector< ResourceVertexBuffer_t >  aVertexBuffers;
	std::vector< ResourceIndexBuffer_t >   aIndexBuffers;
};

//generic info for a renderpass
struct RenderPassExecution
{
	RenderPassHandle    handle;
	RenderPassResources resources;
};

struct RenderTarget
{
	ImageHandle image;
	uint32_t    mipLevel = 0;
};

// Contains RenderPassExecution and additional info for graphic pass
struct GraphicPassExecution
{
	RenderPassExecution         genericInfo;
	std::vector< RenderTarget > targets;
};

// Contains RenderPassExecution and additional info for compute pass
struct ComputePassExecution
{
	RenderPassExecution genericInfo;
	std::vector< char > pushConstants;
	uint32_t            dispatchCount[ 3 ] = { 1, 1, 1 };
};
#endif


// Function callback for game code for when renderer gets reset
typedef void ( *Render_OnReset_t )( ERenderResetFlags sFlags );


class IRender : public BaseSystem
{
  public:
	// --------------------------------------------------------------------------------------------
	// General Purpose Functions
	// --------------------------------------------------------------------------------------------

	virtual bool        InitImGui( Handle shRenderPass )                                                         = 0;
	virtual void        ShutdownImGui()                                                                          = 0;

	virtual SDL_Window* GetWindow()                                                                              = 0;
	virtual void        GetSurfaceSize( int& srWidth, int& srHeight )                                            = 0;

	// --------------------------------------------------------------------------------------------
	// Buffers
	// --------------------------------------------------------------------------------------------

	virtual Handle      CreateBuffer( const char* spName, u32 sSize, EBufferFlags sBufferFlags, EBufferMemory sBufferMem ) = 0;
	virtual Handle      CreateBuffer( u32 sSize, EBufferFlags sBufferFlags, EBufferMemory sBufferMem )           = 0;
	virtual void        MemWriteBuffer( Handle buffer, u32 sSize, void* spData )                                 = 0;
	virtual void        MemCopyBuffer( Handle shSrc, Handle shDst, u32 sSize )                                   = 0;
	virtual void        DestroyBuffer( Handle buffer )                                                           = 0;

	// --------------------------------------------------------------------------------------------
	// Textures
	// --------------------------------------------------------------------------------------------

	virtual Handle      LoadTexture( const std::string& srTexturePath, const TextureCreateData_t& srCreateData ) = 0;
	virtual Handle      CreateTexture( const TextureCreateInfo_t& srTextureCreateInfo, const TextureCreateData_t& srCreateData ) = 0;
	virtual void        FreeTexture( Handle shTexture )                                                          = 0;
	virtual int         GetTextureIndex( Handle shTexture )                                                      = 0;

	// virtual Handle      CreateRenderTarget( const CreateRenderTarget_t& srCreate )                             = 0;
	// virtual void        DestroyRenderTarget( Handle shTarget )                                                 = 0;

	virtual Handle      CreateFramebuffer( const CreateFramebuffer_t& srCreate )                                 = 0;
	virtual void        DestroyFramebuffer( Handle shTarget )                                                    = 0;

	// --------------------------------------------------------------------------------------------
	// Shader System
	// --------------------------------------------------------------------------------------------

	virtual Handle      CreatePipelineLayout( PipelineLayoutCreate_t& srPipelineCreate )                         = 0;
	virtual Handle      CreateGraphicsPipeline( GraphicsPipelineCreate_t& srGraphicsCreate )                     = 0;

	virtual bool        RecreatePipelineLayout( Handle sHandle, PipelineLayoutCreate_t& srPipelineCreate )       = 0;
	virtual bool        RecreateGraphicsPipeline( Handle sHandle, GraphicsPipelineCreate_t& srGraphicsCreate )   = 0;

	virtual void        DestroyPipeline( Handle sPipeline )                                                      = 0;
	virtual void        DestroyPipelineLayout( Handle sPipeline )                                                = 0;

	// HACK
	virtual Handle      GetSamplerLayout()                                                                       = 0;
	virtual void        GetSamplerSets( Handle* spHandles )                                                      = 0;

	virtual Handle      CreateVariableDescLayout( const CreateVariableDescLayout_t& srCreate )                   = 0;
	virtual bool        AllocateVariableDescLayout( const AllocVariableDescLayout_t& srCreate, Handle* handles ) = 0;
	virtual void        UpdateVariableDescSet( const UpdateVariableDescSet_t& srUpdate )                         = 0;

	// --------------------------------------------------------------------------------------------
	// Back Buffer Info
	// --------------------------------------------------------------------------------------------

	virtual GraphicsFmt GetSwapFormatColor()                                                                     = 0;
	virtual GraphicsFmt GetSwapFormatDepth()                                                                     = 0;

	virtual void        GetBackBufferTextures( Handle* spColor, Handle* spDepth, Handle* spResolve )             = 0;
	virtual Handle      GetBackBufferColor()                                                                     = 0;
	virtual Handle      GetBackBufferDepth()                                                                     = 0;

	// --------------------------------------------------------------------------------------------
	// Rendering
	// --------------------------------------------------------------------------------------------

	virtual void        SetResetCallback( Render_OnReset_t sFunc )                                               = 0;

	virtual void        Reset()                                                                                  = 0;
	virtual void        NewFrame()                                                                               = 0;
	virtual void        Present()                                                                                = 0;

	virtual void        WaitForQueues()                                                                          = 0;
	virtual void        ResetCommandPool()                                                                       = 0;

	// blech
	virtual void        GetCommandBufferHandles( std::vector< Handle >& srHandles )                              = 0;

	virtual void        BeginCommandBuffer( Handle cmd )                                                         = 0;
	virtual void        EndCommandBuffer( Handle cmd )                                                           = 0;

	virtual Handle      CreateRenderPass( const RenderPassCreate_t& srCreate )                                   = 0;
	virtual void        DestroyRenderPass( Handle shPass )                                                       = 0;
	virtual void        BeginRenderPass( Handle cmd, const RenderPassBegin_t& srBegin )                          = 0;
	virtual void        EndRenderPass( Handle cmd )                                                              = 0;

	// blech
	virtual void        DrawImGui( ImDrawData* spDrawData, Handle cmd )                                          = 0;

	// --------------------------------------------------------------------------------------------
	// Vulkan Commands
	// --------------------------------------------------------------------------------------------

	virtual void        CmdSetViewport( Handle cmd, u32 sOffset, const Viewport_t* spViewports, u32 sCount )     = 0;

	virtual void        CmdSetScissor( Handle cmd, u32 sOffset, const Rect2D_t* spScissors, u32 sCount )         = 0;

	virtual bool        CmdBindPipeline( Handle cmd, Handle shader ) = 0;

	virtual void        CmdPushConstants( Handle      cmd,
	                                      Handle      shPipelineLayout,
	                                      ShaderStage sShaderStage,
	                                      u32         sOffset,
	                                      u32         sSize,
	                                      void*       spValues ) = 0;

	virtual void        CmdBindDescriptorSets( Handle             cmd,
	                                           size_t             sCmdIndex,
	                                           EPipelineBindPoint sBindPoint,
	                                           Handle             shPipelineLayout,
	                                           Handle*            spSets,
	                                           u32                sSetCount ) = 0;

	virtual void        CmdBindVertexBuffers( Handle        cmd,
	                                          u32           sFirstBinding,
	                                          u32           sCount,
	                                          const Handle* spBuffers,
	                                          const size_t* spOffsets ) = 0;

	virtual void        CmdBindIndexBuffer( Handle     cmd,
	                                        Handle     shBuffer,
	                                        size_t     offset,
	                                        EIndexType indexType ) = 0;

	virtual void        CmdDraw( Handle cmd,
	                             u32    sVertCount,
	                             u32    sInstanceCount,
	                             u32    sFirstVert,
	                             u32    sFirstInstance ) = 0;

	virtual void        CmdDrawIndexed( Handle cmd,
	                                    u32    sIndexCount,
	                                    u32    sInstanceCount,
	                                    u32    sFirstIndex,
	                                    int    sVertOffset,
	                                    u32    sFirstInstance ) = 0;
};
