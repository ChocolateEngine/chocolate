/*
materialsystem.cpp ( Authored by Demez )

Manages all the materials.
Maybe the shadersystem could be inside this material system?
*/

#include "materialsystem.h"
#include "renderer.h"
#include "util.h"
#include "graphics/sprite.h"

#include "shaders/shader_basic_3d.h"
#include "shaders/shader_basic_2d.h"


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
	BaseShader* basic_2d = CreateShader<Basic2D>( "basic_2d" );

	// Create Error Material
	// TODO: move to GetErrorMaterial and make it have a shader name as an input
	/*apErrorMaterial = (Material*)CreateMaterial();
	apErrorMaterial->aName = "ERROR";
	apErrorMaterial->apShader = basic_3d;
	apErrorMaterial->aDiffusePath = "";
	apErrorMaterial->apDiffuse = CreateTexture( apErrorMaterial, "" );*/
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


// stupid, we really should just have one error material with a shader that can do both 2d and 3d (wonder if basic_3d can already do that...)
IMaterial* MaterialSystem::GetErrorMaterial( const std::string& shaderName )
{
	for (auto& matPublic: aErrorMaterials)
	{
		Material* mat = (Material*)matPublic;

		if ( mat->apShader->aName == shaderName )
			return matPublic;
	}

	auto shader = GetShader( shaderName );
	assert( shader != nullptr );

	// Create a new error material with this shader (why??)
	Material* mat = (Material*)CreateMaterial();
	mat->aName = "ERROR_" + shaderName;
	mat->apShader = shader;
	mat->aDiffusePath = "";
	mat->apDiffuse = CreateTexture( mat, "" );

	aErrorMaterials.push_back( mat );
	return mat;
}


// this really should not have a Material input, it should have a Texture class input, right? idk
TextureDescriptor* MaterialSystem::CreateTexture( IMaterial* material, const std::string path )
{
	return InitTexture( path, ((Material*)material)->GetTextureLayout(), *gpPool, renderer->aTextureSampler );
}


// TODO: i need to make BaseRenderable a template for the vertex type which i don't feel like doing right now so
void MaterialSystem::CreateVertexBuffer( BaseRenderable* itemBase )
{
	if ( IMesh* item = dynamic_cast<IMesh*>(itemBase) )
		InitTexBuffer( item->aVertices, item->aVertexBuffer, item->aVertexBufferMem, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT );
	
	else if ( Sprite* item = dynamic_cast<Sprite*>(itemBase) )
		InitTexBuffer( item->aVertices, item->aVertexBuffer, item->aVertexBufferMem, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT );

	// just fucking exit
	else throw std::runtime_error( "[MaterialSystem::CreateVertexBuffer] oh god oh fuck how the fuck do i design this properly\n  JUST USE IMesh or Sprite AAAAA\n" );
}


void MaterialSystem::CreateIndexBuffer( BaseRenderable* item )
{
	if ( item->aIndices.size() > 0 )
		InitTexBuffer( item->aIndices, item->aIndexBuffer, item->aIndexBufferMem, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT );
}


void MaterialSystem::FreeVertexBuffer( BaseRenderable* item )
{
	vkDestroyBuffer( DEVICE, item->aVertexBuffer, NULL );
	vkFreeMemory( DEVICE, item->aVertexBufferMem, NULL );

	item->aVertexBuffer = nullptr;
	item->aVertexBufferMem = nullptr;
}


void MaterialSystem::FreeIndexBuffer( BaseRenderable* item )
{
	vkDestroyBuffer( DEVICE, item->aIndexBuffer, NULL );
	vkFreeMemory( DEVICE, item->aIndexBufferMem, NULL );

	item->aIndexBuffer = nullptr;
	item->aIndexBufferMem = nullptr;
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
	aRenderables.push_back( renderable );
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
	Material* mat = (Material*)renderable->apMaterial;
	mat->apShader->Draw( renderable, c, commandBufferIndex );
}


void MaterialSystem::DestroyRenderable( BaseRenderable* renderable )
{
	// blech
	if ( IMesh* mesh = dynamic_cast<IMesh*>( renderable ) )
		MeshFreeOldResources( mesh );

	FreeVertexBuffer( renderable );

	if ( renderable->aIndexBuffer )
		FreeIndexBuffer( renderable );
}


// TODO: move these to Basic3D
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

