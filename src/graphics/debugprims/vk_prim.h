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

#include "graphics/imesh.h"
#include "graphics/primvtx.hh"
#include "graphics/renderertypes.h"

#include "../types/material.h"

class PrimitiveMesh: public IMesh
{
public:
	virtual glm::mat4               GetModelMatrix() override { return glm::mat4( 1.f ); }
};

class VulkanPrimitiveMaterials {
	using Sets       = std::vector< VkDescriptorSet >;
protected:
	PrimitiveMesh*        aFinalMesh = nullptr;
	Material*             apMaterial = nullptr;

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
