/*
materialsystem.cpp ( Authored by Demez )

Manages all the materials.
Maybe the shadersystem could be inside this material system?
*/

#include "materialsystem.h"
#include "renderer.h"
#include "util.h"

#include "shaders/shader_basic_3d.h"


extern size_t gModelDrawCalls;
extern size_t gVertsDrawn;

MaterialSystem* materialsystem = nullptr;


MaterialSystem::MaterialSystem()
{
	materialsystem = this;
}


void MaterialSystem::Init()
{
	// Setup built in shaders
	BaseShader* basic_3d = CreateShader<Basic3D>( "basic_3d" );

	// Create Error Material
	apErrorMaterial = (Material*)CreateMaterial();
	apErrorMaterial->aName = "ERROR";
	apErrorMaterial->apShader = basic_3d;
	apErrorMaterial->aDiffusePath = "";
	apErrorMaterial->apDiffuse = CreateTexture( apErrorMaterial, "" );
}


IMaterial* MaterialSystem::CreateMaterial()
{
	Material* mat = new Material;
	mat->Init();
	aMaterials.push_back( mat );
	return mat;
}


void MaterialSystem::DeleteMaterial( IMaterial* matPublic )
{
	Material* mat = (Material*)matPublic;
	if ( vec_contains( aMaterials, mat ) )
	{
		vec_remove( aMaterials, mat );
		mat->Destroy();
		delete mat;
	}
}


// this really should not have a Material input, it should have a Texture class input, right? idk
TextureDescriptor* MaterialSystem::CreateTexture( IMaterial* material, const std::string path )
{
	return InitTexture( path, ((Material*)material)->GetTextureLayout(), *gpPool, renderer->aTextureSampler );
}


void MaterialSystem::CreateVertexBuffer( IMesh* mesh )
{
	InitTexBuffer( mesh->aVertices, mesh->aVertexBuffer, mesh->aVertexBufferMem, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT );
}


void MaterialSystem::CreateIndexBuffer( IMesh* mesh )
{
	if ( mesh->aIndices.size() > 0 )
		InitTexBuffer( mesh->aIndices, mesh->aIndexBuffer, mesh->aIndexBufferMem, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT );
}


void MaterialSystem::FreeVertexBuffer( IMesh* mesh )
{
	vkDestroyBuffer( DEVICE, mesh->aVertexBuffer, NULL );
	vkFreeMemory( DEVICE, mesh->aVertexBufferMem, NULL );

	mesh->aVertexBuffer = nullptr;
	mesh->aVertexBufferMem = nullptr;
}


void MaterialSystem::FreeIndexBuffer( IMesh* mesh )
{
	vkDestroyBuffer( DEVICE, mesh->aIndexBuffer, NULL );
	vkFreeMemory( DEVICE, mesh->aIndexBufferMem, NULL );

	mesh->aIndexBuffer = nullptr;
	mesh->aIndexBufferMem = nullptr;
}


void MaterialSystem::ReInitSwapChain()
{
	for ( const auto& [shaderName, shader]: aShaders )
	{
		shader->ReInit();
	}
}


void MaterialSystem::DestroySwapChain()
{
	for ( const auto& [shaderName, shader]: aShaders )
	{
		shader->Destroy();
	}
}


BaseShader* MaterialSystem::GetShader( const std::string& name )
{
	auto search = aShaders.find( name );

	if (search != aShaders.end())
		return search->second;

	return nullptr;
}


void MaterialSystem::InitUniformBuffer( IMesh* mesh )
{
	aUniformLayoutMap[ mesh->GetID() ] = InitDescriptorSetLayout( { { DescriptorLayoutBinding( VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT, NULL ) } } );

	aUniformDataMap[ mesh->GetID() ] = UniformDescriptor{};

	InitUniformData( aUniformDataMap[ mesh->GetID() ], aUniformLayoutMap[ mesh->GetID() ] );
}


void MaterialSystem::RegisterRenderable( BaseRenderable* renderable )
{
	// eh
	//static size_t id = 0;
	//renderable->aId = id++;
}


void MaterialSystem::AddRenderable( BaseRenderable* renderable )
{
	aDrawList.push_back( renderable );
}


size_t MaterialSystem::GetRenderableID( BaseRenderable* renderable )
{
	return renderable->GetID();
}


void MaterialSystem::DrawRenderable( BaseRenderable* renderable, VkCommandBuffer c, uint32_t commandBufferIndex )
{
	// certified bruh moment
	if ( IMesh* mesh = dynamic_cast<IMesh*>(renderable) )
	{
		DrawMesh( mesh, c, commandBufferIndex );
	}
}


// TODO: move to Basic3D Shader!!!!
void MaterialSystem::DrawMesh( IMesh* mesh, VkCommandBuffer c, uint32_t commandBufferIndex )
{
	//if ( mesh->aNoDraw )
	//	return;

#if MESH_USE_PUSH_CONSTANTS
	// mesh.GetShader()->UpdateUniformBuffers( imageIndex, *model, model->aMeshes[i] );

	// AWFUL
#endif

	// Bind the mesh's vertex and index buffers
	VkBuffer 	vBuffers[  ] 	= { mesh->aVertexBuffer };
	VkDeviceSize 	offsets[  ] 	= { 0 };
	vkCmdBindVertexBuffers( c, 0, 1, vBuffers, offsets );

	if ( mesh->aIndexBuffer )
		vkCmdBindIndexBuffer( c, mesh->aIndexBuffer, 0, VK_INDEX_TYPE_UINT32 );

	// BLECH, need to move this whole draw function later
	BaseShader* shader = GetShader( mesh->apMaterial->GetShaderName() );

	// Now draw it
	vkCmdBindPipeline( c, VK_PIPELINE_BIND_POINT_GRAPHICS, shader->GetPipeline(  ) );

	VkDescriptorSet sets[  ] = {
		((Material*)mesh->apMaterial)->apDiffuse->aSets[ commandBufferIndex ],
		materialsystem->GetUniformData( mesh->GetID() ).aSets[ commandBufferIndex ]
	};

	vkCmdBindDescriptorSets( c, VK_PIPELINE_BIND_POINT_GRAPHICS, shader->GetPipelineLayout(  ), 0, 2, sets, 0, NULL );

	if ( mesh->aIndexBuffer )
		vkCmdDrawIndexed( c, (uint32_t)mesh->aIndices.size(), 1, 0, 0, 0 );
	else
		vkCmdDraw( c, (uint32_t)mesh->aVertices.size(), 1, 0, 0 );

	gModelDrawCalls++;
	gVertsDrawn += mesh->aVertices.size();
}


void MaterialSystem::MeshInit( IMesh* mesh )
{
	InitUniformBuffer( mesh );
}

void MaterialSystem::MeshReInit( IMesh* mesh )
{
	InitUniformBuffer( mesh );
}

void MaterialSystem::MeshFreeOldResources( IMesh* mesh )
{
	UniformDescriptor& uniformdata = GetUniformData( mesh->GetID() );

	for ( uint32_t i = 0; i < uniformdata.aSets.GetSize(  ); ++i )
	{
		/* Free uniform data.  */
		vkDestroyBuffer( DEVICE, uniformdata.aData[i], NULL );
		vkFreeMemory( DEVICE, uniformdata.aMem[i], NULL );

		/* Free uniform layout. */
		vkDestroyDescriptorSetLayout( DEVICE, GetUniformLayout( mesh->GetID() ), NULL );
	}
}

void MaterialSystem::MeshDestroy( IMesh* mesh )
{
	MeshFreeOldResources( mesh );

	FreeVertexBuffer( mesh );

	if ( mesh->aIndexBuffer )
		FreeIndexBuffer( mesh );
}

