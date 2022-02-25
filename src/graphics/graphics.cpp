#include "graphics.h"
#include "util.h"

#include <cstdlib>
#include <ctime>

GraphicsSystem* graphics = new GraphicsSystem;

extern "C" {
	DLL_EXPORT void* cframework_get() {
		return graphics;
	}
}

Model *GraphicsSystem::LoadModel( const std::string&srPath )
{
	Model *model = new Model;

	if ( !aRenderer.LoadModel( model, srPath ) )
	{
		delete model;
		return nullptr;
	}
	
	aModels.push_back( model );

	return model;
}

void GraphicsSystem::DrawLine( glm::vec3 sX, glm::vec3 sY, glm::vec3 sColor ) {
	aRenderer.CreateLine( sX, sY, sColor );
}

void GraphicsSystem::FreeModel( Model *spModel )
{
	if ( !spModel )
		return;

	matsys->DestroyRenderable( spModel );
	vec_remove( aModels, spModel );
	delete spModel;
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
