/*
materialsystem.cpp ( Authored by Demez )

Manages all the materials.
Maybe the shadersystem could be inside this material system?
*/

#include "materialsystem.h"
#include "renderer.h"
#include "graphics/sprite.h"
#include "graphics.h"
#include "speedykeyv/KeyValue.h"

#include "shaders/shader_basic_2d.h"
#include "shaders/shader_unlit.h"
#include "shaders/shader_unlitarray.h"
#include "shaders/shader_skybox.h"

extern size_t gModelDrawCalls;
extern size_t gVertsDrawn;

MaterialSystem* matsys = nullptr;


void AddTextureLoader( ITextureLoader* loader )
{
	GetMaterialSystem()->aTextureLoaders.push_back( loader );
}


MaterialSystem* GetMaterialSystem()
{
	static bool init = false;
	if ( !init )
	{
		matsys = new MaterialSystem;
		init = true;
	}

	return matsys;
}


MaterialSystem::MaterialSystem()
{
	matsys = this;
}


void MaterialSystem::Init()
{
	aImageLayout = InitDescriptorSetLayout( DescriptorLayoutBinding( VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr ) );
	// aUniformLayout = InitDescriptorSetLayout( DescriptorLayoutBinding( VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, nullptr ) );

	AllocImageSets( aImageSets, aImageLayout, *gpPool );

	// Create Missing Texture
	apMissingTex = CreateTexture( "" );

	// init shaders
	for ( auto& [name, shader]: aShaders )
	{
		shader->Init();
	}

	// Setup built in shaders
	BaseShader* basic_2d   = CreateShader<Basic2D>( "basic_2d" );
	BaseShader* unlit      = CreateShader<ShaderUnlit>( "unlit" );
	BaseShader* unlitarray = CreateShader<ShaderUnlitArray>( "unlitarray" );
	BaseShader* skybox     = CreateShader<ShaderSkybox>( "skybox" );
}


bool MaterialSystem::AddShader( BaseShader* spShader, const std::string& name )
{
	// check if we already made this shader
	auto search = aShaders.find( name );

	if ( search != aShaders.end() )
		return false;

	spShader->aName = name;

	if ( gpDevice )
		spShader->Init();

	aShaders[name] = spShader;

	return true;
}


IMaterial* MaterialSystem::CreateMaterial()
{
	Material* mat = new Material;
	mat->Init();
	aMaterials.push_back( mat );
	return mat;
}


IMaterial* MaterialSystem::CreateMaterial( const std::string& srShader )
{
	Material* mat = new Material;
	mat->Init();
	mat->SetShader( srShader );
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

	auto it = std::find( aMaterials.begin(), aMaterials.end(), matPublic );
	if ( it != aMaterials.end() )
	{
		aMaterials.erase( it );
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
		LogWarn( gGraphicsChannel, "Failed to find material: %s\n", path.c_str() );
		return nullptr;
	}

	std::vector< char > rawData = filesys->ReadFile( fullPath );

	if ( rawData.empty() )
	{
		LogWarn( gGraphicsChannel, "Failed to read material: %s\n", fullPath.c_str() );
		return nullptr;
	}

	// append a null terminator for c strings
	rawData.push_back( '\0' );

	KeyValueRoot kvRoot;
	KeyValueErrorCode err = kvRoot.Parse( rawData.data() );

	if ( err != KeyValueErrorCode::NO_ERROR )
	{
		LogWarn( gGraphicsChannel, "Failed to parse material: %s\n", fullPath.c_str() );
		return nullptr;
	}

	// parsing time
	kvRoot.Solidify();
	
	KeyValue* kvShader = kvRoot.children;

	IMaterial *mat = CreateMaterial();
	mat->aName = filesys->GetFileNameNoExt( path );
	// mat->aName = std::filesystem::path( path ).filename().string();
	mat->SetShader( kvShader->key.string );

	KeyValue *kv = kvShader->children;
	for ( int i = 0; i < kvShader->childCount; i++ )
	{
		if ( kv->hasChildren )
		{
			LogDev( gGraphicsChannel, 1, "Skipping extra children in material: %s", fullPath.c_str() );
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
		else if ( strnlen(kv->value.string, kv->value.length) == 0 )
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
				LogMsg( gGraphicsChannel, "\"%s\":\n\tCan't Find Texture: \"%s\": \"%s\"\n", fullPath.c_str(), kv->key.string, kv->value.string );
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
					LogMsg( gGraphicsChannel, "\"%s\":n\tUnknown Material Var Type: \"%s\": \"%s\"\n", fullPath.c_str(), kv->key.string, kv->value.string );
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

	it = aTexturePaths.find( path );

	if ( it != aTexturePaths.end() )
		return it->second;

	// Not found, so try to load it
	std::string absPath = filesys->FindFile( path );
	if ( absPath == "" && apMissingTex )
	{
		LogWarn( gGraphicsChannel, "Failed to Find Texture \"%s\"\n", path.c_str() );
		return nullptr;
	}

	std::string fileExt = filesys->GetFileExt( path );

	for ( ITextureLoader* loader: aTextureLoaders )
	{
		if ( !loader->CheckExt( fileExt.c_str() ) )
			continue;

		if ( TextureDescriptor* texture = loader->LoadTexture( absPath ) )
		{
			texture->aId = aTextures.size();
			aTextures.push_back( texture );
			aTexturePaths[path] = texture;

			UpdateImageSets( aImageSets, aTextures, *apSampler );

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
	return vec_index( aTextures, spTexture, aMissingTexId );
}


// TODO: i need to make BaseRenderable a template for the vertex type which i don't feel like doing right now so
void MaterialSystem::CreateVertexBuffer( IRenderable* renderable )
{
	if ( HasBufferInternal( renderable, gVertexBufferFlags ) )
	{
		LogWarn( gGraphicsChannel, "Vertex Buffer already exists for renderable\n" );
		Assert( false );
		return;
	}

	// create a new buffer
	RenderableBuffer* buffer = new RenderableBuffer;
	buffer->aFlags = gVertexBufferFlags;

	InitRenderableBuffer(
		renderable->GetVertexData(),
		renderable->GetVertexFormatSize() * renderable->GetTotalVertexCount(),
		buffer->aBuffer,
		buffer->aBufferMem,
		VK_BUFFER_USAGE_TRANSFER_DST_BIT | buffer->aFlags
	);

	// store this buffer
	aRenderBuffers[renderable->GetID()].push_back( buffer );
}


void MaterialSystem::CreateIndexBuffer( IRenderable* renderable )
{
	if ( HasBufferInternal( renderable, gIndexBufferFlags ) )
	{
		LogWarn( gGraphicsChannel, "Vertex Buffer already exists for renderable\n" );
		Assert( false );
		return;
	}

	// create a new buffer
	RenderableBuffer* buffer = new RenderableBuffer;
	buffer->aFlags = gIndexBufferFlags;

	if ( renderable->GetIndices().size() > 0 )
	{
		InitRenderableBuffer(
			renderable->GetIndices().data(),
			sizeof( uint32_t ) * renderable->GetIndices().size(),
			buffer->aBuffer,
			buffer->aBufferMem,
			VK_BUFFER_USAGE_TRANSFER_DST_BIT | buffer->aFlags
		);
	}

	// store this buffer
	aRenderBuffers[renderable->GetID()].push_back( buffer );
}


bool MaterialSystem::HasVertexBuffer( IRenderable* item )
{
	return (HasBufferInternal( item, gVertexBufferFlags ));
}


bool MaterialSystem::HasIndexBuffer( IRenderable* item )
{
	return (HasBufferInternal( item, gIndexBufferFlags ));
}


void MaterialSystem::FreeVertexBuffer( IRenderable* item )
{
	auto it = aRenderBuffers.find( item->GetID() );

	if ( it == aRenderBuffers.end() )
		return;

	for ( size_t i = 0; i < it->second.size(); i++ )
	{
		RenderableBuffer* buffer = it->second[i];
		if ( buffer->aFlags & gVertexBufferFlags )
		{
			vkDestroyBuffer( DEVICE, buffer->aBuffer, NULL );
			vkFreeMemory( DEVICE, buffer->aBufferMem, NULL );

			vec_remove_index( it->second, i );

			delete buffer;
			return;
		}
	}
}


void MaterialSystem::FreeIndexBuffer( IRenderable* item )
{
	auto it = aRenderBuffers.find( item->GetID() );

	if ( it == aRenderBuffers.end() )
		return;

	for ( size_t i = 0; i < it->second.size(); i++ )
	{
		RenderableBuffer* buffer = it->second[i];
		if ( buffer->aFlags & gIndexBufferFlags )
		{
			vkDestroyBuffer( DEVICE, buffer->aBuffer, NULL );
			vkFreeMemory( DEVICE, buffer->aBufferMem, NULL );

			vec_remove_index( it->second, i );

			delete buffer;
			return;
		}
	}
}


void MaterialSystem::FreeAllBuffers( IRenderable* item )
{
	auto it = aRenderBuffers.find( item->GetID() );

	if ( it == aRenderBuffers.end() )
		return;

	for ( size_t i = 0; i < it->second.size(); i++ )
	{
		RenderableBuffer* buffer = it->second[i];

		vkDestroyBuffer( DEVICE, buffer->aBuffer, NULL );
		vkFreeMemory( DEVICE, buffer->aBufferMem, NULL );

		delete buffer;
	}

	aRenderBuffers.erase( it );
}


static std::vector< RenderableBuffer* > gRenderBufferEmpty;

/* Get Vector of Buffers for a Renderable. */
const std::vector< RenderableBuffer* >& MaterialSystem::GetRenderBuffers( IRenderable* renderable )
{
	auto it = aRenderBuffers.find( renderable->GetID() );

	if ( it != aRenderBuffers.end() )
		return it->second;

	return gRenderBufferEmpty;
}


// Does a buffer exist with these flags for this renderable already?
bool MaterialSystem::HasBufferInternal( IRenderable* renderable, VkBufferUsageFlags flags )
{
	auto it = aRenderBuffers.find( renderable->GetID() );

	if ( it == aRenderBuffers.end() )
		return false;

	for ( RenderableBuffer* buffer : it->second )
	{
		if ( buffer->aFlags & flags )
			return true;
	}

	return false;
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

	LogError( gGraphicsChannel, "Shader not found: %s\n", name.c_str() );
	return nullptr;
}


void MaterialSystem::InitUniformBuffer( IRenderable* model )
{
	// AWFUL
	// check if any material needs a uniform buffer

	for ( size_t i = 0; i < model->GetMaterialCount(); i++ )
	{
		if ( !model->GetMaterial( i ) )
			continue;

		Material* mat = (Material*)model->GetMaterial( i );

		if ( !mat->apShader->UsesUniformBuffers() )
			continue;

		mat->apShader->InitUniformBuffer( model );
	}
}


void MaterialSystem::RegisterRenderable( IRenderable* renderable )
{
	// eh
	//static size_t id = 0;
	//renderable->aId = id++;
	aRenderables.push_back( renderable );
}


void MaterialSystem::AddRenderable( IRenderable* renderable )
{
	if ( !renderable )
		return;

	for ( size_t i = 0; i < renderable->GetMaterialCount(); i++ )
	{
		Material* mat = (Material*)renderable->GetMaterial( i );

		if ( !mat )
		{
			LogError( gGraphicsChannel, "Renderable part \"%d\" has no material!\n", i );
			continue;
		}

		if ( !mat->apShader )
		{
			LogError( gGraphicsChannel, "Material has no shader!\n" );
			continue;
		}

		// auto search = aDrawList.find( mat->apShader );
		// 
		// if ( search != aDrawList.end() )
		// 	search->second.push_back( renderable );
		// else
		// 	aDrawList[mat->apShader].push_back( renderable );

		// aDrawList[mat->apShader].push_front( renderable );
		aDrawList[mat->apShader].emplace_front( renderable, i, RenderableDrawData() );
	}
}


void MaterialSystem::AddRenderable( IRenderable* renderable, const RenderableDrawData& srDrawData )
{
	if ( !renderable )
		return;

	for ( size_t i = 0; i < renderable->GetMaterialCount(); i++ )
	{
		Material* mat = (Material*)renderable->GetMaterial( i );

		if ( !mat )
		{
			LogError( gGraphicsChannel, "Renderable part \"%d\" has no material!\n", i );
			continue;
		}

		if ( !mat->apShader )
		{
			LogError( gGraphicsChannel, "Material has no shader!\n" );
			continue;
		}

		// auto search = aDrawList.find( mat->apShader );
		// 
		// if ( search != aDrawList.end() )
		// 	search->second.push_back( renderable );
		// else
		// 	aDrawList[mat->apShader].push_back( renderable );

		// aDrawList[mat->apShader].push_front( renderable );
		aDrawList[mat->apShader].emplace_front( renderable, i, srDrawData );
	}
}

#if 0
constexpr void MaterialSystem::AddRenderable( BaseRenderableGroup *group )
{
	uint32_t size = group->GetRenderableCount();

	for ( uint32_t i = 0; i < size; i++ )
		AddRenderable( group->GetRenderable( i ) );
}
#endif


void MaterialSystem::BindRenderBuffers( IRenderable* renderable, VkCommandBuffer c, uint32_t cIndex )
{
	const std::vector< RenderableBuffer* >& renderBuffers = matsys->GetRenderBuffers( renderable );

	// Bind the mesh's vertex and index buffers
	for ( RenderableBuffer* buffer : renderBuffers )
	{
		if ( buffer->aFlags & gVertexBufferFlags )
		{
			VkBuffer        vBuffers[]  = { buffer->aBuffer };
			VkDeviceSize	offsets[]   = { 0 };

			vkCmdBindVertexBuffers( c, 0, 1, vBuffers, offsets );
		}
		else if ( buffer->aFlags & gIndexBufferFlags )
		{
			vkCmdBindIndexBuffer( c, buffer->aBuffer, 0, VK_INDEX_TYPE_UINT32 );
		}
	}
}


void MaterialSystem::DrawRenderable( size_t memIndex, IRenderable* renderable, size_t matIndex, const RenderableDrawData& srDrawData, VkCommandBuffer c, uint32_t commandBufferIndex )
{
	Material* mat = (Material*)renderable->GetMaterial( matIndex );
	if ( !mat )
		return;

	// mat->apShader->BindBuffers( renderable, c, commandBufferIndex );
	mat->apShader->Draw( memIndex, renderable, matIndex, srDrawData, c, commandBufferIndex );
}


void MaterialSystem::DestroyRenderable( IRenderable* renderable )
{
	MeshFreeOldResources( renderable );
	FreeAllBuffers( renderable );
}




// TODO: move these to Basic3D
void MaterialSystem::MeshInit( Model* mesh )
{
	InitUniformBuffer( mesh );
}

void MaterialSystem::MeshReInit( Model* mesh )
{
	InitUniformBuffer( mesh );
}

void MaterialSystem::MeshFreeOldResources( IRenderable* mesh )
{
	auto it = aUniformMap.find( mesh->GetID() );

	if ( it == aUniformMap.end() )
		return;

	for ( auto& uniform : it->second )
	{
		UniformDescriptor& uniformdata = uniform.aDesc;

		for ( uint32_t i = 0; i < uniformdata.aSets.GetSize(  ); ++i )
		{
			/* Free uniform data.  */
			vkDestroyBuffer( DEVICE, uniformdata.aData[i], NULL );
			vkFreeMemory( DEVICE, uniformdata.aMem[i], NULL );

			/* Free uniform layout. */
			vkDestroyDescriptorSetLayout( DEVICE, uniform.aLayout, NULL );
		}
	}

	aUniformMap.erase( mesh->GetID() );
}


VkFormat MaterialSystem::ToVkFormat( ColorFormat colorFmt )
{
	switch ( colorFmt )
	{
		case ColorFormat::INVALID:
		default:
			LogError( gGraphicsChannel, "Unspecified Color Format to VkFormat Conversion!\n" );
			return VK_FORMAT_UNDEFINED;

		case ColorFormat::R32G32B32_SFLOAT:
			return VK_FORMAT_R32G32B32_SFLOAT;

		case ColorFormat::R32G32_SFLOAT:
			return VK_FORMAT_R32G32_SFLOAT;
	}
}


VkFormat MaterialSystem::GetVertexElementVkFormat( VertexElement element )
{
	return ToVkFormat( GetVertexElementFormat( element ) );
}


ColorFormat MaterialSystem::GetVertexElementFormat( VertexElement element )
{
	switch ( element )
	{
		default:
			LogError( gGraphicsChannel, "GetVertexElementFormat: Invalid VertexElement specified: %d\n", element );
			return ColorFormat::INVALID;

		case VertexElement_Position:
			return ColorFormat::R32G32B32_SFLOAT;

		// NOTE: could be smaller probably
		case VertexElement_Normal:
			return ColorFormat::R32G32B32_SFLOAT;

		case VertexElement_Color:
			return ColorFormat::R32G32B32_SFLOAT;

		case VertexElement_TexCoord:
			return ColorFormat::R32G32_SFLOAT;

		case VertexElement_Position2D:
			return ColorFormat::R32G32_SFLOAT;
	}
}


size_t MaterialSystem::GetVertexElementSize( VertexElement element )
{
	switch ( element )
	{
		default:
			LogError( gGraphicsChannel, "GetVertexElementSize: Invalid VertexElement specified: %d\n", element );
			return 0;

		case VertexElement_Position:
			return ( 3 * sizeof( float ) );

		case VertexElement_Normal:
			return ( 3 * sizeof( float ) );

		case VertexElement_Color:
			return ( 3 * sizeof( float ) );

		case VertexElement_TexCoord:
			return ( 2 * sizeof( float ) );

		case VertexElement_Position2D:
			return ( 2 * sizeof( float ) );
	}
}

