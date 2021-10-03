/*
modeldata.h ( Auhored by p0lyh3dron )

Declares the modeldata, a container that
stores all data related to a model.
*/
#pragma once

#include "../core/renderer/allocator.h"
#include "../core/renderer/initializers.h"
#include "transform.h"
#include "renderertypes.h"

class IndexInfo
{
public:
	uint32_t	aIndexCount;
	uint32_t	aOffset;
};

class Mesh
{
public:
	IndexInfo 		        aShape;
        TextureDescriptor		*apTexture;
};

static std::vector< TextureDescriptor* > 	gTextures;
static std::vector< IndexInfo >			gOffsets;

class ModelData
{
	typedef std::vector< VkBuffer >			BufferSet;
	typedef std::vector< VkDeviceMemory >		MemorySet;
	typedef std::vector< VkDescriptorSet > 		DescriptorSetList;
	typedef DataBuffer< Mesh >			Meshes;
	typedef DataBuffer< VkDescriptorSetLayout >	Layouts;
public:
	VkBuffer 		aVertexBuffer;
	VkBuffer		aIndexBuffer;
	VkDeviceMemory 		aVertexBufferMem;
	VkDeviceMemory		aIndexBufferMem;
	VkDescriptorSetLayout	aTextureLayout;
        UniformDescriptor	aUniformData;
	VkDescriptorSetLayout	aUniformLayout;
	VkPipelineLayout	aPipelineLayout;
	VkPipeline		aPipeline;
	Layouts			aSetLayouts{  };

	uint32_t 		aVertexCount;
	uint32_t		aIndexCount;
	vertex_3d_t 		*apVertices;
	uint32_t 		*apIndices;
	bool 			aNoDraw = true;
	
	Transform		aTransform;
        Meshes			aMeshes{  };
	/* Allocates the model, and loads its textures etc.  */
	void		Init(  );
	/* Reinitialize data that is useless after the swapchain becomes outdated.  */
	void		Reinit(  );
	/* Binds the model data to the GPU to be rendered later.  */
	void 		Bind( VkCommandBuffer c, uint32_t i );
	/* Draws the model to the screen.  */
	void 		Draw( VkCommandBuffer c, uint32_t i );
	/* Adds a material to the model.  */
	void		AddMaterial( const std::string &srTexturePath, uint32_t sMaterialId, VkSampler sSampler );
	/* Adds a mesh to the model.  */
	void		AddMesh( const std::string &srTexturePath, uint32_t sIndexCount, uint32_t sIndexOffset, VkSampler sSampler );
	/* Adds an index group to the model which groups together indices in the same material to be used for multiple draw calls for multiple textures.  */
	void		AddIndexGroup( std::vector< uint32_t > sVec );
	/* Default the model and set limits.  */
	ModelData(  ) : apVertices( NULL ), apIndices( NULL ), aNoDraw( false ), aMeshes(  ), aSetLayouts(  ){  };
	/* Frees the memory used by objects outdated by a new swapchain state.  */
	void		FreeOldResources(  );
	/* Frees all memory used by model.  */
			~ModelData(  );
};

