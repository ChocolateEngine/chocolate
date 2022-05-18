/*
 *	vk_prim.h
 *
 *	Authored by "p0ly_" on January 20, 2022
 *
 *	Declares the vulkan things required for debug rendering.
 *      
 *	
 */
#pragma once

#include "graphics/primvtx.hh"
#include "graphics/meshbuilder.hpp"
#include "graphics/renderertypes.h"

#include "../types/material.h"

#if 0
class PrimitiveMesh: public IRenderable
{
public:
	IMaterial*                       apMaterial = nullptr;

	std::vector< vertex_debug_t >    aVertices;
	std::vector< uint32_t >          aIndices;

	// --------------------------------------------------------------------------------------
	// Materials

	virtual size_t     GetMaterialCount() override                          { return 1; }
	virtual IMaterial* GetMaterial( size_t i = 0 ) override                 { return apMaterial; }
	virtual void       SetMaterial( size_t i, IMaterial* mat ) override     { apMaterial = mat; }

	virtual void SetShader( size_t i, const std::string& name ) override
	{
		if ( apMaterial )
			apMaterial->SetShader( name );
	}

	// --------------------------------------------------------------------------------------
	// Drawing Info

	virtual u32 GetVertexOffset( size_t material ) override     { return 0; }
	virtual u32 GetVertexCount( size_t material ) override      { return aVertices.size(); }

	virtual u32 GetIndexOffset( size_t material ) override      { return 0; }
	virtual u32 GetIndexCount( size_t material ) override       { return aIndices.size(); }

	// --------------------------------------------------------------------------------------

	virtual VertexFormatFlags                   GetVertexFormatFlags() override     { return g_vertex_flags_debug; }
	virtual size_t                              GetVertexFormatSize() override      { return sizeof( vertex_debug_t ); }
	virtual void*                               GetVertexData() override            { return aVertices.data(); };
	virtual size_t                              GetTotalVertexCount() override      { return aVertices.size(); };

	virtual std::vector< vertex_debug_t >&      GetVertices()                       { return aVertices; };
	virtual std::vector< uint32_t >&            GetIndices() override               { return aIndices; };
};
#else
// lazy
using PrimitiveMesh = Model;
#endif


class VulkanPrimitiveMaterials {
	using Sets       = std::vector< VkDescriptorSet >;
protected:
	PrimitiveMesh*        aFinalMesh = nullptr;
	Material*             apMaterial = nullptr;

	MeshBuilder           aMeshBuilder;

public:
	/* Create one large primative for one draw call  */
	void   ResetMesh();
	/* Creates Vertex Buffer */
	void   PrepareMeshForDraw();
	/* Creates a new line.  */
	void   InitLine( const glm::vec3& sX, const glm::vec3& sY, const glm::vec3& sColor );
	/* Initializes the general stuff required for rendering.  */
	void   Init();
	      ~VulkanPrimitiveMaterials();
};
