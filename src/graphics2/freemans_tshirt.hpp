#pragma once

// Super Basic Test Model Rendering
#include "config.hh"

struct freeman_push_t
{
	alignas( 16 )glm::mat4 aMatrix;
	alignas( 16 )int       aTexIndex;
};


struct vertex_t
{
	glm::vec3 aPos;
	glm::vec2 aUV;
};


// Copies the memory to the GPU.
static void vk_memcpy( VkDeviceMemory sBufferMemory, VkDeviceSize sSize, const void *spData )
{
	void *pData;
	vkMapMemory( GetDevice(), sBufferMemory, 0, sSize, 0, &pData );
	memcpy( pData, spData, ( size_t )sSize );
	vkUnmapMemory( GetDevice(), sBufferMemory );
}


class freemans_tshirt_t
{
public:
	VkPipeline       aPipeline;
	VkPipelineLayout aPipelineLayout;

	std::vector< VkDescriptorSetLayout > aLayouts;
	std::vector< VkDescriptorSet >*      apImageSets;

	std::vector< VkShaderModule > aModules;
	
	// mesh data
	std::vector< vertex_t > aVertices;
	VkBuffer                aVertexBuffer;
	VkDeviceMemory          aVertexBufferMem;

	// ------------------------------------------------------------------

	void Init( std::vector< VkDescriptorSet >* spImageSets, VkDescriptorSetLayout sImageLayout )
	{
		aModules.push_back( CreateShaderModule( filesys->ReadFile( "shaders/unlit.vert.spv" ) ) );
		aModules.push_back( CreateShaderModule( filesys->ReadFile( "shaders/unlit.frag.spv" ) ) );
		
		aLayouts.push_back( sImageLayout );
		
		CreateGraphicsPipeline();

		apImageSets = spImageSets;

		CreateMesh();
	}

	void Draw( VkCommandBuffer cmd, uint32_t cmdIndex )
	{
		// update push constant
		freeman_push_t aPushConstant;
		aPushConstant.aMatrix = glm::mat4( 1.0f );
		aPushConstant.aTexIndex = 0;
		vkCmdPushConstants( cmd, aPipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof( freeman_push_t ), &aPushConstant );
		
		// bind pipeline and descriptor sets
		vkCmdBindPipeline( cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, aPipeline );
		
		VkDescriptorSet sets[  ] = {
			(*apImageSets)[ cmdIndex ],
		};

		vkCmdBindDescriptorSets( cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, aPipelineLayout, 0, 1, sets, 0, nullptr);
		
		// bind vertex buffer
		VkBuffer aBuffers[] = { aVertexBuffer };
		VkDeviceSize aOffsets[] = { 0 };
		vkCmdBindVertexBuffers( cmd, 0, 1, aBuffers, aOffsets );
		
		// draw
		vkCmdDraw( cmd, aVertices.size(), 1, 0, 0 );
	}
	
	// ------------------------------------------------------------------
	
	void CreateMesh()
	{
		// create a triangle
		aVertices.emplace_back( glm::vec3{ -1.0f, -1.0f, 0.0f }, glm::vec2{ 0.0f, 0.0f } );
		aVertices.emplace_back( glm::vec3{ 1.0f, -1.0f, 0.0f }, glm::vec2{ 1.0f, 0.0f } );
		aVertices.emplace_back( glm::vec3{ 1.0f, 1.0f, 0.0f }, glm::vec2{ 1.0f, 1.0f } );

		// create a vertex buffer for the triangle
		VkDeviceSize bufferSize = sizeof( vertex_t ) * aVertices.size();

		VkBuffer        stagingBuffer;
		VkDeviceMemory  stagingBufferMemory;

		CreateBuffer(
			stagingBuffer,
			stagingBufferMemory,
			bufferSize,
			VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
		);

		vk_memcpy( stagingBufferMemory, bufferSize, aVertices.data() );

		CreateBuffer(
			aVertexBuffer,
			aVertexBufferMem,
			bufferSize,
			VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
		);
		
		// copy buffer from cpu to the gpu
		VkBufferCopy copyRegion{
			.srcOffset 	= 0,
			.dstOffset  = 0,
			.size       = bufferSize,
		};
		
		SingleCommand( [ & ]( VkCommandBuffer c ){ vkCmdCopyBuffer( c, stagingBuffer, aVertexBuffer, 1, &copyRegion ); } );

		// free staging buffer
		vkDestroyBuffer( GetDevice(), stagingBuffer, NULL );
		vkFreeMemory( GetDevice(), stagingBufferMemory, NULL );
	}

	void CreateBuffer( VkBuffer& srBuffer, VkDeviceMemory& srBufferMem, u32 sBufferSize, VkBufferUsageFlags sUsage, int sMemBits )
	{
		// create a vertex buffer
		VkBufferCreateInfo aBufferInfo = {};
		aBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		aBufferInfo.size = sBufferSize;
		aBufferInfo.usage = sUsage;
		aBufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		CheckVKResult( vkCreateBuffer( GetDevice(), &aBufferInfo, nullptr, &srBuffer ), "Failed to create vertex buffer" );

		// allocate memory for the vertex buffer
		VkMemoryRequirements aMemReqs;
		vkGetBufferMemoryRequirements( GetDevice(), srBuffer, &aMemReqs );

		u32 memType = GetGInstance().GetMemoryType( aMemReqs.memoryTypeBits, sMemBits );

		VkMemoryAllocateInfo aMemAllocInfo = {};
		aMemAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		aMemAllocInfo.allocationSize = aMemReqs.size;
		aMemAllocInfo.memoryTypeIndex = memType;
		CheckVKResult( vkAllocateMemory( GetDevice(), &aMemAllocInfo, nullptr, &srBufferMem ), "Failed to allocate vertex buffer memory" );

		// bind the vertex buffer to the device memory
		CheckVKResult( vkBindBufferMemory( GetDevice(), srBuffer, srBufferMem, 0 ), "Failed to bind vertex buffer" );
	}

	// ------------------------------------------------------------------
	
	VkShaderModule CreateShaderModule( const std::vector< char >& aShaderCode )
	{
		VkShaderModuleCreateInfo 	createInfo{  };
		VkShaderModule 			shaderModule;
		createInfo.sType 	= VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		createInfo.codeSize 	= aShaderCode.size(  );
		createInfo.pCode 	= ( const uint32_t* )aShaderCode.data(  );
		
		CheckVKResult( vkCreateShaderModule( GetDevice(), &createInfo, NULL, &shaderModule ), "Failed to create shader module!" );
		
		return shaderModule;
	}
	
	void CreateGraphicsPipeline()
	{
		VkPipelineLayoutCreateInfo pipelineCreateInfo {
			.sType          = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
			.setLayoutCount = (u32)aLayouts.size(),
			.pSetLayouts    = aLayouts.data()
		};

		VkPushConstantRange pushConstantRange {
			.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
			.offset     = 0,
			.size       = sizeof( freeman_push_t )
		};

		pipelineCreateInfo.pushConstantRangeCount = 1;
		pipelineCreateInfo.pPushConstantRanges = &pushConstantRange;

		CheckVKResult( vkCreatePipelineLayout( GetDevice(), &pipelineCreateInfo, NULL, &aPipelineLayout ), "Failed to create pipeline layout" );

		VkPipelineShaderStageCreateInfo shaderStages[ 2 ] {
			{
				.sType 		= VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
				.stage 		= VK_SHADER_STAGE_VERTEX_BIT,
				.module 	= aModules[ 0 ],
				.pName 		= "main"
			},
			{
				.sType 		= VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
				.stage 		= VK_SHADER_STAGE_FRAGMENT_BIT,
				.module 	= aModules[ 1 ],
				.pName 		= "main"
			}
		};

		std::vector< VkVertexInputBindingDescription > bindingDescriptions;
		std::vector< VkVertexInputAttributeDescription > attributeDescriptions;

		bindingDescriptions.push_back(
			VkVertexInputBindingDescription {
				.binding 	= 0,
				.stride 	= sizeof( vertex_t ),
				.inputRate = VK_VERTEX_INPUT_RATE_VERTEX
			}
		);

		attributeDescriptions.push_back(
			VkVertexInputAttributeDescription {
				.location 	= 0,
				.binding 	= 0,
				.format 	= VK_FORMAT_R32G32B32_SFLOAT,
				.offset 	= offsetof( vertex_t, aPos )
			}
		);
		
		attributeDescriptions.push_back(
			VkVertexInputAttributeDescription {
				.location 	= 1,
				.binding 	= 0,
				.format 	= VK_FORMAT_R32G32_SFLOAT,
				.offset 	= offsetof( vertex_t, aUV )
			}
		);
		
		VkPipelineVertexInputStateCreateInfo vertexInputInfo {
			.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
			.vertexBindingDescriptionCount = (u32)bindingDescriptions.size(),
			.pVertexBindingDescriptions = bindingDescriptions.data(),
			.vertexAttributeDescriptionCount = (u32)attributeDescriptions.size(),
			.pVertexAttributeDescriptions = attributeDescriptions.data()
		};

		VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo {
			.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
			.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
			.primitiveRestartEnable = VK_FALSE
		};

		float height = (float)GetSwapchain().GetExtent().height;
		
		VkViewport viewport {
			.x = 0.0f,
			.y = height,
			.width = ( float )GetSwapchain().GetExtent().width,
			.height = height * -1.f,
			.minDepth = 0.0f,
			.maxDepth = 1.0f
		};
		
		VkRect2D scissor {
			.offset = { 0, 0 },
			.extent = GetSwapchain().GetExtent()
		};

		VkPipelineViewportStateCreateInfo viewportInfo {
			.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
			.viewportCount = 1,
			.pViewports = &viewport,
			.scissorCount = 1,
			.pScissors = &scissor
		};

		// create rasterizer
		VkPipelineRasterizationStateCreateInfo rasterizer{  };		//	Turns  primitives into fragments, aka, pixels for the framebuffer
		rasterizer.sType 			= VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		rasterizer.depthClampEnable 		= VK_FALSE;
		rasterizer.rasterizerDiscardEnable 	= VK_FALSE;
		rasterizer.polygonMode 			= VK_POLYGON_MODE_FILL;		//	Fill with fragments, can optionally use VK_POLYGON_MODE_LINE for a wireframe
		rasterizer.lineWidth 			= 1.0f;
		// rasterizer.cullMode 			= ( sFlags & NO_CULLING ) ? VK_CULL_MODE_NONE : VK_CULL_MODE_BACK_BIT;	//	FIX FOR MAKING 2D SPRITES WORK!!! WOOOOO!!!!
		// rasterizer.cullMode 			= VK_CULL_MODE_BACK_BIT;
		rasterizer.cullMode 			= VK_CULL_MODE_NONE;
		rasterizer.frontFace 			= VK_FRONT_FACE_COUNTER_CLOCKWISE;
		rasterizer.depthBiasEnable 		= VK_FALSE;
		rasterizer.depthBiasConstantFactor 	= 0.0f; // Optional
		rasterizer.depthBiasClamp 		= 0.0f; // Optional
		rasterizer.depthBiasSlopeFactor 	= 0.0f; // Optional

		VkPipelineMultisampleStateCreateInfo multisampling{  };		//	Performs anti-aliasing
		multisampling.sType 		    = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		multisampling.sampleShadingEnable   = VK_FALSE;
		multisampling.rasterizationSamples  = GetMSAASamples();
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
		pipelineInfo.pStages 			= shaderStages;
		pipelineInfo.pVertexInputState 		= &vertexInputInfo;
		pipelineInfo.pInputAssemblyState 	= &inputAssemblyInfo;
		pipelineInfo.pViewportState 		= &viewportInfo;
		pipelineInfo.pRasterizationState 	= &rasterizer;
		pipelineInfo.pMultisampleState 		= &multisampling;
		pipelineInfo.pDepthStencilState 	= &depthStencil;
		pipelineInfo.pColorBlendState 		= &colorBlending;
		pipelineInfo.pDynamicState 		= NULL; // Optional
		pipelineInfo.layout 			= aPipelineLayout;
		pipelineInfo.renderPass 		= GetRenderPass( RenderPass_Color | RenderPass_Depth | RenderPass_Resolve );
		pipelineInfo.subpass 			= 0;
		pipelineInfo.basePipelineHandle 	= VK_NULL_HANDLE; // Optional, very important for later when making new pipelines. It is less expensive to reference an existing similar pipeline
		pipelineInfo.basePipelineIndex 		= -1; // Optional

		CheckVKResult( vkCreateGraphicsPipelines( GetDevice(), VK_NULL_HANDLE, 1, &pipelineInfo, NULL, &aPipeline ), "Failed to create graphics pipeline!" );
	}
};


extern freemans_tshirt_t freemans_tshirt;
