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
		matsys->FreeAllBuffers( aFinalMesh );

		delete aFinalMesh;
		aFinalMesh = nullptr;
	}

	if ( !g_debug_draw )
		return;

	aFinalMesh = new PrimitiveMesh;
	// aFinalMesh->SetMaterial( 0, apMaterial );

	aMeshBuilder.Reset();
	aMeshBuilder.Start( matsys, aFinalMesh );
	aMeshBuilder.SetMaterial( apMaterial );
}

void VulkanPrimitiveMaterials::PrepareMeshForDraw()
{
	if ( !g_debug_draw || !aFinalMesh || aMeshBuilder.aSurfaces.empty() )
		return;

	aMeshBuilder.End();

	matsys->AddRenderable( aFinalMesh );

	aMeshBuilder.Reset();
}

void VulkanPrimitiveMaterials::InitLine( const glm::vec3& sX, const glm::vec3& sY, const glm::vec3& sColor )
{
	if ( !g_debug_draw || !aFinalMesh )
		return;

	aMeshBuilder.SetPos( sX );
	aMeshBuilder.SetColor( sColor );
	aMeshBuilder.NextVertex();

	aMeshBuilder.SetPos( sY );
	aMeshBuilder.SetColor( sColor );
	aMeshBuilder.NextVertex();
}

void VulkanPrimitiveMaterials::Init()
{
	apMaterial = (Material*)matsys->CreateMaterial( "debug_line" );

	ResetMesh();
}

VulkanPrimitiveMaterials::~VulkanPrimitiveMaterials()
{
	if ( aFinalMesh )
	{
		matsys->FreeAllBuffers( aFinalMesh );

		delete aFinalMesh;
		aFinalMesh = nullptr;
	} 

	matsys->DeleteMaterial( apMaterial );
	apMaterial = nullptr;
}
