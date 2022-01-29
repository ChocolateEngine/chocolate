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

#include "core/filesystem.h"
#include "core/console.h"
#include "../materialsystem.h"

#include "public/util.h"

void VulkanPrimitiveMaterials::ResetMesh()
{
	if ( aFinalMesh )
	{
		matsys->FreeVertexBuffer( aFinalMesh );

		delete aFinalMesh;
		aFinalMesh = nullptr;
	}

	aFinalMesh = new PrimitiveMesh;
	aFinalMesh->apMaterial = apMaterial;
}

void VulkanPrimitiveMaterials::PrepareMeshForDraw()
{
	if ( aFinalMesh->aVertices.empty() )
		return;

	matsys->CreateVertexBuffer( aFinalMesh );
	matsys->AddRenderable( aFinalMesh );
}

void VulkanPrimitiveMaterials::InitLine( glm::vec3 sX, glm::vec3 sY, glm::vec3 sColor )
{
	aFinalMesh->aVertices.push_back({sX, sColor});
	aFinalMesh->aVertices.push_back({sY, sColor});
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
