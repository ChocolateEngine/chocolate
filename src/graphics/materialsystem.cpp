/*
materialsystem.cpp ( Authored by Demez )

Manages all the materials.
Maybe the shadersystem could be inside this material system?
*/

#include "core/filesystem.h"
#include "materialsystem.h"
#include "renderer.h"
#include "util.h"
#include "graphics/sprite.h"
#include "speedykeyv/KeyValue.h"

#include "shaders/shader_basic_3d.h"
#include "shaders/shader_basic_2d.h"
#include "shaders/shader_unlit.h"
#include "shaders/shader_debug.h"
#include "shaders/shader_unlitarray.h"

#include "textures/textureloader_ktx.h"
#include "textures/textureloader_stbi.h"
#include "textures/textureloader_ctx.h"

extern size_t gModelDrawCalls;
extern size_t gVertsDrawn;

MaterialSystem* matsys = nullptr;


MaterialSystem::MaterialSystem()
{
	matsys = this;
}


void MaterialSystem::Init()
{
	// Create Texture Loaders
	aTextureLoaders.push_back( new KTXTextureLoader );
	aTextureLoaders.push_back( new CTXTextureLoader );
	aTextureLoaders.push_back( new STBITextureLoader );

	aImageLayout = InitDescriptorSetLayout( DescriptorLayoutBinding( VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr ) );
	// aUniformLayout = InitDescriptorSetLayout( DescriptorLayoutBinding( VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, nullptr ) );

	// Create Missing Texture
	apMissingTex = CreateTexture( "" );

	// Setup built in shaders
	BaseShader* basic_3d   = CreateShader<Basic3D>( "basic_3d" );
	BaseShader* basic_2d   = CreateShader<Basic2D>( "basic_2d" );
	BaseShader* unlit      = CreateShader<ShaderUnlit>( "unlit" );
	BaseShader* debug      = CreateShader<ShaderDebug>( "debug" );
	BaseShader* unlitarray = CreateShader<ShaderUnlitArray>( "unlitarray" );
}


IMaterial* MaterialSystem::CreateMaterial()
{
	Material* mat = new Material;
	mat->Init();
	aMaterials.push_back( mat );
	return mat;
}


IMaterial* MaterialSystem::FindMaterial( const std::string &name )
{
	for ( Material *mat : aMaterials )
	{
		if ( mat->aName == name )
			return mat;
	}

	return nullptr;
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


IMaterial* MaterialSystem::ParseMaterial( const std::string &path )
{
	std::string fullPath = path;

	if ( !fullPath.ends_with( ".cmt" ) )
		fullPath += ".cmt";

	fullPath = filesys->FindFile( fullPath );

	if ( fullPath.empty() )
	{
		Print( "[Graphics] Failed to find file: %s\n", path.c_str() );
		return nullptr;
	}

	std::vector< char > rawData = filesys->ReadFile( fullPath );

	if ( rawData.empty() )
	{
		Print( "[Graphics] Failed to read file: %s\n", fullPath.c_str() );
		return nullptr;
	}

	// append a null terminator for c strings
	rawData.push_back( '\0' );

	KeyValueRoot kvRoot;
	KeyValueErrorCode err = kvRoot.Parse( rawData.data() );

	if ( err != KeyValueErrorCode::NO_ERROR )
	{
		Print( "[Graphics] Failed to parse file: %s\n", fullPath.c_str() );
		return nullptr;
	}

	// parsing time
	kvRoot.Solidify();
	
	KeyValue* kvShader = kvRoot.children;

	IMaterial *mat = CreateMaterial();
	mat->aName = std::filesystem::path( path ).filename().string();
	mat->SetShader( kvShader->key.string );

	KeyValue *kv = kvShader->children;
	for ( int i = 0; i < kvShader->childCount; i++ )
	{
		if ( kv->hasChildren )
		{
			Print( "[Graphics] Skipping extra children in kv file: %s", fullPath.c_str() );
			kv = kv->next;
			continue;
		}

		// awful and lazy
		double valFloat = 0.f;
		long valInt = 0;

		if ( ToDouble2( kv->value.string, valFloat ) )
		{
			mat->SetVar( kv->key.string, (float)valFloat );
		}
		else if ( ToLong2( kv->value.string, valInt ) )
		{
			mat->SetVar( kv->key.string, (int)valInt );
		}
		else if ( strncmp(kv->value.string, "", kv->value.length) == 0 )
		{
			// empty string case, just an int with 0 as the value, idk
			mat->SetVar( kv->key.string, 0 );
		}

		// TODO: check if a vec and parse it somehow

		// assume it's a texture?
		else
		{
			// TODO: check for ctx, png, and jpg
			std::string texPath = kv->value.string;
			if ( !texPath.ends_with( ".ktx" ) )
				texPath += ".ktx";

			std::string absTexPath = filesys->FindFile( texPath );
			if ( absTexPath == "" )
			{
				if ( !filesys->IsAbsolute( texPath ) && !texPath.starts_with( "materials" ) )
					texPath = "materials/" + texPath;

				absTexPath = filesys->FindFile( texPath );
			}

			if ( absTexPath == "" )
			{
				Print( "[Graphics] \"%s\":\n\tCan't Find Texture: \"%s\": \"%s\"\n", fullPath.c_str(), kv->key.string, kv->value.string );
				mat->SetVar( kv->key.string, CreateTexture( kv->value.string ) );
			}
			else
			{
				Texture *texture = CreateTexture( absTexPath );
				if ( texture != nullptr )
				{
					mat->SetVar( kv->key.string, texture );
				}
				else
				{
					Print( "[Graphics] \"%s\":n\tUnknown Material Var Type: \"%s\": \"%s\"\n", fullPath.c_str(), kv->key.string, kv->value.string );
					mat->SetVar( kv->key.string, 0 );
				}
			}
		}

		kv = kv->next;
	}

	return mat;
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
	mat->SetVar( "diffuse", apMissingTex );

	aErrorMaterials.push_back( mat );
	return mat;
}


Texture* MaterialSystem::GetMissingTexture()
{
	return apMissingTex;
}


Texture *MaterialSystem::CreateTexture( const std::string &path )
{
	// Check if texture was already loaded
	std::unordered_map< std::string, TextureDescriptor* >::iterator it;

	it = aTextures.find( path );

	if ( it != aTextures.end() )
		return it->second;

	// Not found, so try to load it
	std::string absPath = filesys->FindFile( path );
	if ( absPath == "" && apMissingTex )
	{
		Print( "[Graphics] Failed to Find Texture \"%s\"\n", path );
		return nullptr;
	}

	std::string fileExt = filesys->GetFileExt( path );

	for ( ITextureLoader* loader: aTextureLoaders )
	{
		if ( !loader->CheckExt( fileExt.c_str() ) )
			continue;

		if ( TextureDescriptor* texture = loader->LoadTexture( absPath ) )
		{
			aTextures[path] = texture;

			std::vector< TextureDescriptor* > v;
			v.reserve( aTextures.size() );

			for ( const auto &rTex : aTextures )
				v.push_back( rTex.second );

			UpdateImageSets( aImageSets, aImageLayout, *gpPool, v, *apSampler );

			// reassign the missing texture id due to order shifting
			aMissingTexId = GetTextureId( apMissingTex );

			return texture;
		}
	}

	// error print is handled in STBI texture loader

	return nullptr;
}


int MaterialSystem::GetTextureId( Texture *spTexture )
{
	int i = 0;
	for ( const auto& [path, tex]: aTextures )
	{
		if ( tex == spTexture )
			return i;
		++i;
	}
	return aMissingTexId;
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

	Print( "[Graphics] Shader not found: %s", name.c_str() );
	return nullptr;
}


void MaterialSystem::InitUniformBuffer( IMesh* mesh )
{
	if ( !mesh->apMaterial )
		return;

	Material* mat = (Material*)mesh->apMaterial;

	if ( !mat->apShader->UsesUniformBuffers() )
		return;

	mat->apShader->InitUniformBuffer( mesh );
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

void MaterialSystem::AddRenderable( BaseRenderableGroup *group )
{
	for ( auto renderable : group->GetRenderables() )
		aDrawList.push_back( renderable );
}


size_t MaterialSystem::GetRenderableID( BaseRenderable* renderable )
{
	return renderable->GetID();
}


void MaterialSystem::DrawRenderable( BaseRenderable* renderable, VkCommandBuffer c, uint32_t commandBufferIndex )
{
	Material* mat = (Material*)renderable->apMaterial;
	if ( !mat )
		return;
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


void MaterialSystem::DestroyRenderable( BaseRenderableGroup* group )
{
	for ( auto renderable : group->GetRenderables() )
		DestroyRenderable( renderable );
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

