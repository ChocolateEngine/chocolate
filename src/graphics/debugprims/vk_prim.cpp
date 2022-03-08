/*
 *	vk_prim.cpp
 *
 *	Authored by "p0ly_" on January 20, 2022
 *
 *	Defines the vulkan things required for debug rendering.
 *      
 *	
 */
#include "vk_prim.h"

#include "../allocator.h"

#include "core/core.h"
#include "../materialsystem.h"

CONVAR( g_debug_draw, 1 );

void VulkanPrimitiveMaterials::ResetMesh()
{
	if ( aFinalMesh )
	{
		matsys->FreeVertexBuffer( aFinalMesh );

		delete aFinalMesh;
		aFinalMesh = nullptr;
	}

	if ( !g_debug_draw )
		return;

	aFinalMesh = new PrimitiveMesh;
	aFinalMesh->apMaterial = apMaterial;
}

void VulkanPrimitiveMaterials::PrepareMeshForDraw()
{
	if ( aFinalMesh->aVertices.empty() || !g_debug_draw )
		return;

	matsys->CreateVertexBuffer( aFinalMesh );
	matsys->AddRenderable( aFinalMesh );
}

void VulkanPrimitiveMaterials::InitLine( const glm::vec3& sX, const glm::vec3& sY, const glm::vec3& sColor )
{
	if ( !g_debug_draw )
		return;

	aFinalMesh->aVertices.resize( aFinalMesh->aVertices.size() + 2 );

	size_t size = aFinalMesh->aVertices.size();
	aFinalMesh->aVertices[size-2] = {sX, sColor};
	aFinalMesh->aVertices[size-1] = {sY, sColor};
}

void VulkanPrimitiveMaterials::Init()
{
	apMaterial = (Material*)matsys->CreateMaterial();
	apMaterial->SetShader( "debug" );

	ResetMesh();
}

VulkanPrimitiveMaterials::~VulkanPrimitiveMaterials()
{
	matsys->DeleteMaterial( apMaterial );
	apMaterial = nullptr;
}
