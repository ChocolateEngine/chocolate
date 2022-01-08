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

#ifdef KTX
#include "ktx/ktx.h"
#include "ktx/ktxvulkan.h"
#endif

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
	TextureDescriptor	*pTexture = new TextureDescriptor;

	if ( !LoadKTXTexture( pTexture, path, pTexture->aTextureImage, pTexture->aTextureImageMem, pTexture->aMipLevels ) )
	{
		// fallback to stbi
		InitTextureImage( path, pTexture->aTextureImage, pTexture->aTextureImageMem, pTexture->aMipLevels );
		InitTextureImageView( pTexture->aTextureImageView, pTexture->aTextureImage, pTexture->aMipLevels );
	}

	VkDescriptorSetLayout layout = ((Material*)material)->GetTextureLayout();

	InitDescriptorSets(
		pTexture->aSets,
		layout,
		*gpPool,
		{ { {
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			pTexture->aTextureImageView,
			renderer->aTextureSampler,
			VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER
		} } },
		{  }
	);

	return pTexture;
}


bool MaterialSystem::LoadKTXTexture( TextureDescriptor* pTexture, const String &srImagePath, VkImage &srTImage, VkDeviceMemory &srTImageMem, uint32_t &srMipLevels )
{
#ifndef KTX
	return false;
#else
	int 		texWidth;
	int		texHeight;
	int		texChannels;
	VkBuffer 	stagingBuffer;
	VkDeviceMemory 	stagingBufferMemory;
	bool		noTexture = false;

	ktxVulkanDeviceInfo vdi;

	KTX_error_code result = ktxVulkanDeviceInfo_Construct(&vdi, gpDevice->GetPhysicalDevice(), DEVICE,
														  gpDevice->GetGraphicsQueue(), gpDevice->GetCommandPool(), nullptr);

	if ( result != KTX_SUCCESS )
	{
		Print( "KTX Error %d: %s - Failed to Construct KTX Vulkan Device\n", result, ktxErrorString(result) );
		return false;
	}

	result = ktxTexture_CreateFromNamedFile(srImagePath.c_str(), KTX_TEXTURE_CREATE_NO_FLAGS, &pTexture->kTexture);

	if ( result != KTX_SUCCESS )
	{
		Print( "KTX Error %d: %s - Failed to open texture: %s\n", result, ktxErrorString(result), srImagePath.c_str(  ) );
		ktxVulkanDeviceInfo_Destruct( &vdi );
		return false;
	}

	result = ktxTexture_VkUploadEx(pTexture->kTexture, &vdi, &pTexture->texture,
								   VK_IMAGE_TILING_OPTIMAL,
								   VK_IMAGE_USAGE_SAMPLED_BIT,
								   VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

	if ( result != KTX_SUCCESS )
	{
		Print( "KTX Error %d: %s - Failed to upload texture: %s\n", result, ktxErrorString(result), srImagePath.c_str(  ) );
		ktxTexture_Destroy( pTexture->kTexture );
		ktxVulkanDeviceInfo_Destruct( &vdi );
		return false;
	}

	Print( "Loaded Image: %s - dataSize: %d\n", srImagePath.c_str(  ), pTexture->kTexture->dataSize );

	ktxTexture_Destroy( pTexture->kTexture );
	ktxVulkanDeviceInfo_Destruct( &vdi );

	pTexture->aMipLevels = pTexture->kTexture->numLevels;
	pTexture->aTextureImage = pTexture->texture.image;
	pTexture->aTextureImageMem = pTexture->texture.deviceMemory;

	InitImageView( pTexture->aTextureImageView, pTexture->aTextureImage, pTexture->texture.imageFormat, VK_IMAGE_ASPECT_COLOR_BIT, pTexture->aMipLevels );

	return true;
#endif
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

