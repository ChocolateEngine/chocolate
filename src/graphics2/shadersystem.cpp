#include "shadersystem.h"
#include "core/resources.hh"
#include "gutil.hh"
#include "config.hh"
#include "swapchain.h"

#include <vulkan/vulkan.h>


ShaderSystem& GetShaderSystem()
{
	static ShaderSystem shaderSystem;
	return shaderSystem;
}


void ShaderSystem::Init()
{
}


void ShaderSystem::Shutdown()
{
}


// TODO: give more control over shader creation
// and make that chocolate shader format
HShader ShaderSystem::CreateShader( const ShaderCreateInfo_t& srInfo )
{
	// check if we already made this shader
	auto it = aShaderNames.find( srInfo.aName );
	
	if( it != aShaderNames.end() )
	{
		LogWarn( gGraphics2Channel, "Shader \"%s\" already exists\n", srInfo.aName.c_str() );
		return it->second;
	}

	// ---------------------------------------------------------------------------------------------------------------------
	// create the shader stage create infos
	std::vector< VkPipelineShaderStageCreateInfo > shaderStages;
	
	for ( size_t i = 0; i < srInfo.aModules.size(); i++ )
	{
		// const std::string& modulePath = srInfo.aModules[i];
		ShaderCreateInfo_t::Module_t moduleInfo = srInfo.aModules[i];

		// load the shader module
		VkShaderModule shaderModule = CreateShaderModule( moduleInfo.aPath );

		if ( shaderModule == VK_NULL_HANDLE )
		{
			LogError( gGraphics2Channel, "Failed to create shader module \"%s\"\n", moduleInfo.aPath.c_str() );
			return InvalidHandle;
		}

		// create the shader stage info
		VkPipelineShaderStageCreateInfo& createInfo = shaderStages.emplace_back();
		createInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		createInfo.stage = moduleInfo.aStage;
		createInfo.module = shaderModule;
		createInfo.pName = moduleInfo.aEntry.c_str();
	}

	// ---------------------------------------------------------------------------------------------------------------------
	// setup the vertex input bindings and attributes
	std::vector< VkVertexInputBindingDescription > bindingDescriptions;
	std::vector< VkVertexInputAttributeDescription > attributeDescriptions;

	GetVertexBindingDesc( srInfo.aVertexFormat, bindingDescriptions );
	GetVertexAttributeDesc( srInfo.aVertexFormat, attributeDescriptions );

	VkPipelineVertexInputStateCreateInfo vertexInputInfo{ VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO  };
	vertexInputInfo.vertexBindingDescriptionCount 	= ( u32 )bindingDescriptions.size();
	vertexInputInfo.pVertexBindingDescriptions 	= bindingDescriptions.data();			//	Contains details for loading vertex data
	vertexInputInfo.vertexAttributeDescriptionCount = ( u32 )( attributeDescriptions.size() );
	vertexInputInfo.pVertexAttributeDescriptions 	= attributeDescriptions.data();	//	Same as above

	// ---------------------------------------------------------------------------------------------------------------------
	// setup the input assembly state
	VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo{ VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO };
	inputAssemblyInfo.topology = srInfo.aPrimitiveTopology;
	inputAssemblyInfo.primitiveRestartEnable = VK_FALSE;

	// ---------------------------------------------------------------------------------------------------------------------
	// setup the viewport state
	VkPipelineViewportStateCreateInfo viewportStateInfo{ VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO };
	viewportStateInfo.viewportCount = srInfo.aViewports.size();
	viewportStateInfo.scissorCount = srInfo.aScissors.size();
	viewportStateInfo.pScissors = srInfo.aScissors.data();
	viewportStateInfo.pViewports = srInfo.aViewports.data();

	// ---------------------------------------------------------------------------------------------------------------------
	// setup the rasterizer state
	VkPipelineRasterizationStateCreateInfo rasterizerStateInfo{ VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO };
	rasterizerStateInfo.depthClampEnable = VK_FALSE;
	rasterizerStateInfo.rasterizerDiscardEnable = VK_FALSE;
	rasterizerStateInfo.polygonMode = srInfo.aPolygonMode;
	rasterizerStateInfo.lineWidth = 1.0f;
	rasterizerStateInfo.cullMode = srInfo.aCullMode;
	rasterizerStateInfo.frontFace = srInfo.aFrontFace;
	rasterizerStateInfo.depthBiasEnable = VK_FALSE;
	rasterizerStateInfo.depthBiasConstantFactor = 0.0f;
	rasterizerStateInfo.depthBiasClamp = 0.0f;
	rasterizerStateInfo.depthBiasSlopeFactor = 0.0f;

	// ---------------------------------------------------------------------------------------------------------------------
	// setup the multisampling state
	VkPipelineMultisampleStateCreateInfo multisamplingStateInfo{ VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO };
	multisamplingStateInfo.sampleShadingEnable = VK_FALSE;
	multisamplingStateInfo.rasterizationSamples = srInfo.aRasterizationSamples;
	multisamplingStateInfo.minSampleShading = 1.0f;
	multisamplingStateInfo.pSampleMask = nullptr;
	multisamplingStateInfo.alphaToCoverageEnable = srInfo.aAlphaToCoverageEnable;
	multisamplingStateInfo.alphaToOneEnable = srInfo.aAlphaToOneEnable;
	
	// ---------------------------------------------------------------------------------------------------------------------
	// setup the color blend state
	VkPipelineColorBlendAttachmentState colorBlendAttachmentState{};
	colorBlendAttachmentState.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	colorBlendAttachmentState.blendEnable = VK_TRUE;
	colorBlendAttachmentState.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
	colorBlendAttachmentState.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	colorBlendAttachmentState.colorBlendOp = VK_BLEND_OP_ADD;
	colorBlendAttachmentState.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	colorBlendAttachmentState.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	colorBlendAttachmentState.alphaBlendOp = VK_BLEND_OP_ADD;

	// ---------------------------------------------------------------------------------------------------------------------
	// setup depth stencil state
	VkPipelineDepthStencilStateCreateInfo depthStencilInfo{ VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO };
	depthStencilInfo.depthTestEnable = srInfo.aEnableDepthTest;
	depthStencilInfo.depthWriteEnable = srInfo.aEnableDepthWrite;
	depthStencilInfo.depthCompareOp = srInfo.aDepthCompareOp;
	depthStencilInfo.depthBoundsTestEnable = VK_FALSE;
	depthStencilInfo.stencilTestEnable = VK_FALSE;
	depthStencilInfo.front = {};
	depthStencilInfo.back = {};
	depthStencilInfo.minDepthBounds = 0.0f;
	depthStencilInfo.maxDepthBounds = 1.0f;

	// ---------------------------------------------------------------------------------------------------------------------
	// setup the color blend state
	VkPipelineColorBlendStateCreateInfo colorBlendInfo{ VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO };
	colorBlendInfo.logicOpEnable = VK_FALSE;
	colorBlendInfo.logicOp = VK_LOGIC_OP_COPY;
	colorBlendInfo.attachmentCount = 1;
	colorBlendInfo.pAttachments = &colorBlendAttachmentState;
	colorBlendInfo.blendConstants[ 0 ] = 0.0f;
	colorBlendInfo.blendConstants[ 1 ] = 0.0f;
	colorBlendInfo.blendConstants[ 2 ] = 0.0f;
	colorBlendInfo.blendConstants[ 3 ] = 0.0f;
	
	// ---------------------------------------------------------------------------------------------------------------------
	// setup the dynamic state
	VkPipelineDynamicStateCreateInfo dynamicStateInfo{ VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO };
	dynamicStateInfo.dynamicStateCount = srInfo.aDynamicStates.size();
	dynamicStateInfo.pDynamicStates = srInfo.aDynamicStates.data();
	
	// ---------------------------------------------------------------------------------------------------------------------
	// setup the pipeline layout
	VkPipelineLayoutCreateInfo pipelineLayoutInfo{ VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };
	pipelineLayoutInfo.setLayoutCount = srInfo.aSetLayouts.size();
	pipelineLayoutInfo.pSetLayouts = srInfo.aSetLayouts.data();
	pipelineLayoutInfo.pushConstantRangeCount = srInfo.aPushConstantRanges.size();
	pipelineLayoutInfo.pPushConstantRanges = srInfo.aPushConstantRanges.data();
	VkPipelineLayout pipelineLayout;
	CheckVkResultF( vkCreatePipelineLayout( GetDevice(), &pipelineLayoutInfo, nullptr, &pipelineLayout ), "Failed to create pipeline layout for shader \"%s\"", srInfo.aName.c_str() );

	// ---------------------------------------------------------------------------------------------------------------------
	// setup the pipeline
	VkGraphicsPipelineCreateInfo pipelineInfo{ VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO };
	pipelineInfo.stageCount = shaderStages.size();
	pipelineInfo.pStages = shaderStages.data();
	pipelineInfo.pVertexInputState = &vertexInputInfo;
	pipelineInfo.pInputAssemblyState = &inputAssemblyInfo;
	pipelineInfo.pViewportState = &viewportStateInfo;
	pipelineInfo.pRasterizationState = &rasterizerStateInfo;
	pipelineInfo.pMultisampleState = &multisamplingStateInfo;
	pipelineInfo.pDepthStencilState = &depthStencilInfo;
	pipelineInfo.pColorBlendState = &colorBlendInfo;
	pipelineInfo.pDynamicState = &dynamicStateInfo;
	pipelineInfo.layout = pipelineLayout;

	// NOTE: because of the render graph system going to allow multiple render passes, how will this work here?
	pipelineInfo.renderPass = renderPass;
	
	pipelineInfo.subpass = 0;
	pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
	pipelineInfo.basePipelineIndex = -1;
	VkPipeline pipeline;
	CheckVkResultF( vkCreateGraphicsPipelines( GetDevice(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline ), "Failed to create pipeline for shader \"%s\"", srInfo.aName.c_str() );
	
	// ---------------------------------------------------------------------------------------------------------------------
	// store the pipeline
	GraphicsPipeline_t* shader = new GraphicsPipeline_t;
	shader->aPipeline = pipeline;
	shader->aPipelineLayout = pipelineLayout;
	shader->aPushConstantCount = srInfo.aPushConstantRanges.size();

	// take all VkShaderStageFlags and turn them into a bitmask
	uint32_t stageMask = 0;
	for( auto pushConst : srInfo.aPushConstantRanges )
	{
		stageMask |= pushConst.stageFlags;
	}

	shader->aPushConstantStages = stageMask;

	return aShaderPipelines.Add( shader );
}


VkShaderModule ShaderSystem::CreateShaderModule( const std::string& aPath )
{
	std::vector< char > data = filesys->ReadFile( aPath );

	if ( data.empty() )
	{
		// could also be not found
		LogError( gGraphics2Channel, "Shader Module data is empty\"%s\"\n", aPath.c_str() );
		return VK_NULL_HANDLE;
	}

	VkShaderModuleCreateInfo createInfo{};
	VkShaderModule           shaderModule;
	
	createInfo.sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	createInfo.codeSize = data.size();
	createInfo.pCode    = ( const uint32_t* )data.data();
	
	CheckVKResult( vkCreateShaderModule( GetDevice(), &createInfo, NULL, &shaderModule ), "Failed to create shader module!" );
	
	return shaderModule;
}


void ShaderSystem::CreateShaderPipeline( const ShaderCreateInfo_t& aCreateInfo, ShaderPipelineInfo_t& aPipelineInfo )
{
	
}


HShader ShaderSystem::GetShader( const std::string& srName )
{
	auto it = aShaderNames.find( srName );

	if ( it == aShaderNames.end() )
	{
		LogError( gGraphics2Channel, "Shader \"%s\" not found!\n", srName.c_str() );
		return InvalidHandle;
	}
	
	return it->second;
}


HShader ShaderSystem::GetShader( std::string_view sName )
{
	auto it = aShaderNames.find( sName );

	if ( it == aShaderNames.end() )
	{
		LogError( gGraphics2Channel, "Shader \"%s\" not found!\n", sName.data() );
		return InvalidHandle;
	}
	
	return it->second;
}


bool ShaderSystem::BindPipeline( HShader hShader )
{
	if ( hShader == InvalidHandle )
	{
		// vkCmdBindPipeline( GetCommandBuffer(), VK_PIPELINE_BIND_POINT_GRAPHICS, VK_NULL_HANDLE );
		return false;
	}

	// NOTE: need to deduce shader type here, is it a compute pipeline? graphics pipeline? idk
	
	GraphicsPipeline_t* shader = aShaderPipelines.Get( hShader );

	// check if shader is nullptr
	if ( shader == nullptr )
	{
		LogError( gGraphics2Channel, "Shader is nullptr!" );
		return false;
	}

	// GetCommandBuffer() is not a thing, but could be
	vkCmdBindPipeline( GetCommandBuffer(), VK_PIPELINE_BIND_POINT_GRAPHICS, shader->aPipeline );

	return true;
}


// bool ShaderSystem::Draw( uint32_t aVertexCount, uint32_t aInstanceCount )
// {
// 	vkCmdDraw( GetCommandBuffer(), aVertexCount, aInstanceCount, 0, 0 );
// 	return true;
// }


// ----------------------------------------------------------------------------
// Vertex Functions
// ----------------------------------------------------------------------------


// Probably move this elsewhere
VkFormat ToVkFormat( GraphicsFmt colorFmt )
{
	switch ( colorFmt )
	{
		case GraphicsFmt::INVALID:
		default:
			LogError( gGraphics2Channel, "Unspecified Color Format to VkFormat Conversion!\n" );
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


VkFormat ShaderSystem::GetVertexAttributeVkFormat( VertexAttribute attrib )
{
	return ToVkFormat( GetVertexAttributeFormat( attrib ) );
}


void ShaderSystem::GetVertexBindingDesc( VertexFormat format, std::vector< VkVertexInputBindingDescription >& srAttrib )
{
	u32 binding  = 0;

	for ( u8 attrib = 0; attrib < VertexAttribute_Count; attrib++ )
	{
		// does this format contain this attribute?
		// if so, add this attribute to the vector
		if ( format & (1 << attrib) )
		{
			srAttrib.emplace_back(
				binding++,
				( u32 )GetVertexAttributeSize( (VertexAttribute)attrib ),
				VK_VERTEX_INPUT_RATE_VERTEX
			);
		}
	}
}


GraphicsFmt ShaderSystem::GetVertexAttributeFormat( VertexAttribute attrib )
{
	switch ( attrib )
	{
		default:
			LogError( gGraphics2Channel, "GetVertexAttributeFormat: Invalid VertexAttribute specified: %d\n", attrib );
			return GraphicsFmt::INVALID;

		case VertexAttribute_Position:
			return GraphicsFmt::RGB323232_SFLOAT;

		// NOTE: could be smaller probably
		case VertexAttribute_Normal:
			return GraphicsFmt::RGB323232_SFLOAT;

		case VertexAttribute_Color:
			return GraphicsFmt::RGB323232_SFLOAT;

		case VertexAttribute_TexCoord:
			return GraphicsFmt::RG3232_SFLOAT;
	}
}


size_t ShaderSystem::GetVertexAttributeTypeSize( VertexAttribute attrib )
{
	GraphicsFmt format = GetVertexAttributeFormat( attrib );

	switch ( format )
	{
		default:
			LogError( gGraphics2Channel, "GetVertexAttributeTypeSize: Invalid DataFormat specified from vertex attribute: %d\n", format );
			return 0;

		case GraphicsFmt::INVALID:
			return 0;

		// uh is this right?
		case GraphicsFmt::RGB323232_SFLOAT:
		case GraphicsFmt::RG3232_SFLOAT:
			return sizeof( float );
	}
}


size_t ShaderSystem::GetVertexAttributeSize( VertexAttribute attrib )
{
	GraphicsFmt format = GetVertexAttributeFormat( attrib );

	switch ( format )
	{
		default:
			LogError( gGraphics2Channel, "GetVertexAttributeSize: Invalid DataFormat specified from vertex attribute: %d\n", format );
			return 0;

		case GraphicsFmt::INVALID:
			return 0;

		case GraphicsFmt::RGB323232_SFLOAT:
			return ( 3 * sizeof( float ) );

		case GraphicsFmt::RG3232_SFLOAT:
			return ( 2 * sizeof( float ) );
	}
}


size_t ShaderSystem::GetVertexFormatSize( VertexFormat format )
{
	size_t size = 0;

	for ( int attrib = 0; attrib < VertexAttribute_Count; attrib++ )
	{
		// does this format contain this attribute?
		// if so, add the attribute size to it
		if ( format & (1 << attrib) )
			size += GetVertexAttributeSize( (VertexAttribute)attrib );
	}

	return size;
}


void ShaderSystem::GetVertexAttributeDesc( VertexFormat format, std::vector< VkVertexInputAttributeDescription >& srAttrib )
{
	u32 location = 0;
	u32 binding  = 0;
	u32 offset   = 0;

	for ( u8 attrib = 0; attrib < VertexAttribute_Count; attrib++ )
	{
		// does this format contain this attribute?
		// if so, add this attribute to the vector
		if ( format & (1 << attrib) )
		{
			srAttrib.emplace_back(
				location++,
				binding++,
				GetVertexAttributeVkFormat( (VertexAttribute)attrib ),
				0 // no offset
			);
		}
	}
}


// ----------------------------------------------------------------------------
// ShaderBufferManager
// ----------------------------------------------------------------------------


void CreateVkBuffer( VkBuffer& srBuffer, VkDeviceMemory& srBufferMem, u32 sBufferSize, VkBufferUsageFlags sUsage, int sMemBits )
{
	// create a buffer
	VkBufferCreateInfo aBufferInfo = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
	aBufferInfo.size = sBufferSize;
	aBufferInfo.usage = sUsage;
	aBufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;  // should be something you can change

	CheckVKResult( vkCreateBuffer( GetDevice(), &aBufferInfo, nullptr, &srBuffer ), "Failed to create vertex buffer" );

	// allocate memory for the buffer
	VkMemoryRequirements aMemReqs;
	vkGetBufferMemoryRequirements( GetDevice(), srBuffer, &aMemReqs );

	u32 memType = GetGInstance().GetMemoryType( aMemReqs.memoryTypeBits, sMemBits );

	VkMemoryAllocateInfo aMemAllocInfo = { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
	aMemAllocInfo.allocationSize = aMemReqs.size;
	aMemAllocInfo.memoryTypeIndex = memType;
	CheckVKResult( vkAllocateMemory( GetDevice(), &aMemAllocInfo, nullptr, &srBufferMem ), "Failed to allocate vertex buffer memory" );

	// bind the buffer to the device memory
	CheckVKResult( vkBindBufferMemory( GetDevice(), srBuffer, srBufferMem, 0 ), "Failed to bind vertex buffer" );
}


HBuffer BufferManager::CreateBuffer( const BufferCreateInfo_t& srInfo )
{
	// check if a buffer with this name exists
	auto it = aBufferNames.find( srInfo.aName );

	if ( it == aBufferNames.end() )
	{
		LogError( gGraphics2Channel, "Buffer name is already taken: %s\n", srInfo.aName );
		return InvalidHandle;
	}

	Buffer_t* buffer = new Buffer_t;

	CreateVkBuffer( buffer->aBuffer, buffer->aMemory, srInfo.aSize, srInfo.aUsage, srInfo.aMemFlags );

	HBuffer hBuffer = aBuffers.Add( buffer );
	aBufferCreateInfos[ srInfo.aName ] = srInfo;
	aBufferNames[ srInfo.aName ] = hBuffer;

	return hBuffer;
}


void BufferManager::DestroyBuffer( HBuffer hBuffer )
{
	Buffer_t* buffer = aBuffers.Get( hBuffer );

	// check if buffer is nullptr
	if ( buffer == nullptr )
	{
		LogError( gGraphics2Channel, "DestroyBuffer(): Buffer is nullptr!" );
		return;
	}

	vkUnmapMemory( GetDevice(), buffer->aMemory );
	vkFreeMemory( GetDevice(), buffer->aMemory, NULL );
	vkDestroyBuffer( GetDevice(), buffer->aBuffer, NULL );

	aBufferNames.erase( buffer->aName );
	aBufferCreateInfos.erase( buffer->aName );
}


void BufferManager::SetBufferData( HBuffer hBuffer, void* spData )
{
	Buffer_t* buffer = aBuffers.Get( hBuffer );
	
	// check if buffer is nullptr
	if ( buffer == nullptr )
	{
		LogError( gGraphics2Channel, "SetBufferData(): Buffer is nullptr!" );
		return;
	}

	void* vramData;
	vkMapMemory( GetDevice(), buffer->aMemory, 0, buffer->aSize, 0, &vramData );
	memcpy( vramData, &spData, buffer->aSize );
	vkUnmapMemory( GetDevice(), buffer->aMemory );
}

