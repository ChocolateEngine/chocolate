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
#include "../shared/imaterialsystem.h"

#define MESH_USE_PUSH_CONSTANTS 0


// TODO: the functions in here are really for renderer use only so i might want to get rid of them soon
// 96 bytes
class IMesh: public BaseRenderable
{
public:
	virtual ~IMesh() = default;

	virtual glm::mat4               GetModelMatrix(  ) = 0;

	/* Allocates the model, and loads its textures etc.  */
	void                    Init(  );

	/* Reinitialize data that is useless after the swapchain becomes outdated.  */
	void                    ReInit(  );

	/* Frees the memory used by objects outdated by a new swapchain state.  */
	void                    FreeOldResources(  );

	/* Frees all memory used by model.  */
	void                    Destroy(  );

	// 48 bytes (24 + 24)
	std::vector< vertex_3d_t >		aVertices;
	std::vector< uint32_t >         aIndices;

	// 32 bytes
	VkBuffer                        aVertexBuffer = nullptr;
	VkBuffer                        aIndexBuffer = nullptr;
	VkDeviceMemory                  aVertexBufferMem = nullptr;
	VkDeviceMemory                  aIndexBufferMem = nullptr;

	// could move elsewhere, takes up too much space (12+12+4 = 28 bytes)
	//glm::vec3                       aMinSize = {};
	//glm::vec3                       aMaxSize = {};
	//float                           aRadius = 0.f;  // could just be calculated with an inline CalcRadius() function
};


// Simple Mesh Class, can be moved, rotated, and scaled
// XX bytes - 36 bytes + XX bytes
class Mesh: public IMesh
{
public:
	virtual ~Mesh() {}

	inline glm::mat4                GetModelMatrix(  ) override { return aTransform.ToMatrix(); }

	// takes up 36 bytes
	Transform                       aTransform;
};


// this really should only be used for models loaded in or grouping meshes together
class ModelData
{
private:
	Transform                   aTransform;

public:
	bool                        aNoDraw = true;
	std::vector<vertex_3d_t>    aVertices;
	std::vector<uint32_t>       aIndices;
	std::vector< Mesh* >        aMeshes{  };
	std::string                 aPath;

	// awful, need to do this better somehow, so that meshes can be offset from this model position,
	// and the offset stay the same when changing this
	inline void SetTransform( const Transform& transform )
	{
		aTransform = transform;

		for ( auto& mesh: aMeshes )
			mesh->aTransform = transform;
	}

	inline void SetPos( const glm::vec3& pos )
	{
		aTransform.aPos = pos;

		for ( auto& mesh: aMeshes )
			mesh->aTransform.aPos = pos;
	}

	inline void SetAng( const glm::vec3& ang )
	{
		aTransform.aAng = ang;

		for ( auto& mesh: aMeshes )
			mesh->aTransform.aAng = ang;
	}

	inline void SetScale( const glm::vec3& scale )
	{
		aTransform.aScale = scale;

		for ( auto& mesh: aMeshes )
			mesh->aTransform.aScale = scale;
	}

	inline Transform& GetTransform(  )
	{
		return aTransform;
	}

	/* Allocates the model, and loads its textures etc.  */
	void        Init(  );
	/* Reinitialize data that is useless after the swapchain becomes outdated.  */
	void        ReInit(  );
	/* Bind and Draws the model to the screen.  */
	//void        Draw( VkCommandBuffer c, uint32_t i );
	/* Default the model and set limits.  */
				ModelData(  ) : aNoDraw( false ), aMeshes(  ){  };
	/* Frees the memory used by objects outdated by a new swapchain state.  */
	void        FreeOldResources(  );
	/* Frees all memory used by model.  */
	void        Destroy(  );

				~ModelData(  ) {}
};

