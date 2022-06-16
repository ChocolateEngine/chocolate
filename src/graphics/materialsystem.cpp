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
#include "gutil.hh"

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
	VkDescriptorSetLayoutBinding imageBinding{};
	imageBinding.descriptorCount = 1000;
	imageBinding.descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	imageBinding.stageFlags      = VK_SHADER_STAGE_FRAGMENT_BIT;

	aImageLayout = InitDescriptorSetLayout( imageBinding );
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
	// BaseShader* basic_2d   = CreateShader<Basic2D>( "basic_2d" );
	// BaseShader* unlit      = CreateShader<ShaderUnlit>( "unlit" );
	// BaseShader* unlitarray = CreateShader<ShaderUnlitArray>( "unlitarray" );
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


// BufferSize is sizeof(element) * count
void InitRenderableBuffer( void* srData, size_t sBufferSize, VkBuffer &srBuffer, VkDeviceMemory &srBufferMem, VkBufferUsageFlags sUsage )
{
	VkBuffer        stagingBuffer;
	VkDeviceMemory  stagingBufferMemory;
	VkDeviceSize    bufferSize = sBufferSize;

	if ( !bufferSize )
	{
		srBuffer    = 0;
		srBufferMem = 0;

		LogError( "Tried to create a vertex buffer / index buffer with no size!\n" );
		return;
	}

	InitBuffer( bufferSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory );

	MapMemory( stagingBufferMemory, bufferSize, srData );

	InitBuffer( bufferSize, sUsage, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, srBuffer, srBufferMem );
	CopyBuffer( stagingBuffer, srBuffer, bufferSize );

	vkDestroyBuffer( DEVICE, stagingBuffer, NULL );
	vkFreeMemory( DEVICE, stagingBufferMemory, NULL );
}


// ---------------------------------------------------------------------------------------


void MaterialSystem::CreateVertexBufferInt( IModel* renderable, size_t surface, InternalMeshData_t& meshData )
{
	Assert( meshData.apVertexBuffer == nullptr );

	if ( meshData.apVertexBuffer )
	{
		LogWarn( gGraphicsChannel, "Vertex Buffer already exists for renderable surface %d\n", surface );
		return;
	}

	// create a new buffer
	meshData.apVertexBuffer = new VertexBuffer;
	meshData.apVertexBuffer->aFlags = gVertexBufferFlags;

	VertexData_t& vertData = renderable->GetSurfaceVertexData( surface );

	// Get Attributes the shader wants
	// TODO: what about if we don't have an attribute the shader wants???
	// maybe create a temporary empty buffer full of zeros? idk
	std::vector< VertAttribData_t* > attribs;

	VertexFormat shaderFormat = renderable->GetMaterial( surface )->GetVertexFormat();

	// wtf
	if ( shaderFormat == VertexFormat_None )
		return;

	for ( size_t j = 0; j < vertData.aData.size(); j++ )
	{
		VertAttribData_t& data = vertData.aData[j];
		
		if ( shaderFormat & (1 << data.aAttrib) )
			attribs.push_back( &data );
	}

	meshData.apVertexBuffer->aBufferMem.resize( attribs.size() );
	meshData.apVertexBuffer->aBuffers.resize( attribs.size() );
	meshData.apVertexBuffer->aOffsets.resize( attribs.size() );

	for ( size_t j = 0; j < attribs.size(); j++ )
	{
		auto& data = attribs[j];

		InitRenderableBuffer(
			data->apData,
			// GetVertexAttributeTypeSize( data->aAttrib ) * vertData.aCount,
			GetVertexAttributeSize( data->aAttrib ) * vertData.aCount,
			meshData.apVertexBuffer->aBuffers[j],
			meshData.apVertexBuffer->aBufferMem[j],
			VK_BUFFER_USAGE_TRANSFER_DST_BIT | meshData.apVertexBuffer->aFlags
		);
	}
}


void MaterialSystem::CreateVertexBuffer( IModel* renderable, size_t surface )
{
	if ( !renderable )
	{
		LogWarn( gGraphicsChannel, "MaterialSystem::CreateVertexBuffer - Renderable is nullptr\n" );
		return;
	}

	aMeshData[renderable->GetID()].resize( surface + 1 );
	InternalMeshData_t& meshData = aMeshData[renderable->GetID()][surface];
	CreateVertexBufferInt( renderable, surface, meshData );
}


void MaterialSystem::CreateVertexBuffers( IModel* renderable )
{
	if ( !renderable )
	{
		LogWarn( gGraphicsChannel, "MaterialSystem::CreateVertexBuffers - Renderable is nullptr\n" );
		return;
	}

	aMeshData[renderable->GetID()].resize( renderable->GetSurfaceCount() );

	for ( size_t i = 0; i < renderable->GetSurfaceCount(); i++ )
	{
		InternalMeshData_t& meshData = aMeshData[renderable->GetID()][i];
		CreateVertexBufferInt( renderable, i, meshData );
	}
}


// ---------------------------------------------------------------------------------------


void MaterialSystem::CreateIndexBufferInt( IModel* renderable, size_t surface, InternalMeshData_t& meshData )
{
	Assert( meshData.apIndexBuffer == nullptr );

	if ( meshData.apIndexBuffer )
	{
		LogWarn( gGraphicsChannel, "Index Buffer already exists for renderable surface %d\n", surface );
		return;
	}

	// create a new buffer
	meshData.apIndexBuffer = new IndexBuffer;
	meshData.apIndexBuffer->aFlags = gIndexBufferFlags;

	std::vector< uint32_t >& ind = renderable->GetSurfaceIndices( surface );

	InitRenderableBuffer(
		renderable->GetSurfaceIndices( surface ).data(),
		sizeof( uint32_t ) * renderable->GetSurfaceIndices( surface ).size(),
		meshData.apIndexBuffer->aBuffer,
		meshData.apIndexBuffer->aBufferMem,
		VK_BUFFER_USAGE_TRANSFER_DST_BIT | meshData.apIndexBuffer->aFlags
	);
}


void MaterialSystem::CreateIndexBuffer( IModel* renderable, size_t surface )
{
	if ( !renderable )
	{
		LogWarn( gGraphicsChannel, "MaterialSystem::CreateIndexBuffer - Renderable is nullptr\n" );
		return;
	}

	aMeshData[renderable->GetID()].resize( surface );
	InternalMeshData_t& meshData = aMeshData[renderable->GetID()][surface];
	CreateIndexBufferInt( renderable, surface, meshData );
}


void MaterialSystem::CreateIndexBuffers( IModel* renderable )
{
	if ( !renderable )
	{
		LogWarn( gGraphicsChannel, "MaterialSystem::CreateIndexBuffers - Renderable is nullptr\n" );
		return;
	}

	aMeshData[renderable->GetID()].resize( renderable->GetSurfaceCount() );

	for ( size_t i = 0; i < renderable->GetSurfaceCount(); i++ )
	{
		InternalMeshData_t& meshData = aMeshData[renderable->GetID()][i];
		CreateIndexBufferInt( renderable, i, meshData );
	}
}


bool MaterialSystem::HasVertexBuffer( IModel* item, size_t surface )
{
	if ( !item )
	{
		LogWarn( gGraphicsChannel, "MaterialSystem::HasVertexBuffer - Renderable is nullptr\n" );
		return false;
	}

	if ( surface > item->GetSurfaceCount() )
	{
		LogWarn( gGraphicsChannel, "Surface Requested is higher than Mesh Surface Count! (%d < %d)\n", surface, item->GetSurfaceCount() );
		return false;
	}

	auto it = aMeshData.find( item->GetID() );

	if ( it == aMeshData.end() )
		return false;

	InternalMeshData_t& meshData = it->second[surface];

	if ( meshData.apVertexBuffer == nullptr )
		return false;

	return true;
}


bool MaterialSystem::HasIndexBuffer( IModel* item, size_t surface )
{
	if ( !item )
	{
		LogWarn( gGraphicsChannel, "MaterialSystem::HasIndexBuffer - Renderable is nullptr\n" );
		return false;
	}

	if ( surface > item->GetSurfaceCount() )
	{
		LogWarn( gGraphicsChannel, "Surface Requested is higher than Mesh Surface Count! (%d < %d)\n", surface, item->GetSurfaceCount() );
		return false;
	}

	auto it = aMeshData.find( item->GetID() );

	if ( it == aMeshData.end() )
		return false;

	InternalMeshData_t& meshData = it->second[surface];

	if ( meshData.apIndexBuffer == nullptr )
		return false;

	return true;
}


void MaterialSystem::FreeVertexBuffer( IModel* item, size_t surface )
{
	auto it = aMeshData.find( item->GetID() );

	if ( it == aMeshData.end() )
		return;

	InternalMeshData_t& meshData = it->second[surface];

	if ( meshData.apVertexBuffer == nullptr )
		return;

	meshData.apVertexBuffer->Free();
	delete meshData.apVertexBuffer;
	meshData.apVertexBuffer = nullptr;
}


void MaterialSystem::FreeVertexBuffers( IModel* item )
{
	auto it = aMeshData.find( item->GetID() );

	if ( it == aMeshData.end() )
		return;

	for ( size_t i = 0; i < it->second.size(); i++ )
	{
		InternalMeshData_t& meshData = it->second[i];

		if ( meshData.apVertexBuffer == nullptr )
			continue;

		meshData.apVertexBuffer->Free();
		delete meshData.apVertexBuffer;
		meshData.apVertexBuffer = nullptr;
	}
}


void MaterialSystem::FreeIndexBuffer( IModel* item, size_t surface )
{
	auto it = aMeshData.find( item->GetID() );

	if ( it == aMeshData.end() )
		return;

	InternalMeshData_t& meshData = it->second[surface];

	if ( meshData.apIndexBuffer == nullptr )
		return;

	meshData.apIndexBuffer->Free();
	delete meshData.apIndexBuffer;
	meshData.apIndexBuffer = nullptr;
}


void MaterialSystem::FreeIndexBuffers( IModel* item )
{
	auto it = aMeshData.find( item->GetID() );

	if ( it == aMeshData.end() )
		return;

	for ( size_t i = 0; i < it->second.size(); i++ )
	{
		InternalMeshData_t& meshData = it->second[i];

		if ( meshData.apIndexBuffer == nullptr )
			continue;

		meshData.apIndexBuffer->Free();
		delete meshData.apIndexBuffer;
		meshData.apIndexBuffer = nullptr;
	}
}


void MaterialSystem::FreeAllBuffers( IModel* item )
{
	auto it = aMeshData.find( item->GetID() );

	if ( it == aMeshData.end() )
		return;

	for ( size_t i = 0; i < it->second.size(); i++ )
	{
		InternalMeshData_t& meshData = it->second[i];

		if ( meshData.apVertexBuffer == nullptr )
		{
			meshData.apVertexBuffer->Free();
			delete meshData.apVertexBuffer;
			meshData.apVertexBuffer = nullptr;
		}

		if ( meshData.apIndexBuffer )
		{
			meshData.apIndexBuffer->Free();
			delete meshData.apIndexBuffer;
			meshData.apIndexBuffer = nullptr;
		}
	}

	aMeshData.erase( it );
}


static std::vector< InternalMeshData_t > gMeshDataEmpty;

/* Get Vector of Buffers for a Renderable. */
const std::vector< InternalMeshData_t >& MaterialSystem::GetMeshData( IModel* renderable )
{
	auto it = aMeshData.find( renderable->GetID() );

	if ( it != aMeshData.end() )
		return it->second;

	return gMeshDataEmpty;
}


void MaterialSystem::OnReInitSwapChain()
{
	for ( const auto& [shaderName, shader]: aShaders )
	{
		shader->ReInit();
	}
}


void MaterialSystem::OnDestroySwapChain()
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


void MaterialSystem::InitUniformBuffer( IModel* model )
{
	// AWFUL
	// check if any material needs a uniform buffer

	for ( size_t i = 0; i < model->GetSurfaceCount(); i++ )
	{
		if ( !model->GetMaterial( i ) )
			continue;

		Material* mat = (Material*)model->GetMaterial( i );

		if ( !mat->apShader || !mat->apShader->UsesUniformBuffers() )
			continue;

		mat->apShader->InitUniformBuffer( model );
	}
}


#if 0
Handle MaterialSystem::RegisterRenderable( IModel* renderable )
{
	// eh
	//static size_t id = 0;
	//renderable->aId = id++;
	aRenderables.push_back( renderable );
}
#endif


void MaterialSystem::AddRenderable( IRenderable* renderable )
{
	if ( !renderable )
		return;

	IModel* model = renderable->GetModel();
	if ( !model )
	{
		LogWarn( gGraphicsChannel, "Renderable has no model!\n" );
		return;
	}

	for ( size_t i = 0; i < model->GetSurfaceCount(); i++ )
	{
		Material* mat = (Material*)model->GetMaterial( i );

		if ( !mat )
		{
			LogError( gGraphicsChannel, "Model part \"%d\" has no material!\n", i );
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
		aDrawList[mat->apShader].emplace_front( renderable, i );
	}
}


void MaterialSystem::DrawRenderable( size_t memIndex, IRenderable* renderable, size_t matIndex, VkCommandBuffer c, uint32_t commandBufferIndex )
{
	if ( !renderable )
	{
		LogError( gGraphicsChannel, "Renderable is null!\n" );
		return;
	}

	IModel* model = renderable->GetModel();
	if ( !model )
	{
		LogError( gGraphicsChannel, "Renderable has no model!\n" );
		return;
	}

	Material* mat = (Material*)model->GetMaterial( matIndex );
	if ( !mat )
		return;

	// mat->apShader->BindBuffers( renderable, c, commandBufferIndex );
	mat->apShader->Draw( memIndex, renderable, matIndex, c, commandBufferIndex );
}


void MaterialSystem::DestroyModel( IModel* renderable )
{
	MeshFreeOldResources( renderable );
	FreeAllBuffers( renderable );
}


void MaterialSystem::MeshFreeOldResources( IModel* mesh )
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


VkFormat MaterialSystem::ToVkFormat( GraphicsFormat colorFmt )
{
	switch ( colorFmt )
	{
		case GraphicsFormat::INVALID:
		default:
			LogError( gGraphicsChannel, "Unspecified Color Format to VkFormat Conversion!\n" );
			return VK_FORMAT_UNDEFINED;

		// ------------------------------------------

		case GraphicsFormat::R8_SINT:
			return VK_FORMAT_R8_SINT;

		case GraphicsFormat::R8_UINT:
			return VK_FORMAT_R8_UINT;

		case GraphicsFormat::R8_SRGB:
			return VK_FORMAT_R8_SRGB;

		case GraphicsFormat::R8G8_SRGB:
			return VK_FORMAT_R8G8_SRGB;

		case GraphicsFormat::R8G8_SINT:
			return VK_FORMAT_R8G8_SINT;

		case GraphicsFormat::R8G8_UINT:
			return VK_FORMAT_R8G8_UINT;

		// ------------------------------------------

		case GraphicsFormat::R16_SFLOAT:
			return VK_FORMAT_R16_SFLOAT;

		case GraphicsFormat::R16_SINT:
			return VK_FORMAT_R16_SINT;

		case GraphicsFormat::R16_UINT:
			return VK_FORMAT_R16_UINT;

		case GraphicsFormat::R16G16_SFLOAT:
			return VK_FORMAT_R16G16_SFLOAT;

		case GraphicsFormat::R16G16_SINT:
			return VK_FORMAT_R16G16_SINT;

		case GraphicsFormat::R16G16_UINT:
			return VK_FORMAT_R16G16_UINT;

		// ------------------------------------------

		case GraphicsFormat::R32_SFLOAT:
			return VK_FORMAT_R32_SFLOAT;

		case GraphicsFormat::R32_SINT:
			return VK_FORMAT_R32_SINT;

		case GraphicsFormat::R32_UINT:
			return VK_FORMAT_R32_UINT;

		case GraphicsFormat::R32G32_SFLOAT:
			return VK_FORMAT_R32G32_SFLOAT;

		case GraphicsFormat::R32G32_SINT:
			return VK_FORMAT_R32G32_SINT;

		case GraphicsFormat::R32G32_UINT:
			return VK_FORMAT_R32G32_UINT;

		case GraphicsFormat::R32G32B32_SFLOAT:
			return VK_FORMAT_R32G32B32_SFLOAT;

		case GraphicsFormat::R32G32B32_SINT:
			return VK_FORMAT_R32G32B32_SINT;

		case GraphicsFormat::R32G32B32_UINT:
			return VK_FORMAT_R32G32B32_UINT;

		case GraphicsFormat::R32G32B32A32_SFLOAT:
			return VK_FORMAT_R32G32B32A32_SFLOAT;

		case GraphicsFormat::R32G32B32A32_SINT:
			return VK_FORMAT_R32G32B32A32_SINT;

		case GraphicsFormat::R32G32B32A32_UINT:
			return VK_FORMAT_R32G32B32A32_UINT;

		// ------------------------------------------

		case GraphicsFormat::BC1_RGB_UNORM_BLOCK:
			return VK_FORMAT_BC1_RGB_UNORM_BLOCK;

		case GraphicsFormat::BC1_RGB_SRGB_BLOCK:
			return VK_FORMAT_BC1_RGB_SRGB_BLOCK;

		case GraphicsFormat::BC1_RGBA_UNORM_BLOCK:
			return VK_FORMAT_BC1_RGBA_UNORM_BLOCK;

		case GraphicsFormat::BC1_RGBA_SRGB_BLOCK:
			return VK_FORMAT_BC1_RGBA_SRGB_BLOCK;


		case GraphicsFormat::BC2_UNORM_BLOCK:
			return VK_FORMAT_BC2_UNORM_BLOCK;

		case GraphicsFormat::BC2_SRGB_BLOCK:
			return VK_FORMAT_BC2_SRGB_BLOCK;


		case GraphicsFormat::BC3_UNORM_BLOCK:
			return VK_FORMAT_BC3_UNORM_BLOCK;

		case GraphicsFormat::BC3_SRGB_BLOCK:
			return VK_FORMAT_BC3_SRGB_BLOCK;
			

		case GraphicsFormat::BC4_UNORM_BLOCK:
			return VK_FORMAT_BC4_UNORM_BLOCK;

		case GraphicsFormat::BC4_SNORM_BLOCK:
			return VK_FORMAT_BC4_SNORM_BLOCK;


		case GraphicsFormat::BC5_UNORM_BLOCK:
			return VK_FORMAT_BC5_UNORM_BLOCK;

		case GraphicsFormat::BC5_SNORM_BLOCK:
			return VK_FORMAT_BC5_SNORM_BLOCK;


		case GraphicsFormat::BC6H_UFLOAT_BLOCK:
			return VK_FORMAT_BC6H_UFLOAT_BLOCK;

		case GraphicsFormat::BC6H_SFLOAT_BLOCK:
			return VK_FORMAT_BC6H_SFLOAT_BLOCK;


		case GraphicsFormat::BC7_SRGB_BLOCK:
			return VK_FORMAT_BC7_SRGB_BLOCK;

		case GraphicsFormat::BC7_UNORM_BLOCK:
			return VK_FORMAT_BC7_UNORM_BLOCK;
	}
}


VkFormat MaterialSystem::GetVertexAttributeVkFormat( VertexAttribute attrib )
{
	return ToVkFormat( GetVertexAttributeFormat( attrib ) );
}


GraphicsFormat MaterialSystem::GetVertexAttributeFormat( VertexAttribute attrib )
{
	switch ( attrib )
	{
		default:
			LogError( gGraphicsChannel, "GetVertexAttributeFormat: Invalid VertexAttribute specified: %d\n", attrib );
			return GraphicsFormat::INVALID;

		case VertexAttribute_Position:
			return GraphicsFormat::R32G32B32_SFLOAT;

		// NOTE: could be smaller probably
		case VertexAttribute_Normal:
			return GraphicsFormat::R32G32B32_SFLOAT;

		case VertexAttribute_Color:
			return GraphicsFormat::R32G32B32_SFLOAT;

		case VertexAttribute_TexCoord:
			return GraphicsFormat::R32G32_SFLOAT;

		case VertexAttribute_MorphPos:
			return GraphicsFormat::R32G32B32_SFLOAT;
	}
}


size_t MaterialSystem::GetVertexAttributeTypeSize( VertexAttribute attrib )
{
	GraphicsFormat format = GetVertexAttributeFormat( attrib );

	switch ( format )
	{
		default:
			LogError( gGraphicsChannel, "GetVertexAttributeTypeSize: Invalid DataFormat specified from vertex attribute: %d\n", format );
			return 0;

		case GraphicsFormat::INVALID:
			return 0;

		case GraphicsFormat::R32G32B32_SFLOAT:
		case GraphicsFormat::R32G32_SFLOAT:
			return sizeof( float );
	}
}


size_t MaterialSystem::GetVertexAttributeSize( VertexAttribute attrib )
{
#if 0
	return GetFormatSize( GetVertexAttributeFormat( attrib ) );
#else
	GraphicsFormat format = GetVertexAttributeFormat( attrib );

	switch ( format )
	{
		default:
			LogError( gGraphicsChannel, "GetVertexAttributeSize: Invalid DataFormat specified from vertex attribute: %d\n", format );
			return 0;

		case GraphicsFormat::INVALID:
			return 0;

		case GraphicsFormat::R32G32B32_SFLOAT:
			return ( 3 * sizeof( float ) );

		case GraphicsFormat::R32G32_SFLOAT:
			return ( 2 * sizeof( float ) );
	}
#endif
}


size_t MaterialSystem::GetVertexFormatSize( VertexFormat format )
{
	size_t size = 0;

	for ( int attrib = 0; attrib < VertexAttribute_Count; attrib++ )
	{
		// does this format contain this attribute?
		// if so, add the attribute size to it
		if ( format & (1 << attrib) )
			size += GetVertexAttributeSize( (VertexAttribute)attrib );
	}

	return size;
}


// NOTE: this won't work for stuff like GPU compressed formats
#if 0
size_t MaterialSystem::GetFormatSize( DataFormat format )
{
	switch ( format )
	{
		default:
			LogError( gGraphicsChannel, "GetFormatSize: Invalid DataFormat specified: %d\n", format );
			return 0;

		case DataFormat::R32G32B32_SFLOAT:
			return ( 3 * sizeof( float ) );

		case DataFormat::R32G32_SFLOAT:
			return ( 2 * sizeof( float ) );
	}
}
#endif


void MaterialSystem::GetVertexBindingDesc( VertexFormat format, std::vector< VkVertexInputBindingDescription >& srAttrib )
{
	u32 binding  = 0;

	for ( u8 attrib = 0; attrib < VertexAttribute_Count; attrib++ )
	{
		// does this format contain this attribute?
		// if so, add this attribute to the vector
		if ( format & (1 << attrib) )
		{
			srAttrib.emplace_back(
				binding++,
				( u32 )GetVertexAttributeSize( (VertexAttribute)attrib ),
				VK_VERTEX_INPUT_RATE_VERTEX
			);
		}
	}
}


void MaterialSystem::GetVertexAttributeDesc( VertexFormat format, std::vector< VkVertexInputAttributeDescription >& srAttrib )
{
	u32 location = 0;
	u32 binding  = 0;
	u32 offset   = 0;

	auto PushAttrib = [&]( VertexAttribute attrib )
	{
		srAttrib.emplace_back(
			location++,
			binding++,
			GetVertexAttributeVkFormat( attrib ),
			0 // no offset
		);

		// offset += GetVertexAttributeSize( attrib );
	};

	for ( u8 attrib = 0; attrib < VertexAttribute_Count; attrib++ )
	{
		// does this format contain this attribute?
		// if so, add this attribute to the vector
		if ( format & (1 << attrib) )
			PushAttrib( (VertexAttribute)attrib );
	}
}

