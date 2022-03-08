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


class Model;


class Mesh : public IMesh
{
public:
	virtual ~Mesh() {}

	inline glm::mat4                GetModelMatrix() override;

	Model* apModel;
};


// Simple Model Class, can be moved, rotated, and scaled
class Model : public BaseRenderableGroup
{
private:

public:
	std::vector<vertex_3d_t>    aVertices;
	std::vector<uint32_t>       aIndices;

	std::vector< Mesh* >        aMeshes{  };
	std::string                 aPath;

	Transform                   aTransform;

	constexpr inline uint32_t GetRenderableCount() override
	{
		return aMeshes.size();
	}

	constexpr inline BaseRenderable* GetRenderable( uint32_t index ) override
	{
		return aMeshes[index];
	}

	constexpr inline void SetTransform( const Transform& transform )
	{
		aTransform = transform;
	}

	constexpr inline void SetPos( const glm::vec3& pos )
	{
		aTransform.aPos = pos;
	}

	constexpr inline void SetAng( const glm::vec3& ang )
	{
		aTransform.aAng = ang;
	}

	constexpr inline void SetScale( const glm::vec3& scale )
	{
		aTransform.aScale = scale;
	}

	constexpr inline Transform& GetTransform(  )
	{
		return aTransform;
	}

	Model(  ) : aMeshes(  ){  };
	~Model(  )
	{
		for ( auto mesh : aMeshes )
		{
			delete mesh;
		}
	}
};


inline glm::mat4 Mesh::GetModelMatrix()
{
	return apModel->aTransform.ToMatrix();
}

