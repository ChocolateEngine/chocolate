#pragma once

// should probably remove this
#include <vulkan/vulkan.h>

using HShader = Handle;


// 
// Really stupid idea:
// 
// enum for shader data input types
// (ModelMatrix, ProjMatrix, ViewMatrix, etc)
// and then, for each type, a struct with the data
// and a pointer to the data.
// 
// This way, we can have multiple inputs for a single shader
// and we can have multiple shaders with different inputs.
// 
// the game code (or graphics abstraction) will set the data in the render graph
// the shader will be able to request data from wherever it's stored, by using the enum
// the shader will also contain info on where to place that data (in a uniform buffer, push constant, a texture, etc)
// 


enum class EShaderDataType
{
	ModelMatrix,
	ViewMatrix,
	ProjMatrix,
	// CameraPosition,
	// LightPosition,
	// LightColor,
	// LightIntensity,
	// LightRadius,
	// LightFalloff,
	// LightConeAngle,
	// LightConeDirection,
};


// this should eventually be able to be loaded in a single file on disk
struct ShaderCreateInfo_t
{
	// shader module info
	// could change to Stage_t
	struct Module_t
	{
		std::string           aPath;
		std::string           aEntry;
		VkShaderStageFlagBits aStage;
	};

	std::string               aName;
	std::vector< Module_t >   aModules;  // could change to aStages
	
	// Vertex Input and Assembly
	VertexFormat              aVertexFormat;
	VkPrimitiveTopology       aPrimitiveTopology;

	// Viewport State
	std::vector< VkViewport > aViewports;
	std::vector< VkRect2D >   aScissors;

	// Rasterization State
	VkPolygonMode             aPolygonMode;
	VkCullModeFlagBits        aCullMode;
	VkFrontFace               aFrontFace;

	// Multisampling State
	VkSampleCountFlagBits     aRasterizationSamples;
	bool                      aAlphaToCoverageEnable = false;
	bool                      aAlphaToOneEnable = false;
	
	// Depth Stencil
	bool                      aEnableDepthTest = true;
	bool                      aEnableDepthWrite = true;
	VkCompareOp               aDepthCompareOp = VK_COMPARE_OP_LESS;

	// Dynamic States
	std::vector< VkDynamicState > aDynamicStates;

	// Color Blend State
	
	// Color Blend Attachment State

	// Pipeline Layout
	std::vector< VkPushConstantRange > aPushConstantRanges;
	std::vector< VkDescriptorSetLayout > aSetLayouts;
};


// extra information that's part of the shader
struct ShaderPipelineInfo_t
{
	std::string aName;
};


// -----------------------------------------------------------------------
// Resource Bindings


constexpr u8 VK_MAX_PUSH_CONSTANT_SIZE = 128;


// useless? unless i want to abstract this, just use VkBufferUsageFlagBits
enum class EShaderBufferType
{
	Vertex,
	Index,
	Uniform,
	ShaderStorage,
	PushConstant,
};


enum class ShaderBindingType_t
{
	PushConstant,
	UniformBuffer,
	StorageBuffer,
	Texture,
	Sampler,
	SampledImage,
	StorageImage,
	InputAttachment,
	Max
};


struct ResourceBindings_t
{
	std::vector< VkDescriptorSetLayout > aSetLayouts;
	std::vector< VkPushConstantRange >   aPushConstantRanges;

	// u8 aPushConstantData[VK_MAX_PUSH_CONSTANT_SIZE];
	std::vector< u8 > aPushConstantData;
};


// -----------------------------------------------------------------------
// Shader Pipeline Types


enum class ShaderPipelineType
{
	Graphics,
	Compute,
	RayTracing,
	Count
};


struct GraphicsPipeline_t
{
	VkPipeline aPipeline = VK_NULL_HANDLE;
	// if it's a graphics pipeline, it's VK_PIPELINE_BIND_POINT_GRAPHICS, so this is useless
	// VkPipelineBindPoint aPipelineBindPoint;
	VkPipelineLayout aPipelineLayout = VK_NULL_HANDLE;
	
	std::vector< VkDescriptorSetLayout > aDescriptorSetLayouts;
	
	u32                aPushConstantCount = 0;
	VkShaderStageFlags aPushConstantStages = 0;
};


struct ComputePipeline_t
{
	VkPipeline aPipeline = VK_NULL_HANDLE;
	VkPipelineLayout aPipelineLayout = VK_NULL_HANDLE;
};


// for future use
struct RayTracingPipeline_t
{
	VkPipeline aPipeline = VK_NULL_HANDLE;
	VkPipelineLayout aPipelineLayout = VK_NULL_HANDLE;
};


// -----------------------------------------------------------------------

// 
// maybe make some sort of struct of shader requirements? idk
// or, falling back to object oriented, blech, make a mini class per shader
// that could be used by game code to define certain things, like uniform buffers
// or push constants, or shader storage buffers, etc.
// 
// bruh->GetShaderBufferSize( sShader, sBufferType );
// bruh->SetShaderBufferData( sShader, sBufferType, void* spData, sOffset );
// 


class ShaderSystem
{
private:
	VkShaderModule        CreateShaderModule( const std::string& aPath );

	VkPipeline            CreateGraphicsPipeline();
	VkPipeline            CreateComputePipeline();
	VkPipeline            CreateRayTracingPipeline();

	void                  CreateShaderPipeline( const ShaderCreateInfo_t& aCreateInfo, ShaderPipelineInfo_t& aPipelineInfo );
	
public:
	
	void                  Init();
	void                  Shutdown();
						  
	HShader               CreateShader( const ShaderCreateInfo_t& srInfo );
	void                  DestroyShader( HShader hShader );
	
	HShader               GetShader( const std::string& srName );
	HShader               GetShader( std::string_view sName );
						  
	GraphicsPipeline_t*   GetGraphicsPipeline( HShader hShader );
	ComputePipeline_t*    GetComputePipeline( HShader hShader );
						  
	bool                  BindPipeline( HShader hShader );
	
	// bool                  Draw( HShader hShader );
	// bool                  Dispatch( HShader hShader );
	// bool                  Draw( HShader hShader, u32 aVertexCount, u32 aInstanceCount );
	
	// bool                  BindPipeline( HShader hShader, ShaderPipelineType aPipelineType );

	VkFormat              GetVertexAttributeVkFormat( VertexAttribute attrib );
	GraphicsFmt           GetVertexAttributeFormat( VertexAttribute attrib );
	size_t                GetVertexAttributeTypeSize( VertexAttribute attrib );
	size_t                GetVertexAttributeSize( VertexAttribute attrib );
	size_t                GetVertexFormatSize( VertexFormat format );
				       	  
	void                  GetVertexBindingDesc( VertexFormat format, std::vector< VkVertexInputBindingDescription >& srAttrib );
	void                  GetVertexAttributeDesc( VertexFormat format, std::vector< VkVertexInputAttributeDescription >& srAttrib );
	
	// -----------------------------------------------------------------------

	ResourceManager< GraphicsPipeline_t >                        aShaderPipelines;

	std::unordered_map< std::string_view, ShaderCreateInfo_t >   aShaderCreateInfos;
	std::unordered_map< std::string_view, HShader >              aShaderNames;
	
	// std::unordered_map< HShader, GraphicsPipeline_t >            aGraphicsPipelines;
};


VkFormat ToVkFormat( GraphicsFmt colorFmt );


ShaderSystem& GetShaderSystem();


using HBuffer = Handle;


struct BufferCreateInfo_t
{
	std::string aName;
	VkBufferUsageFlagBits aUsage;
	VkMemoryPropertyFlags aMemFlags;
	size_t aSize;
};


// TODO:
// look into vma stuff and look at stuff like a "Dynamic Buffer Ring", and other things like that
class BufferManager
{
public:
	HBuffer               CreateBuffer( const BufferCreateInfo_t& srInfo );
	void                  DestroyBuffer( HBuffer hBuffer );
	
	void                  SetBufferData( HBuffer hBuffer, void* spData );

	// maybe this might be possible if the buffer is dynamic, or on the cpu still
	// void*                 GetBufferData( HShaderBuffer hBuffer );

	size_t                GetBufferSize( HBuffer hBuffer );
	
	std::string           GetBufferName( HBuffer hBuffer );
	EShaderBufferType     GetBufferType( HBuffer hBuffer );
	
private:
	struct Buffer_t
	{
		std::string aName;  // hmm
		VkBuffer aBuffer;
		VkDeviceMemory aMemory;
		size_t aSize;
	};
	
	// -----------------------------------------------------------------------
	
	ResourceManager< Buffer_t > aBuffers;
	
	std::unordered_map< std::string_view, HBuffer >            aBufferNames;
	std::unordered_map< std::string_view, BufferCreateInfo_t > aBufferCreateInfos;
};

