/*
materialsystem.cpp ( Authored by Demez )

Manages all the materials.
Maybe the shadersystem could be inside this material system?
*/

#include "core/renderer/materialsystem.h"
#include "core/renderer/renderer.h"
#include "shared/util.h"


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
	apErrorMaterial = CreateMaterial();
	apErrorMaterial->aName = "ERROR";
	apErrorMaterial->apShader = basic_3d;
	apErrorMaterial->aDiffusePath = "";
	apErrorMaterial->apDiffuse = CreateTexture( "", apErrorMaterial->apShader );
}


Material* MaterialSystem::CreateMaterial()
{
	Material* mat = new Material;
	aMaterials.push_back( mat );
	return mat;
}


void MaterialSystem::DeleteMaterial( Material* mat )
{
	if ( vec_contains( aMaterials, mat ) )
	{
		vec_remove( aMaterials, mat );
		delete mat;
	}
}


TextureDescriptor* MaterialSystem::CreateTexture( const std::string path, BaseShader* shader )
{
	return InitTexture( path, shader->GetTextureLayout(), *gpPool, apRenderer->aTextureSampler );
}


void MaterialSystem::CreateVertexBuffer( Mesh* mesh )
{
	InitTexBuffer( mesh->aVertices, mesh->aVertexBuffer, mesh->aVertexBufferMem, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT );
}


void MaterialSystem::CreateIndexBuffer( Mesh* mesh )
{
	if ( mesh->aIndices.size() > 0 )
		InitTexBuffer( mesh->aIndices, mesh->aIndexBuffer, mesh->aIndexBufferMem, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT );
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

