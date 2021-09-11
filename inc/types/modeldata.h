/*
modeldata.h ( Auhored by p0lyh3dron )

Declares the modeldata, a container that
stores all data related to a model.
*/
#pragma once

#define MODEL_SET_PARAMETERS_TEMP( tImageView, uBuffers ) { { { VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, tImageView, srTextureSampler, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER } } }, { { { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, uBuffers, sizeof( ubo_3d_t ) } } }
#define MODEL_SET_LAYOUT_PARAMETERS_TEMP { { DescriptorLayoutBinding( VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT, NULL ), DescriptorLayoutBinding( VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, NULL ) } }

#include "../core/renderer/allocator.h"
#include "../core/renderer/initializers.h"
#include "renderertypes.h"

class IndexInfo
{
public:
	uint32_t	aIndexCount;
	uint32_t	aOffset;
	uint32_t	aMaterialId;
};

static std::vector< TextureDescriptor > gTextures;
static std::vector< IndexInfo >		gOffsets;
static std::vector< uint32_t >		gDontDrawTheseTexturesPls;

class ModelData
{
	typedef std::vector< VkBuffer >			BufferSet;
	typedef std::vector< VkDeviceMemory >		MemorySet;
	typedef std::vector< VkDescriptorSet > 		DescriptorSetList;
public:
	VkBuffer 		aVertexBuffer;
	VkBuffer		aIndexBuffer;
	VkDeviceMemory 		aVertexBufferMem;
	VkDeviceMemory		aIndexBufferMem;
        TextureDescriptor       *apTextures;
	uint32_t		aTextureCount = 0;
	VkDescriptorSetLayout	aTextureLayout;
        UniformDescriptor	aUniformData;
	VkDescriptorSetLayout	aUniformLayout;
	VkPipelineLayout	aPipelineLayout;
	VkPipeline		aPipeline;

	uint32_t 		aVertexCount;
	uint32_t		aIndexCount;
	vertex_3d_t 		*apVertices;
	uint32_t 		*apIndices;
	bool 			aNoDraw = true;
	glm::vec3 		aPos;
	float 			aWidth 	= 0.5f;
	float			aHeight = 0.5f;

	void		Init( Allocator &srAllocator, const std::string &srModelPath, const std::string &srTexturePath,
				VkDescriptorSetLayout srModelSetLayout, VkSampler srTextureSampler, VkDescriptorPool srDescPool, VkExtent2D srSwapChainExtent )
	{
		aNoDraw = true;
		aTextureCount = 0;
		//apTextures = new TextureDescriptor;
		aTextureLayout = srAllocator.InitDescriptorSetLayout( { { DescriptorLayoutBinding( VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1,
												   VK_SHADER_STAGE_FRAGMENT_BIT, NULL ) } } );
		aUniformLayout = srAllocator.InitDescriptorSetLayout( { { DescriptorLayoutBinding( VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1,
												   VK_SHADER_STAGE_VERTEX_BIT, NULL ) } } );
		//apTextures[ 0 ] = srAllocator.InitTexture( srTexturePath, aTextureLayout, srDescPool, srTextureSampler );
		srAllocator.InitUniformBuffers( aUniformData.apUniformBuffers, aUniformData.apUniformBuffersMem );
		srAllocator.InitDescriptorSets( aUniformData.apDescriptorSets, aUniformLayout, srDescPool, {  }, { { { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, aUniformData.apUniformBuffers, sizeof( ubo_3d_t ) } } } );
		VkDescriptorSetLayout layouts[  ] = { aTextureLayout, aUniformLayout };
	        aPipelineLayout = srAllocator.InitPipelineLayouts( layouts, 2 );
		srAllocator.InitGraphicsPipeline< vertex_3d_t >( aPipeline, aPipelineLayout, aUniformLayout, "materials/shaders/3dvert.spv",
								 "materials/shaders/3dfrag.spv", 0 );
	}
	/* Binds the model data to the GPU to be rendered later.  */
	void 		Bind( VkCommandBuffer c, uint32_t i )
		{
			if ( aTextureCount ) aNoDraw = false;
			if ( !aNoDraw )
			{
				VkBuffer 	vBuffers[  ] 	= { aVertexBuffer };
				VkDeviceSize 	offsets[  ] 	= { 0 };
			        VkDescriptorSet sets[  ] = { gTextures[ 0 ].apDescriptorSets[ i ], aUniformData.apDescriptorSets[ i ] };
				vkCmdBindPipeline( c, VK_PIPELINE_BIND_POINT_GRAPHICS, aPipeline );
				vkCmdBindVertexBuffers( c, 0, 1, vBuffers, offsets );
				vkCmdBindIndexBuffer( c, aIndexBuffer, 0, VK_INDEX_TYPE_UINT32 );
				vkCmdBindDescriptorSets( c, VK_PIPELINE_BIND_POINT_GRAPHICS, aPipelineLayout, 0, 2, sets, 0, NULL );
			}
		}
	/* Draws the model to the screen.  */
	void 		Draw( VkCommandBuffer c, uint32_t i )
		{
			if ( !aNoDraw )
			{
				for ( auto&& index : gOffsets )
				{
					/*uint32_t j;
					bool shouldDraw = true;
					for ( j = 0; j < gTextures.size(  ) && gTextures[ j ].aMaterialId != index.aMaterialId; ++j );
					for ( auto&& noDraw : gDontDrawTheseTexturesPls )
						if ( j == noDraw )
						        shouldDraw = false; break;
					if ( shouldDraw )
					{
						VkDescriptorSet sets[  ] = { gTextures[ j ].apDescriptorSets[ i ], aUniformData.apDescriptorSets[ i ] };
						vkCmdBindDescriptorSets( c, VK_PIPELINE_BIND_POINT_GRAPHICS, aPipelineLayout, 0, 2, sets, 0, NULL );
						vkCmdDrawIndexed( c, index.aIndexCount, 1, index.aOffset, 0, 0 );
						}*/
					if ( gTextures[ index.aMaterialId ].apDescriptorSets )
					{
						VkDescriptorSet sets[  ] = { gTextures[ index.aMaterialId ].apDescriptorSets[ i ], aUniformData.apDescriptorSets[ i ] };
						vkCmdBindDescriptorSets( c, VK_PIPELINE_BIND_POINT_GRAPHICS, aPipelineLayout, 0, 2, sets, 0, NULL );
						vkCmdDrawIndexed( c, index.aIndexCount, 1, index.aOffset, 0, 0 );
					}
				}
			}
		}
	void		AddMaterial( const std::string &srTexturePath, Allocator &srAllocator, uint32_t sMaterialId, VkDescriptorPool sPool, VkSampler sSampler )
		{
			if ( srTexturePath == "" )
			{
				/*gDontDrawTheseTexturesPls.push_back( sMaterialId );
				  return;*/
				gTextures.push_back( srAllocator.InitTexture( "materials/act_like_a_baka.jpg", aTextureLayout, sPool, sSampler ) );
				++aTextureCount;
				return;
			}
			++aTextureCount;
		        /*auto pTextures = new TextureDescriptor[ ++aTextureCount ];
			memcpy( pTextures, apTextures, ( aTextureCount - 1 ) * sizeof( TextureDescriptor ) );
			delete[  ] apTextures;
			apTextures = pTextures;*/
			gTextures.push_back( srAllocator.InitTexture( srTexturePath, aTextureLayout, sPool, sSampler ) );
			gTextures[ aTextureCount ].aMaterialId = sMaterialId;
			//apTextures[ aTextureCount ] = srAllocator.InitTexture( srTexturePath, aTextureLayout, sPool, sSampler );
		}
	void		AddIndexGroup( std::vector< uint32_t > sVec )
		{
			uint32_t search;
			uint32_t numIndices;
		        for ( uint32_t i = 0; i < sVec.size(  ); )
			{
				search  = sVec[ i ];
				for ( numIndices = 0; i < sVec.size(  ) && search == sVec[ i ]; ++i, ++numIndices );
				//++i; ++numIndices;
				gOffsets.push_back( { numIndices, i - numIndices, sVec[ i - 1 ] } );
				printf( "%i indices, at offset %i, using material id %i\n", numIndices, i - numIndices, sVec[ i - 1 ] );
			}
		}
};

