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
#include "../core/renderer/material.h"


/* This is broken right now and idk why, so it's disabled for now. */
#define MESH_UNIFORM_DATA 0


class Mesh
{
public:
	/* Allocates the model, and loads its textures etc.  */
	void                            Init(  );

	/* Reinitialize data that is useless after the swapchain becomes outdated.  */
	void                            ReInit(  );

	inline BaseShader*              GetShader() const           { return apMaterial->apShader; }

#if MESH_UNIFORM_DATA
	UniformDescriptor               aUniformData;
	VkDescriptorSetLayout           aUniformLayout = nullptr;

	inline UniformDescriptor&       GetUniformData()            { return aUniformData; }
	inline VkDescriptorSetLayout    GetUniformLayout()          { return aUniformLayout; }
#endif

	std::vector< vertex_3d_t >		aVertices;
	std::vector< uint32_t >         aIndices;

	VkBuffer                        aVertexBuffer;
	VkBuffer                        aIndexBuffer;
	VkDeviceMemory                  aVertexBufferMem;
	VkDeviceMemory                  aIndexBufferMem;

	Material*                       apMaterial = nullptr;
	glm::vec3                       aMinSize = {};
	glm::vec3                       aMaxSize = {};
	float                           aRadius = 0.f;
};


class ModelData
{
	typedef std::vector< VkBuffer >			BufferSet;
	typedef std::vector< VkDeviceMemory >		MemorySet;
	typedef std::vector< VkDescriptorSet > 		DescriptorSetList;
	typedef DataBuffer< Mesh >			Meshes;
	typedef DataBuffer< VkDescriptorSetLayout >	Layouts;
public:
	uint32_t        aVertexCount;
	uint32_t        aIndexCount;
	vertex_3d_t     *apVertices;
	uint32_t        *apIndices;

	bool 			aNoDraw = true;
	Transform		aTransform;
	Meshes			aMeshes{  };

#if MESH_UNIFORM_DATA
	inline UniformDescriptor&       GetUniformData( size_t mesh )        { return aMeshes[mesh].GetUniformData(); }
	inline VkDescriptorSetLayout    GetUniformLayout( size_t mesh )      { return aMeshes[mesh].GetUniformLayout(); }
#else
	// blech, move this later
	UniformDescriptor               aUniformData;
	VkDescriptorSetLayout           aUniformLayout = nullptr;

	inline UniformDescriptor&       GetUniformData( size_t mesh )        { return aUniformData; }
	inline VkDescriptorSetLayout    GetUniformLayout( size_t mesh )      { return aUniformLayout; }
#endif

	/* Allocates the model, and loads its textures etc.  */
	void        Init(  );
	/* Reinitialize data that is useless after the swapchain becomes outdated.  */
	void        ReInit(  );
	/* Binds the model data to the GPU to be rendered later.  */
	void        Bind( VkCommandBuffer c, uint32_t i );
	/* Draws the model to the screen.  */
	void        Draw( VkCommandBuffer c, uint32_t i );
	/* Default the model and set limits.  */
				ModelData(  ) : apVertices( NULL ), apIndices( NULL ), aNoDraw( false ), aMeshes(  ){  };
	/* Frees the memory used by objects outdated by a new swapchain state.  */
	void        FreeOldResources(  );
	/* Frees all memory used by model.  */
			~ModelData(  );
};

