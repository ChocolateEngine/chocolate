/*
shader_skybox.cpp ( Authored by Demez )

3D Skybox Shader
*/
#include "../renderer.h"
#include "shader_skybox.h"

#include <glm/gtx/transform.hpp>


extern size_t gModelDrawCalls;
extern size_t gVertsDrawn;


// =========================================================


struct SkyboxPushConst
{
	alignas( 16 )glm::mat4 trans;
	alignas( 16 )int       sky;
};


constexpr const char *pVShader = "shaders/skybox.vert.spv";
constexpr const char *pFShader = "shaders/skybox.frag.spv";


void ShaderSkybox::Init()
{
	aModules.Allocate(2);
	aModules[0] = CreateShaderModule( filesys->ReadFile( pVShader ) ); // Processes incoming verticies, taking world position, color, and texture coordinates as an input
	aModules[1] = CreateShaderModule( filesys->ReadFile( pFShader ) ); // Fills verticies with fragments to produce color, and depth

	BaseShader::Init();
}


void ShaderSkybox::ReInit()
{
	aModules[0] = CreateShaderModule( filesys->ReadFile( pVShader ) ); // Processes incoming verticies, taking world position, color, and texture coordinates as an input
	aModules[1] = CreateShaderModule( filesys->ReadFile( pFShader ) ); // Fills verticies with fragments to produce color, and depth

	BaseShader::ReInit();
}


void ShaderSkybox::CreateGraphicsPipeline(  )
{
	aPipelineLayout = InitPipelineLayouts( aLayouts.GetBuffer(  ), aLayouts.GetSize(  ), sizeof( SkyboxPushConst ) );

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

#if 0
	VkPipelineVertexInputStateCreateInfo vertexInputInfo{  };	//	Format of vertex data
	vertexInputInfo.sType 				= VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertexInputInfo.vertexBindingDescriptionCount 	= 0;
	vertexInputInfo.vertexAttributeDescriptionCount = 0;
#else
	auto attributeDescriptions 	= vertex_cube_3d_t::GetAttributeDesc(  );
	auto bindingDescription 	= vertex_cube_3d_t::GetBindingDesc(  );      

	VkPipelineVertexInputStateCreateInfo vertexInputInfo{  };	//	Format of vertex data
	vertexInputInfo.sType 				= VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertexInputInfo.vertexBindingDescriptionCount 	= 1;
	vertexInputInfo.pVertexBindingDescriptions 	= &bindingDescription;			//	Contains details for loading vertex data
	vertexInputInfo.vertexAttributeDescriptionCount = ( uint32_t )( attributeDescriptions.size(  ) );
	vertexInputInfo.pVertexAttributeDescriptions 	= attributeDescriptions.data(  );	//	Same as above
#endif

	VkPipelineInputAssemblyStateCreateInfo inputAssembly{  };	//	Collects raw vertex data from buffers
	inputAssembly.sType 			= VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssembly.topology 			= VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	inputAssembly.primitiveRestartEnable 	= VK_FALSE;

	/* Region of frambuffer to be rendered to, likely will always use 0, 0 and width, height  */
	// SKYBOX SHADER HACK: use min and max depth of 1 to put it all the way back
	VkViewport viewport = Viewport( 0.f, 0.f, ( float )PSWAPCHAIN.GetExtent(  ).width, ( float )PSWAPCHAIN.GetExtent(  ).height, 0.999f, 1.f );
	// VkViewport viewport = Viewport( 0.f, 0.f, ( float )PSWAPCHAIN.GetExtent(  ).width, ( float )PSWAPCHAIN.GetExtent(  ).height, 0.1f, 1.f );

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
	multisampling.rasterizationSamples  = gpDevice->GetSamples();
	multisampling.minSampleShading 	    = 1.0f; // Optional
	multisampling.pSampleMask 	    = NULL; // Optional
	multisampling.alphaToCoverageEnable = VK_FALSE; // Optional
	multisampling.alphaToOneEnable 	    = VK_FALSE; // Optional

	VkPipelineColorBlendAttachmentState colorBlendAttachment{  };
	colorBlendAttachment.colorWriteMask 	 = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT;
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
	// depthStencil.depthCompareOp 		= VK_COMPARE_OP_NOT_EQUAL;
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


void ShaderSkybox::UpdateBuffers( uint32_t sCurrentImage, BaseRenderable* spRenderable )
{
}


// KTX fails to load this and i don't feel like figuring out why right now
constexpr const char* gFallback = "materials/base/black.png";


#define VIEWMAT_ANG( axis ) glm::vec3(viewMatrix[0][axis], viewMatrix[1][axis], viewMatrix[2][axis])

/* Y Up version of the ViewMatrix */
inline void ToViewMatrixY( glm::mat4& viewMatrix, const glm::vec3& ang )
{
	/* Y Rotation - YAW (Mouse X for Y up) */
	viewMatrix = glm::rotate( viewMatrix, glm::radians(ang[YAW]), vec_right );

	/* X Rotation - PITCH (Mouse Y) */
	viewMatrix = glm::rotate( viewMatrix, glm::radians(ang[PITCH]), VIEWMAT_ANG(0) );

	/* Z Rotation - ROLL */
	viewMatrix = glm::rotate( viewMatrix, glm::radians(ang[ROLL]), VIEWMAT_ANG(2) );
}


void ShaderSkybox::Draw( BaseRenderable* renderable, VkCommandBuffer c, uint32_t commandBufferIndex )
{
	// why did i do this, remove this ASAP
	ISkyboxMesh* mesh = static_cast<ISkyboxMesh*>(renderable);

	assert(mesh != nullptr);

	// Bind the mesh's vertex and index buffers
	VkBuffer 	vBuffers[  ] 	= { mesh->GetVertexBuffer()};
	VkDeviceSize 	offsets[  ] 	= { 0 };
	vkCmdBindVertexBuffers( c, 0, 1, vBuffers, offsets );

	VkBuffer indexBuffer = mesh->GetIndexBuffer();
	if ( indexBuffer )
		vkCmdBindIndexBuffer( c, indexBuffer, 0, VK_INDEX_TYPE_UINT32 );

	Material* mat = (Material*)mesh->GetMaterial();

	// TODO: add GetMat4 to MaterialVar types
	glm::mat4 viewMat( 1.f );
	ToViewMatrixY( viewMat, mat->GetVec3( "ang" ) );

	SkyboxPushConst p = {
		renderer->aView.projMatrix * viewMat,
		mat->GetTextureId( "sky" )
	};

	vkCmdPushConstants(
		c, aPipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
		0, sizeof( SkyboxPushConst ), &p
	);

	VkDescriptorSet sets[  ] = {
		matsys->aImageSets[ commandBufferIndex ],
	};

	vkCmdBindDescriptorSets( c, VK_PIPELINE_BIND_POINT_GRAPHICS, aPipelineLayout, 0, 1, sets, 0, NULL );

	if ( indexBuffer )
		vkCmdDrawIndexed( c, (uint32_t)mesh->GetIndices().size(), 1, 0, 0, 0 );
	else
		vkCmdDraw( c, (uint32_t)mesh->GetVertices().size(), 1, 0, 0 );

	gModelDrawCalls++;
	gVertsDrawn += mesh->GetVertices().size();
}
