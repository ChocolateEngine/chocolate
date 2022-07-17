#pragma once

#include <unordered_set>


// ---------------------------------------------------------------------
// Render Graph Ideas
// ---------------------------------------------------------------------


class RenderGraph;
class RenderGraphPass;

struct DeviceCommand;
struct CommandBufferHelper;


// uhh
enum TextureSizing
{
	Absolute,
	SwapchainRelative,
	InputRelative
};


enum RenderGraphQueueFlagBits;


// Images (maybe call this TextureVk)
struct AttachmentInfo
{
	TextureSizing aTextureSize = TextureSizing::SwapchainRelative;
	float aSizeX = 1.0f;
	float aSizeY = 1.0f;
	VkFormat aFormat = VK_FORMAT_UNDEFINED;
	std::string aSizeRelativeName;
	u32 aSamples = 1;
	u32 aLevels = 1;
	u32 aLayers = 1;
	bool aPersistent = true;
};


// Buffers
struct BufferInfo
{
	VkDeviceSize aSize = 0;
	VkBufferUsageFlags aUsage = 0;
	bool aPersistent = true;
};


using HRenderResource        = Handle;
using HRenderResourceBuffer  = Handle;
using HRenderResourceTexture = Handle;


enum class ERenderResource
{
	Generic,
	Buffer,
	Texture,
	Count
};


struct RenderResourceSmaller
{
	// what passes is this resource written/read in?
	std::unordered_set< u16 > aWrittenInPasses;
	std::unordered_set< u16 > aReadInPasses;
};


// THIS BELOW WILL BE SUBJECT TO CHANGE !!!!!!!!
// need to get rid of this inheritance
struct RenderResource
{
	enum class Type
	{
		Buffer,
		Texture,
		Proxy
	};

	RenderResource( Type type, u32 index ) :
		aType( type ), aIndex( index )
	{}

	// blech
	virtual ~RenderResource() = default;

	// --------------------------------------------------

	// what is this resource type?
	Type aType;

	// resource index (kinda weird...)
	u32 aIndex;

	// what (0 is unused)
	u32 aPhysicalIndex = 0;

	// what passes is this resource written/read in?
	std::unordered_set< u16 > aWrittenInPasses;
	std::unordered_set< u16 > aReadInPasses;

	// queues this resource will be used in
	RenderGraphQueueFlags aUsedQueues = RenderGraphQueue_None;

	// name (really should not be used here
	std::string aName;
};


struct RenderBufferResource : public RenderResource
{
	explicit RenderBufferResource( u32 index )
		: RenderResource( RenderResource::Type::Buffer, index )
	{}

	BufferInfo aInfo;
	VkBufferUsageFlags aBufferUsage = 0;
};


struct RenderTextureResource : public RenderResource
{
	explicit RenderTextureResource( u32 index )
		: RenderResource( RenderResource::Type::Texture, index )
	{}

	AttachmentInfo aInfo;
	VkImageUsageFlags aImageUsage = 0;
	bool aTransient = false;
};


struct RenderPassInfo
{
};


class RenderGraphPass
{
public:
	struct AccessedResource
	{
		u32                  aIndex;
		VkPipelineStageFlags aStages = 0;
		VkAccessFlags        aAccess = 0;
		VkImageLayout        aLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	};

	// uhhh
	RenderGraph*             apGraph;
	u32                      aIndex;
	RenderGraphQueueFlagBits aQueue;

	// maybe change to store a list of indices for each resource, or handles even
	std::vector< RenderTextureResource* > aColorInputs;
	std::vector< RenderTextureResource* > aColorOutputs;

	std::vector< AccessedResource > aBufferResources;
	std::vector< AccessedResource > aTextureResources;

	RenderTextureResource* aDepthStencilInput = nullptr;
	RenderTextureResource* aDepthStencilOutput = nullptr;

	// ------------------------------------------------------------------

	RenderGraphPass( RenderGraph* sGraph, u32 sIndex, RenderGraphQueueFlagBits sQueue ) :
		apGraph( sGraph ), aIndex( sIndex ), aQueue( sQueue )
	{}

	// RenderTextureResource& AddAttachmentInput( const AttachmentInfo& srInfo );
	// void AddSubpass( const std::vector<std::string>& srInputs, const std::vector<std::string>& srOutputs );
	// void AddSubpass( const std::vector<std::string>& srInputs, const std::vector<std::string>& srOutputs, const std::vector<std::string>& srResolves );

	RenderTextureResource& AddColorInput( const std::string& srName, const std::string& srAttachment );
	RenderTextureResource& AddDepthInput( const std::string& srName, const std::string& srAttachment );

	RenderTextureResource& AddColorOutput( const std::string& srName, const AttachmentInfo& srInfo );
	RenderTextureResource& AddDepthOutput( const std::string& srName, const AttachmentInfo& srInfo );

	RenderTextureResource& AddTextureInput( const std::string& srName );

	// RenderBufferResource& AddStorageOutput( const std::string& srName );  // sBufferInfo, another name

	RenderBufferResource& AddBufferInput( const std::string& srName, VkPipelineStageFlags sStages, VkAccessFlags sAccess, VkBufferUsageFlags sUsage );
	RenderBufferResource& AddVertexBufferInput( const std::string& srName );
	RenderBufferResource& AddIndexBufferInput( const std::string& srName );

	// RenderBufferResource&  AddIndirectBufferInput( const std::string& srName );

	void SetBuildRenderPass();
	void SetGetClearDepthStencil();
	void SetGetClearColor();

	// ------------------------------------------------------------------

	std::string aName;

	std::unordered_map< RenderBufferResource*, u32 >  aBufferIndex;
	std::unordered_map< RenderTextureResource*, u32 > aTextureIndex;
};


class RenderGraph
{
public:
	HRenderPass CreateRenderPass( const std::string& srName, RenderGraphQueueFlagBits sStage );
	RenderGraphPass* GetRenderPass( HRenderPass handle );

	void SetBackbufferSource( const std::string& srName );

	RenderBufferResource& GetBufferResource( const std::string& srName );
	RenderTextureResource& GetTextureResource( const std::string& srName );

	bool Bake();

private:
	// ensure the data in each renderpass makes sense
	bool ValidatePasses();

	// flatten down into an array of render passes
	// void FlattenPasses();

	// void ReorderPasses();

	std::unordered_map< std::string, HRenderPass > aPassToIndex;
	std::unordered_map< std::string, u32 >         aResourceToIndex;

	std::vector< RenderResource* > aResources;
	// std::vector< RenderGraphPass > aPasses;

	ResourceManager< RenderGraphPass > aPasses;
};


// ---------------------------------------------------------------------
// Command Type
// ---------------------------------------------------------------------


enum class CommandType
{
	Present,
	Clear,
	ExecuteCommands,

	BeginRenderPass,
	EndRenderPass,
	SetViewport,
	SetScissor,

	BindPipeline,
	BindVertexBuffer,
	BindIndexBuffer,
	// BindConstantBuffer,

	BindDescriptorSet,
	BindDescriptorSetLayout,
	PushConstants,

	Draw,
	DrawIndexed,
};


// ---------------------------------------------------------------------
// Command Structs
// ---------------------------------------------------------------------


struct CmdExecuteCommands_t
{
	std::vector< VkCommandBuffer > aCommands;
};


struct CmdBeginRenderPass_t
{
	VkRenderPassBeginInfo  aInfo;
	VkSubpassContents      aContents;
};


struct CmdBindPipeline_t
{
	VkPipelineBindPoint aPipelineBindPoint; // could also be aStage;
	VkPipeline          aPipeline;
};


struct CmdBindDescriptorSet_t
{
	VkPipelineBindPoint aPipelineBindPoint;
	u32                 aFirstSet;
	
	std::vector< VkDescriptorSet > aDescriptorSets;
	std::vector< u32 >             aDynamicOffsets;
};


struct CmdPushConstants_t
{
	u32                 aStageFlags;
	u32                 aOffset;
	u32                 aSize;
	void*               apData;
};


struct CmdDraw_t
{
	u32 aVertexCount;
	u32 aInstanceCount;
	u32 aFirstVertex;
	u32 aFirstInstance;
};


struct CmdDrawIndexed_t
{
	u32 aIndexCount;
	u32 aInstanceCount;
	u32 aFirstIndex;
	u32 aVertexOffset;
	u32 aFirstInstance;
};


// ---------------------------------------------------------------------


struct DeviceCommand
{
	CommandType aType;

	union
	{
		CmdExecuteCommands_t*       apExecuteCommands;
		CmdBindPipeline_t*          apBindPipeline;
		CmdBeginRenderPass_t*       apRenderPass;
		CmdBindVertexBuffer_t*      apVertexBuffer;
		CmdBindIndexBuffer_t*       apIndexBuffer;
		CmdBindDescriptorSet_t*     apDescSet;
		CmdPushConstants_t*         apPushConstants;
		CmdDraw_t*                  apDraw;
		CmdDrawIndexed_t*           apDrawIndexed;
	};
};


class CommandBufferHelper
{
public:
	std::vector< DeviceCommand > aCommands;

	VkPipelineLayout aCurLayout;

	void RunCommand( VkCommandBuffer c, DeviceCommand& command );
	void ExecuteCommands( VkCommandBuffer c );

	void AddPushConstants( void* spData, u32 sOffset, u32 sSize, u32 sStageFlags );
};


#if 0
struct RenderList
{
	RenderGraphPass* apPass;
	
	std::vector< DeviceCommand > aCommands;

	void AddModel( Model_t* spModel, const glm::mat4& sModelMatrix );
	void AddViewportCamera( const ViewportCamera_t& sViewportCamera );

	void BeginRenderPass( VkCommandBuffer cmd, u32 cmdIndex );
	void EndRenderPass( VkCommandBuffer cmd );
	
	// void Build( CommandBufferHelper cmd );
	void Execute( VkCommandBuffer cmd );
	// void End( VkCommandBuffer cmd );
};
#endif

