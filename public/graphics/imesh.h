/*
imesh.h ( Auhored by Demez )

Declares the base Mesh classes, and ModelData, a container that
stores multiple Meshes.
*/
#pragma once

#include "types/transform.h"
#include "graphics/renderertypes.h"
#include "graphics/imaterialsystem.h"

#define MESH_USE_PUSH_CONSTANTS 0

class IMesh: public BaseRenderable
{
public:
	virtual ~IMesh() = default;

	virtual glm::mat4               GetModelMatrix(  ) = 0;

	std::vector< vertex_3d_t >		aVertices;

	// could move elsewhere, takes up too much space (12+12+4 = 28 bytes)
	//glm::vec3                       aMinSize = {};
	//glm::vec3                       aMaxSize = {};
	//float                           aRadius = 0.f;  // could just be calculated with an inline CalcRadius() function
};


// Simple Mesh Class, can be moved, rotated, and scaled
// 140 bytes - 36 bytes + 8 bytes for vtable + 96 bytes from IMesh
class Mesh: public IMesh
{
public:
	virtual ~Mesh() {}

	inline glm::mat4                GetModelMatrix(  ) override { return aTransform.ToMatrix(); }

	// takes up 36 bytes
	Transform                       aTransform;
};

class ModelAttributes
{
public:
	bool                        aNoDraw = true;
	std::vector<vertex_3d_t>    aVertices;
	std::vector<uint32_t>       aIndices;
	std::vector< Mesh* >        aMeshes{  };
	std::string                 aPath;
};

// this really should only be used for models loaded in or grouping meshes together
// IDEA: make this a template class with a base of IMesh somehow?
class ModelData
{
private:
	Transform                   aTransform;
	ModelAttributes		    aAttributes;
public:
	

	// awful, need to do this better somehow, so that meshes can be offset from this model position,
	// and the offset stay the same when changing this
	inline void SetTransform( const Transform& transform )
	{
		aTransform = transform;

		for ( auto& mesh: aAttributes.aMeshes )
			mesh->aTransform = transform;
	}

	inline void SetPos( const glm::vec3& pos )
	{
		aTransform.aPos = pos;

		for ( auto& mesh: aAttributes.aMeshes )
			mesh->aTransform.aPos = pos;
	}

	inline void SetAng( const glm::vec3& ang )
	{
		aTransform.aAng = ang;

		for ( auto& mesh: aAttributes.aMeshes )
			mesh->aTransform.aAng = ang;
	}

	inline void SetScale( const glm::vec3& scale )
	{
		aTransform.aScale = scale;

		for ( auto& mesh: aAttributes.aMeshes )
			mesh->aTransform.aScale = scale;
	}

	inline Transform& GetTransform(  )
	{
		return aTransform;
	}

	inline ModelAttributes& GetAttributes(  )
        {
	        return aAttributes;
        }

	inline void SetAttributes( ModelAttributes sAttributes )
        {
		aAttributes = sAttributes;
        }

	/* Allocates the model, and loads its textures etc.  */
	//void        Init(  );
	/* Reinitialize data that is useless after the swapchain becomes outdated.  */
	//void        ReInit(  );
	/* Bind and Draws the model to the screen.  */
	//void        Draw( VkCommandBuffer c, uint32_t i );
	/* Default the model and set limits.  */
	ModelData(  ) : aAttributes(  ){  };
	/* Frees the memory used by objects outdated by a new swapchain state.  */
	//void        FreeOldResources(  );
	/* Frees all memory used by model.  */
	//void        Destroy(  );

	~ModelData(  ) {}
};

