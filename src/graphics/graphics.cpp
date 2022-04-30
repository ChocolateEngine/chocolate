#include "graphics.h"
#include "util.h"
#include "util/modelloader.h"

#include <cstdlib>
#include <ctime>

LOG_REGISTER_CHANNEL( Graphics, LogColor::Cyan );

GraphicsSystem* graphics = new GraphicsSystem;

extern "C" {
	DLL_EXPORT void* cframework_get() {
		return graphics;
	}
}


bool LoadBaseMeshes( Model* sModel, const std::string& srPath )
{
	std::string fileExt = filesys->GetFileExt( srPath );

	if ( fileExt == "obj" )
	{
		LoadObj( srPath, sModel );
	}
	else if ( fileExt == "glb" || fileExt == "gltf" )
	{
		LogDev( 1, "[Renderer] GLTF currently not supported, on TODO list\n" );
		return false;
	}

	// TODO: load in an error model here somehow?
	if ( sModel->GetMaterialCount() == 0 )
		return false;

	matsys->CreateVertexBuffer( sModel );

	// if the vertex count is different than the index count, use the index buffer
	if ( sModel->GetVertices().size() != sModel->GetIndices().size() )
		matsys->CreateIndexBuffer( sModel );

	//mesh->aRadius = glm::distance( mesh->aMinSize, mesh->aMaxSize ) / 2.0f;

	matsys->MeshInit( sModel );

	return true;
}


Model* GraphicsSystem::LoadModel( const std::string &srPath )
{
	// Have we loaded this model already?
	auto it = aModelPaths.find( srPath );

	if ( it != aModelPaths.end() )
	{
		// We have, use that model instead
		it->second->AddRef();
		return it->second;
	}

	// We have not, so load this model in
	Model* model = new Model;
	LoadBaseMeshes( model, srPath );

	if ( model->GetMaterialCount() == 0 )
	{
		delete model;
		return nullptr;
	}

	aRenderer.aModels.push_back( model );
	aModelPaths[srPath] = model;

	model->AddRef();
	return model;
}


void GraphicsSystem::DrawLine( const glm::vec3& sX, const glm::vec3& sY, const glm::vec3& sColor )
{
	// amazing, fantastic, awful
	aRenderer.aDbgDrawer.aMaterials.InitLine( sX, sY, sColor );
}

void GraphicsSystem::FreeModel( Model *spModel )
{
	if ( !spModel )
		return;

	u32 refCount = spModel->GetRefCount();

	if ( refCount == 1 )
	{
		matsys->DestroyRenderable( spModel );
		vec_remove( aRenderer.aModels, spModel );

		// man
		for ( auto& [path, model] : aModelPaths )
		{
			if ( model == spModel )
			{
				aModelPaths.erase( path );
				break;
			}
		}
	}

	spModel->Release();
}

Sprite *GraphicsSystem::LoadSprite( const std::string& srPath )
{
	Sprite *sprite = new Sprite;

	if ( !aRenderer.LoadSprite( *sprite, srPath ) )
	{
		delete sprite;
		return nullptr;
	}

	aSprites.push_back( sprite );

	return sprite;
}

void GraphicsSystem::FreeSprite( Sprite *spSprite )
{
	vec_remove( aSprites, spSprite );
	delete spSprite;
}

SDL_Window *GraphicsSystem::GetWindow(  )
{
	return aRenderer.GetWindow(  );
}

void GraphicsSystem::SetView( View& view )
{
	aRenderer.SetView( view );
}

void GraphicsSystem::GetWindowSize( uint32_t* width, uint32_t* height )
{
	aRenderer.GetWindowSize( width, height );
}

void GraphicsSystem::DrawFrame(  )
{
}

GraphicsSystem::GraphicsSystem(  ) : BaseGraphicsSystem(  )
{
}

void GraphicsSystem::Init(  )
{
	aRenderer.Init();
	aRenderer.InitVulkan(  );

	gui->Init();
}

void GraphicsSystem::Update( float dt )
{
	aRenderer.DrawFrame();
}

extern MaterialSystem* matsys;

IMaterialSystem* GraphicsSystem::GetMaterialSystem(  )
{
	return matsys;
}

Model* GraphicsSystem::CreateModel(  )
{
	Model* model = new Model;
	aRenderer.aModels.push_back( model );
	return model;
}
