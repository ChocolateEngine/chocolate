#include "graphics.h"
#include "util.h"
#include "util/modelloader.h"
#include "debugprims/primcreator.h"

#include <cstdlib>
#include <ctime>

LOG_REGISTER_CHANNEL( Graphics, LogColor::Cyan );

DebugRenderer   gDbgDrawer;

GraphicsSystem* graphics = new GraphicsSystem;

extern "C" {
	DLL_EXPORT void* cframework_get() {
		return graphics;
	}
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

	// We have not, so try to load this model in
	std::string fullPath = FileSys_FindFile( srPath );

	if ( fullPath.empty() )
	{
		Log_DevF( gGraphicsChannel, 1, "LoadModel: Failed to Find Model: %s\n", srPath.c_str() );
		return nullptr;
	}

	std::string fileExt = FileSys_GetFileExt( srPath );

	Model* model = new Model;

	if ( fileExt == "obj" )
	{
		LoadObj( fullPath, model );
	}
	else if ( fileExt == "glb" || fileExt == "gltf" )
	{
		LoadGltf( fullPath, fileExt, model );
	}
	else
	{
		Log_DevF( gGraphicsChannel, 1, "Unknown Model File Extension: %s\n", fileExt.c_str() );
	}

	//sModel->aRadius = glm::distance( mesh->aMinSize, mesh->aMaxSize ) / 2.0f;

	// TODO: load in an error model here instead
	if ( model->GetSurfaceCount() == 0 )
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
	gDbgDrawer.aMaterials.InitLine( sX, sY, sColor );
}

void GraphicsSystem::FreeModel( Model *spModel )
{
	if ( !spModel )
		return;

	u32 refCount = spModel->GetRefCount();

	if ( refCount == 1 )
	{
		for ( auto& [path, model] : aModelPaths )
		{
			if ( model == spModel )
			{
				aModelPaths.erase( path );
				break;
			}
		}

		matsys->DestroyModel( spModel );
		vec_remove( aRenderer.aModels, spModel );
	}

	spModel->Release();
}

static std::string gStrEmpty;

const std::string& GraphicsSystem::GetModelPath( Model* spModel )
{
	for ( auto& [path, model] : aModelPaths )
	{
		if ( model == spModel )
			return path;
	}

	return gStrEmpty;
}


#if 0
Sprite *GraphicsSystem::LoadSprite( const std::string& srPath )
{
	return nullptr;
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
#endif

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
	aRenderer.InitVulkan();

	gDbgDrawer.Init();

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
