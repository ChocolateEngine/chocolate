/*
imesh.h ( Auhored by Demez )

Declares the base Mesh classes, and ModelData, a container that
stores multiple Meshes.
*/
#pragma once

#include "types/transform.h"
#include "graphics/renderertypes.h"
#include "graphics/imaterialsystem.h"


// NOTE: WE NEED A REF COUNTER OR A SMART POINTER ON THIS SO WE KNOW WHEN TO FREE IT
class IMesh: public BaseRenderable
{
public:
	virtual ~IMesh() = default;

	virtual size_t                  GetID(  ) override { return (size_t)this; }

	virtual glm::mat4               GetModelMatrix() { return glm::mat4( 1.f ); };

	virtual VkBuffer&               GetVertexBuffer() override { return aVertexBuffer; };
	virtual VkBuffer&               GetIndexBuffer() override { return aIndexBuffer; };

	virtual VkDeviceMemory&         GetVertexBufferMem() override { return aVertexBufferMem; };
	virtual VkDeviceMemory&         GetIndexBufferMem() override { return aIndexBufferMem; };

	virtual IMaterial*              GetMaterial() override { return apMaterial; };
	virtual void                    SetMaterial( IMaterial* mat ) override { apMaterial = mat; };

	virtual inline void             SetShader( const char* name ) override { if ( apMaterial ) apMaterial->SetShader( name ); };

	virtual std::vector< vertex_3d_t >& GetVertices() { return aVertices; };
	virtual std::vector< uint32_t >& GetIndices() { return aIndices; };

private:
	IMaterial*                      apMaterial = nullptr;

	// Set the shader for the material by the shader name

	// TODO: remove this from BaseRenderable, really shouldn't have this,
	// gonna have to have some base types for this so game can use this
	// or we just store this in graphics like how we store uniform buffers
	VkBuffer                        aVertexBuffer = nullptr;
	VkBuffer                        aIndexBuffer = nullptr;
	VkDeviceMemory                  aVertexBufferMem = nullptr;
	VkDeviceMemory                  aIndexBufferMem = nullptr;

	std::vector< vertex_3d_t >		aVertices;
	std::vector< uint32_t >         aIndices;

	// could move elsewhere, takes up too much space (12+12+4 = 28 bytes)
	//glm::vec3                       aMinSize = {};
	//glm::vec3                       aMaxSize = {};
	//float                           aRadius = 0.f;  // could just be calculated with an inline CalcRadius() function
};


class Model;


class MeshPtr: public IMesh
{
public:
	virtual ~MeshPtr() = default;

	virtual size_t                  GetID(  ) override { return (size_t)this; }

	virtual glm::mat4               GetModelMatrix() override;

	virtual VkBuffer&               GetVertexBuffer() override { return apMesh->GetVertexBuffer(); };
	virtual VkBuffer&               GetIndexBuffer() override { return apMesh->GetIndexBuffer(); };

	virtual VkDeviceMemory&         GetVertexBufferMem() override { return apMesh->GetVertexBufferMem(); };
	virtual VkDeviceMemory&         GetIndexBufferMem() override { return apMesh->GetIndexBufferMem(); };

	virtual IMaterial*              GetMaterial() override { return apMesh->GetMaterial(); };
	virtual inline void             SetShader( const char* name ) override { apMesh->SetShader( name ); };

	virtual std::vector< vertex_3d_t >& GetVertices() { return apMesh->GetVertices(); };
	virtual std::vector< uint32_t >& GetIndices() { return apMesh->GetIndices(); };

	IMesh* apMesh;  // true mesh data is stored in here
	Model* apModel;

	// could move elsewhere, takes up too much space (12+12+4 = 28 bytes)
	//glm::vec3                       aMinSize = {};
	//glm::vec3                       aMaxSize = {};
	//float                           aRadius = 0.f;  // could just be calculated with an inline CalcRadius() function
};


class Mesh : public IMesh
{
public:
	virtual ~Mesh() {}

	virtual glm::mat4 GetModelMatrix() override;

	IMesh* apMesh;  // true mesh data is stored in here
	Model* apModel;
};


// Simple Model Class, can be moved, rotated, and scaled
class Model : public BaseRenderableGroup
{
private:

public:
	std::vector<vertex_3d_t>    aVertices;
	std::vector<uint32_t>       aIndices;

	std::vector< MeshPtr* >     aMeshes{  };
	std::string                 aPath;

	Transform                   aTransform;

	inline uint32_t GetRenderableCount() override
	{
		return aMeshes.size();
	}

	inline BaseRenderable* GetRenderable( uint32_t index ) override
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


inline glm::mat4 MeshPtr::GetModelMatrix()
{
	return apModel->aTransform.ToMatrix();
}

