/*
modeldata.h ( Auhored by p0lyh3dron )

Declares the modeldata, a container that
stores all data related to a model.
*/
#pragma once

#define MODEL_SET_PARAMETERS_TEMP( tImageView, uBuffers ) { { { VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, tImageView, srTextureSampler, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER } } }, { { { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, uBuffers, sizeof( ubo_3d_t ) } } }

#include "../core/renderer/allocator.h"

class ModelData
{
	typedef std::vector< VkBuffer >		BufferSet;
	typedef std::vector< VkDeviceMemory >	MemorySet;
	typedef std::vector< VkDescriptorSet > 	DescriptorSetList;
public:
	VkBuffer 		aVertexBuffer;
	VkBuffer		aIndexBuffer;
	VkDeviceMemory 		aVertexBufferMem;
	VkDeviceMemory		aIndexBufferMem;
	VkDeviceMemory		aTextureImageMem;
	VkImage 		aTextureImage;
	VkImageView 		aTextureImageView;
	BufferSet		aUniformBuffers;
	MemorySet		aUniformBuffersMem;
	VkPipelineLayout	aPipelineLayout;
	VkPipeline		aPipeline;
	DescriptorSetList 	aDescriptorSets;

	uint32_t 		aVertexCount;
	uint32_t		aIndexCount;
	vertex_3d_t 		*apVertices;
	uint32_t 		*apIndices;
	bool 			aNoDraw = false;
	glm::vec3 		aPos;
	float 			aWidth 	= 0.5f;
	float			aHeight = 0.5f;

	void		Init( Allocator &srAllocator, const std::string &srModelPath, const std::string &srTexturePath,
				VkDescriptorSetLayout srModelSetLayout, VkSampler srTextureSampler, VkDescriptorPool srDescPool, VkExtent2D srSwapChainExtent )
	{
		srAllocator.InitTextureImage( srTexturePath, aTextureImage, aTextureImageMem, NULL, NULL );
		srAllocator.InitTextureImageView( aTextureImageView, aTextureImage );
		srAllocator.InitUniformBuffers( aUniformBuffers, aUniformBuffersMem );
		srAllocator.InitDescriptorSets( aDescriptorSets, srModelSetLayout, srDescPool, MODEL_SET_PARAMETERS_TEMP( aTextureImageView, aUniformBuffers ) );
			aPipelineLayout = srAllocator.InitPipelineLayouts( &srModelSetLayout, 1 );
		srAllocator.InitGraphicsPipeline< vertex_3d_t >( aPipeline, aPipelineLayout, srModelSetLayout, "materials/shaders/3dvert.spv", "materials/shaders/3dfrag.spv", 0 );
	}
	/* Binds the model data to the GPU to be rendered later.  */
	void 		Bind( VkCommandBuffer c, uint32_t i )
		{
			VkBuffer 	vBuffers[  ] 	= { aVertexBuffer };
			VkDeviceSize 	offsets[  ] 	= { 0 };
			vkCmdBindPipeline( c, VK_PIPELINE_BIND_POINT_GRAPHICS, aPipeline );
			vkCmdBindVertexBuffers( c, 0, 1, vBuffers, offsets );
			vkCmdBindIndexBuffer( c, aIndexBuffer, 0, VK_INDEX_TYPE_UINT32 );
			vkCmdBindDescriptorSets( c, VK_PIPELINE_BIND_POINT_GRAPHICS, aPipelineLayout, 0, 1, &aDescriptorSets[ i ], 0, NULL );
		}
	/* Draws the model to the screen.  */
	void 		Draw( VkCommandBuffer c ){ vkCmdDrawIndexed( c, aIndexCount, 1, 0, 0, 0 ); }
};

