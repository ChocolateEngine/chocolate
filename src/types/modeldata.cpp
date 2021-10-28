/*
modeldata.cpp ( Authored by p0lyh3dron )

Defines the ModelData class defined in
modeldata.h.  
*/

#include "../../inc/types/modeldata.h"
#include "../core/renderer/materialsystem.h"

extern size_t gModelDrawCalls;
extern MaterialSystem* materialsystem;
extern Renderer* renderer;

/* Allocates the model's uniform data and layout.  */
void IMesh::Init()
{
	materialsystem->InitUniformBuffer( this );
}

void IMesh::ReInit()
{
	materialsystem->InitUniformBuffer( this );
}

void IMesh::FreeOldResources()
{
	UniformDescriptor& uniformdata = materialsystem->GetUniformData( GetID() );

	for ( uint32_t i = 0; i < uniformdata.aSets.GetSize(  ); ++i )
	{
		/* Free uniform data.  */
		vkDestroyBuffer( DEVICE, uniformdata.aData[i], NULL );
		vkFreeMemory( DEVICE, uniformdata.aMem[i], NULL );

		/* Free uniform layout. */
		vkDestroyDescriptorSetLayout( DEVICE, materialsystem->GetUniformLayout( GetID() ), NULL );
	}
}

void IMesh::Destroy()
{
	FreeOldResources();

	materialsystem->FreeVertexBuffer( this );

	if ( aIndexBuffer )
		materialsystem->FreeIndexBuffer( this );
}


// =============================================================

void ModelData::Init()
{
	for ( uint32_t j = 0; j < aMeshes.size(  ); ++j )
		aMeshes[j]->Init();
}

void ModelData::ReInit()
{
	for ( uint32_t j = 0; j < aMeshes.size(  ); ++j )
		aMeshes[j]->ReInit();
}


/* Default the model and set limits.  */
/* Frees the memory used by objects outdated by a new swapchain state.  */
void ModelData::FreeOldResources(  )
{
	/* Destroy Vulkan objects, keeping the allocations for their storage units.  */

	/* Ugly, fix later.  */
	for ( uint32_t mesh = 0; mesh < aMeshes.size(  ); ++mesh )
	{
		aMeshes[mesh]->FreeOldResources(  );
	}
}


/* Frees all memory used by model.  */
void ModelData::Destroy(  )
{
	for ( uint32_t i = 0; i < aMeshes.size(); i++ )
		aMeshes[i]->Destroy(  );
}

