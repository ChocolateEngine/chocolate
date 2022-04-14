/*
shader_basic_3d.cpp ( Authored by Demez )

The Basic 3D Shader, starting point shader
*/
#include "../renderer.h"
#include "shaders.h"

#include <mutex>


extern size_t gModelDrawCalls;
extern size_t gVertsDrawn;


constexpr const char *pVShader = "shaders/basic3d.vert.spv";
constexpr const char *pFShader = "shaders/basic3d.frag.spv";


struct Basic3D_PushConst
{
	glm::mat4 projView;
	glm::mat4 model;
};


struct Basic3D_UBO
{
	int diffuse, ao, emissive;
	float aoPower, emissivePower;
};


struct Basic3D_DrawData
{
	Basic3D_PushConst  aPushConst;
	Basic3D_UBO        aUBO;
	UniformDescriptor* apUniformDesc;
};


// KTX fails to load this and i don't feel like figuring out why right now
static std::string gFallbackEmissivePath = "materials/base/black.png";
static std::string gFallbackAOPath = "materials/base/white.png";

Texture* gFallbackEmissive = nullptr;
Texture* gFallbackAO = nullptr;


// =========================================================


class Basic3D : public BaseShader
{
public:	

	Basic3D()
	{
		GetMaterialSystem()->AddShader( this, "basic_3d" );
	}

	inline bool UsesUniformBuffers(  ) override { return true; };


	virtual void Init() override
	{
		aModules.Allocate(2);
		aModules[0] = CreateShaderModule( filesys->ReadFile( pVShader ) ); // Processes incoming verticies, taking world position, color, and texture coordinates as an input
		aModules[1] = CreateShaderModule( filesys->ReadFile( pFShader ) ); // Fills verticies with fragments to produce color, and depth

		gFallbackEmissive = matsys->CreateTexture( gFallbackEmissivePath );
		gFallbackAO       = matsys->CreateTexture( gFallbackAOPath );

		BaseShader::Init();
	}


	virtual void ReInit() override
	{
		aModules[0] = CreateShaderModule( filesys->ReadFile( pVShader ) ); // Processes incoming verticies, taking world position, color, and texture coordinates as an input
		aModules[1] = CreateShaderModule( filesys->ReadFile( pFShader ) ); // Fills verticies with fragments to produce color, and depth

		BaseShader::ReInit();
	}


	virtual void        CreateLayouts() override;

	virtual void        CreateGraphicsPipeline(  ) override;

	virtual void        InitUniformBuffer( IMesh* mesh ) override;

	virtual void        UpdateBuffers( uint32_t sCurrentImage, BaseRenderable* spRenderable ) {};
	virtual void        UpdateBuffers( uint32_t sCurrentImage, size_t renderableIndex, BaseRenderable* spRenderable ) override;

	virtual void        Draw( BaseRenderable* renderable, VkCommandBuffer c, uint32_t commandBufferIndex ) override {}
	virtual void        Draw( size_t renderableIndex, BaseRenderable* renderable, VkCommandBuffer c, uint32_t commandBufferIndex ) override;

	virtual void        AllocDrawData( size_t sRenderableCount ) override;
	virtual void        PrepareDrawData( size_t renderableIndex, BaseRenderable* renderable, uint32_t commandBufferCount ) override;

	std::unordered_map< BaseRenderable*, Basic3D_DrawData* > aDrawData;
	MemPool aDrawDataPool;
};


Basic3D* gpBasic3D = new Basic3D;


void Basic3D::CreateLayouts()
{
	aLayouts.Allocate( 2 );
	aLayouts[0] = InitDescriptorSetLayout( DescriptorLayoutBinding( VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr ) );
	aLayouts[1] = matsys->aImageLayout;
}


void Basic3D::CreateGraphicsPipeline(  )
{
	aPipelineLayout = InitPipelineLayouts( aLayouts.GetBuffer(  ), aLayouts.GetSize(  ), sizeof( Basic3D_PushConst ) );

	VkPipelineShaderStageCreateInfo vertShaderStageInfo{  };
	vertShaderStageInfo.sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;	//	Tells Vulkan which stage the shader is going to be used
	vertShaderStageInfo.stage  = VK_SHADER_STAGE_VERTEX_BIT;
	vertShaderStageInfo.module = aModules[0];
	vertShaderStageInfo.pName  = "main";							//	void main() in the shader to be executed

	VkPipelineShaderStageCreateInfo fragShaderStageInfo{  };
	fragShaderStageInfo.sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	fragShaderStageInfo.stage  = VK_SHADER_STAGE_FRAGMENT_BIT;
	fragShaderStageInfo.module = aModules[1];
	fragShaderStageInfo.pName  = "main";

	/* Stack overflow lol.  */
	VkPipelineShaderStageCreateInfo *pShaderStages = new VkPipelineShaderStageCreateInfo[ 2 ];
	pShaderStages[ 0 ] = vertShaderStageInfo;
	pShaderStages[ 1 ] = fragShaderStageInfo;

	auto attributeDescriptions 	= vertex_3d_t::GetAttributeDesc(  );
	auto bindingDescription 	= vertex_3d_t::GetBindingDesc(  );

	VkPipelineVertexInputStateCreateInfo vertexInputInfo{  };	//	Format of vertex data
	vertexInputInfo.sType 				= VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertexInputInfo.vertexBindingDescriptionCount 	= 1;
	vertexInputInfo.pVertexBindingDescriptions 	= &bindingDescription;			//	Contains details for loading vertex data
	vertexInputInfo.vertexAttributeDescriptionCount = ( uint32_t )( attributeDescriptions.size(  ) );
	vertexInputInfo.pVertexAttributeDescriptions 	= attributeDescriptions.data(  );	//	Same as above

	VkPipelineInputAssemblyStateCreateInfo inputAssembly{  };	//	Collects raw vertex data from buffers
	inputAssembly.sType 			= VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssembly.topology 			= VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	inputAssembly.primitiveRestartEnable 	= VK_FALSE;

	/* Region of frambuffer to be rendered to, likely will always use 0, 0 and width, height  */
	VkViewport viewport = Viewport( 0.f, 0.f, ( float )PSWAPCHAIN.GetExtent(  ).width, ( float )PSWAPCHAIN.GetExtent(  ).height, 0.f, 1.0f );

	VkRect2D scissor{  };		//	More agressive cropping than viewport, defining which regions pixels are to be stored
	scissor.offset = { 0, 0 };
	scissor.extent = PSWAPCHAIN.GetExtent(  );

	VkPipelineViewportStateCreateInfo viewportState{  };	//	Combines viewport and scissor
	viewportState.sType 		= VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportState.viewportCount 	= 1;
	viewportState.pViewports 	= &viewport;
	viewportState.scissorCount 	= 1;
	viewportState.pScissors 	= &scissor;

	VkPipelineRasterizationStateCreateInfo rasterizer{  };		//	Turns  primitives into fragments, aka, pixels for the framebuffer
	rasterizer.sType 			= VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizer.depthClampEnable 		= VK_FALSE;
	rasterizer.rasterizerDiscardEnable 	= VK_FALSE;
	rasterizer.polygonMode 			= VK_POLYGON_MODE_FILL;		//	Fill with fragments, can optionally use VK_POLYGON_MODE_LINE for a wireframe
	rasterizer.lineWidth 			= 1.0f;
	// rasterizer.cullMode 			= ( sFlags & NO_CULLING ) ? VK_CULL_MODE_NONE : VK_CULL_MODE_BACK_BIT;	//	FIX FOR MAKING 2D SPRITES WORK!!! WOOOOO!!!!
	rasterizer.cullMode 			= VK_CULL_MODE_BACK_BIT;
	rasterizer.frontFace 			= VK_FRONT_FACE_COUNTER_CLOCKWISE;
	rasterizer.depthBiasEnable 		= VK_FALSE;
	rasterizer.depthBiasConstantFactor 	= 0.0f; // Optional
	rasterizer.depthBiasClamp 		= 0.0f; // Optional
	rasterizer.depthBiasSlopeFactor 	= 0.0f; // Optional

	VkPipelineMultisampleStateCreateInfo multisampling{  };		//	Performs anti-aliasing
	multisampling.sType 		    = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampling.sampleShadingEnable   = VK_FALSE;
	multisampling.rasterizationSamples  = gpDevice->GetSamples(  );
	multisampling.minSampleShading 	    = 1.0f; // Optional
	multisampling.pSampleMask 	    = NULL; // Optional
	multisampling.alphaToCoverageEnable = VK_FALSE; // Optional
	multisampling.alphaToOneEnable 	    = VK_FALSE; // Optional

	VkPipelineColorBlendAttachmentState colorBlendAttachment{  };
	colorBlendAttachment.colorWriteMask 	 = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	colorBlendAttachment.blendEnable 	 = VK_TRUE;
	colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
	colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	colorBlendAttachment.colorBlendOp	 = VK_BLEND_OP_ADD;
	colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	colorBlendAttachment.alphaBlendOp 	 = VK_BLEND_OP_ADD;

	VkPipelineDepthStencilStateCreateInfo depthStencil{  };
	depthStencil.sType 			= VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	//depthStencil.depthTestEnable 		= ( sFlags & NO_DEPTH ) ? VK_FALSE : VK_TRUE;
	//depthStencil.depthWriteEnable		= ( sFlags & NO_DEPTH ) ? VK_FALSE : VK_TRUE;
	depthStencil.depthTestEnable 		= VK_TRUE;
	depthStencil.depthWriteEnable		= VK_TRUE;
	depthStencil.depthCompareOp 		= VK_COMPARE_OP_LESS;
	depthStencil.depthBoundsTestEnable 	= VK_FALSE;
	depthStencil.minDepthBounds 		= 0.0f; // Optional
	depthStencil.maxDepthBounds 		= 1.0f; // Optional
	depthStencil.stencilTestEnable 		= VK_FALSE;
	depthStencil.front 			= {  }; // Optional
	depthStencil.back 			= {  }; // Optional

	VkPipelineColorBlendStateCreateInfo colorBlending{  };
	colorBlending.sType 		  = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlending.logicOpEnable 	  = VK_FALSE;
	colorBlending.logicOp 	          = VK_LOGIC_OP_COPY; // Optional
	colorBlending.attachmentCount 	  = 1;
	colorBlending.pAttachments 	  = &colorBlendAttachment;
	colorBlending.blendConstants[ 0 ] = 0.0f; // Optional
	colorBlending.blendConstants[ 1 ] = 0.0f; // Optional
	colorBlending.blendConstants[ 2 ] = 0.0f; // Optional
	colorBlending.blendConstants[ 3 ] = 0.0f; // Optional

	VkDynamicState dynamicStates[  ] = {	//	Allows for some pipeline configuration values to change during runtime
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_LINE_WIDTH
	};

	VkPipelineDynamicStateCreateInfo dynamicState{  };
	dynamicState.sType 		= VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamicState.dynamicStateCount  = 2;
	dynamicState.pDynamicStates 	= dynamicStates;

	VkGraphicsPipelineCreateInfo pipelineInfo{  };	//	Combine all the objects above into one parameter for graphics pipeline creation
	pipelineInfo.sType 			= VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineInfo.stageCount 		= 2;
	pipelineInfo.pStages 			= pShaderStages;
	pipelineInfo.pVertexInputState 		= &vertexInputInfo;
	pipelineInfo.pInputAssemblyState 	= &inputAssembly;
	pipelineInfo.pViewportState 		= &viewportState;
	pipelineInfo.pRasterizationState 	= &rasterizer;
	pipelineInfo.pMultisampleState 		= &multisampling;
	pipelineInfo.pDepthStencilState 	= &depthStencil;
	pipelineInfo.pColorBlendState 		= &colorBlending;
	pipelineInfo.pDynamicState 		= NULL; // Optional
	pipelineInfo.layout 			= aPipelineLayout;
	pipelineInfo.renderPass 		= *gpRenderPass;
	pipelineInfo.subpass 			= 0;
	pipelineInfo.basePipelineHandle 	= VK_NULL_HANDLE; // Optional, very important for later when making new pipelines. It is less expensive to reference an existing similar pipeline
	pipelineInfo.basePipelineIndex 		= -1; // Optional

	if ( vkCreateGraphicsPipelines( DEVICE, VK_NULL_HANDLE, 1, &pipelineInfo, NULL, &aPipeline ) != VK_SUCCESS )
		throw std::runtime_error( "Failed to create graphics pipeline!" );
}


void Basic3D::InitUniformBuffer( IMesh* mesh )
{
	auto it = matsys->aUniformLayoutMap.find( mesh->GetID() );

	// we have one already lol
	if ( it != matsys->aUniformLayoutMap.end() )
		return;

	matsys->aUniformLayoutMap[mesh->GetID()] = InitDescriptorSetLayout( {{ DescriptorLayoutBinding( VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, NULL ) }} );

	matsys->aUniformDataMap[mesh->GetID()] = UniformDescriptor{};

	InitUniformData( matsys->aUniformDataMap[mesh->GetID()], matsys->aUniformLayoutMap[mesh->GetID()], sizeof( Basic3D_UBO ) );
}


// TODO: only update this during init or swapchain recreation if needed
// because this has got to be slowing this whole thing down lol
void Basic3D::UpdateBuffers( uint32_t sCurrentImage, size_t renderableIndex, BaseRenderable* spRenderable )
{
	Basic3D_DrawData* drawData = (Basic3D_DrawData*)(aDrawDataPool.GetStart() + (sizeof( Basic3D_DrawData ) * renderableIndex));

	auto& uniformDataMem = drawData->apUniformDesc->aMem[ sCurrentImage ];

	void* data;
	vkMapMemory( DEVICE, uniformDataMem, 0, sizeof( drawData->aUBO ), 0, &data );
	memcpy( data, &drawData->aUBO, sizeof( drawData->aUBO ) );
	vkUnmapMemory( DEVICE, uniformDataMem );
}


void Basic3D::Draw( size_t renderableIndex, BaseRenderable* renderable, VkCommandBuffer c, uint32_t i )
{
	// why did i do this, remove this ASAP
	/*bool isMesh = typeid(*renderable) == typeid(IMesh);
	assert( isMesh );

	if ( !isMesh )
	{
		LogWarn( "Basic3D::Draw - Not a Mesh!\n" );
		return;
	}*/

	// use is base of

	IMesh* mesh = static_cast<IMesh*>(renderable);

	Basic3D_DrawData* drawData = (Basic3D_DrawData*)(aDrawDataPool.GetStart() + (sizeof( Basic3D_DrawData ) * renderableIndex));

	// Bind the mesh's vertex and index buffers
	VkBuffer 	vBuffers[  ] 	= { mesh->GetVertexBuffer()};
	VkDeviceSize 	offsets[  ] 	= { 0 };
	vkCmdBindVertexBuffers( c, 0, 1, vBuffers, offsets );

	VkBuffer indexBuffer = mesh->GetIndexBuffer();
	if ( indexBuffer )
		vkCmdBindIndexBuffer( c, indexBuffer, 0, VK_INDEX_TYPE_UINT32 );

	// we don't need this in the fragment shader aaaa
	vkCmdPushConstants(
		c, aPipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
		0, sizeof( Basic3D_PushConst ), &drawData->aPushConst
	);

	VkDescriptorSet sets[  ] = {
		drawData->apUniformDesc->aSets[i],
		matsys->aImageSets[i],
	};

	vkCmdBindDescriptorSets( c, VK_PIPELINE_BIND_POINT_GRAPHICS, aPipelineLayout, 0, 2, sets, 0, NULL );

	if ( indexBuffer )
		vkCmdDrawIndexed( c, (uint32_t)mesh->GetIndices().size(), 1, 0, 0, 0);
	else
		vkCmdDraw( c, (uint32_t)mesh->GetVertices().size(), 1, 0, 0);

	gModelDrawCalls++;
	gVertsDrawn += mesh->GetVertices().size();
}


void Basic3D::AllocDrawData( size_t sRenderableCount )
{
	aDrawDataPool.Clear();
	Assert( MemPool_OutOfMemory != aDrawDataPool.Resize( sizeof( Basic3D_DrawData ) * sRenderableCount ) );
}


// here to avoid constantly creating std::strings
// maybe just change mat var names to const char*
// if doing strlen and strcmp can reach the same speed as std::string
// ... which i doubt

static std::string MatVar_Diffuse          = "diffuse";
static std::string MatVar_Emissive         = "emissive";
static std::string MatVar_AO               = "ao";

static std::string MatVar_AOPower          = "ao_power";
static std::string MatVar_EmissivePower    = "emissive_power";


void Basic3D::PrepareDrawData( size_t renderableIndex, BaseRenderable* renderable, uint32_t commandBufferCount )
{
	// there is the old DataBuffer class as well, hmm

	IMesh* mesh = static_cast<IMesh*>(renderable);

	Basic3D_DrawData* drawData = (Basic3D_DrawData*)(aDrawDataPool.GetStart() + (sizeof( Basic3D_DrawData ) * renderableIndex));

	drawData->aPushConst = {renderer->aView.projViewMatrix, mesh->GetModelMatrix()};
	drawData->apUniformDesc = &matsys->GetUniformData( mesh->GetID() );

	auto mat = (Material*)mesh->GetMaterial();

	// NOTE: should get the MeshPtr directly so there can be less matvar calls since it would always be the same material

	drawData->aUBO.diffuse         = mat->GetTextureId( MatVar_Diffuse );
	drawData->aUBO.emissive        = mat->GetTextureId( MatVar_Emissive, gFallbackEmissive );
	drawData->aUBO.ao              = mat->GetTextureId( MatVar_AO, gFallbackAO );

	drawData->aUBO.aoPower         = mat->GetFloat( MatVar_AOPower, 1.f );
	drawData->aUBO.emissivePower   = mat->GetFloat( MatVar_EmissivePower, 1.f );
}

