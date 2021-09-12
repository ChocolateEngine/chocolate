/*
modeldata.h ( Auhored by p0lyh3dron )

Declares the modeldata, a container that
stores all data related to a model.
*/
#pragma once

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

static std::vector< TextureDescriptor* > 	gTextures;
static std::vector< IndexInfo >			gOffsets;

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
	/* Allocates the model, and loads its textures etc.  */
	void		Init( Allocator &srAllocator, const std::string &srModelPath, const std::string &srTexturePath,
			      VkDescriptorSetLayout srModelSetLayout, VkSampler srTextureSampler, VkDescriptorPool srDescPool, VkExtent2D srSwapChainExtent );
	/* Reinitialize data that is useless after the swapchain becomes outdated.  */
	void		Reinit( Allocator &srAllocator );
	/* Binds the model data to the GPU to be rendered later.  */
	void 		Bind( VkCommandBuffer c, uint32_t i );
	/* Draws the model to the screen.  */
	void 		Draw( VkCommandBuffer c, uint32_t i );
	/* Adds a material to the model.  */
	void		AddMaterial( const std::string &srTexturePath, Allocator &srAllocator, uint32_t sMaterialId, VkDescriptorPool sPool, VkSampler sSampler );
	/* Adds an index group to the model which groups together indices in the same material to be used for multiple draw calls for multiple textures.  */
	void		AddIndexGroup( std::vector< uint32_t > sVec );
	/* Default the model and set limits.  */
        		ModelData(  );
	/* Frees the memory used by objects outdated by a new swapchain state.  */
	void		FreeOldResources(  );
	/* Frees all memory used by model.  */
			~ModelData(  );
};

