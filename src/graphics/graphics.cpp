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

void GraphicsSystem::LoadModel( const std::string& srModelPath, const std::string& srTexturePath, Model *spModel )
{
	if ( spModel == NULL )
		return;

	spModel->GetModelData().SetPos( { 0.f, 0.f, 0.f } );
	aRenderer.InitModel( spModel->GetModelData(  ), srModelPath, srTexturePath );
	
	aModels.push_back( spModel );
}

void GraphicsSystem::UnloadModel( Model *spModel )
{
	vec_remove( aModels, spModel );
}

void GraphicsSystem::LoadSprite( const std::string& srSpritePath, Sprite *spSprite )
{
	if ( spSprite == NULL )
		return;

	aRenderer.InitSprite( *spSprite, srSpritePath );
	
	aSprites.push_back( spSprite );
}

void GraphicsSystem::FreeSprite( Sprite *spSprite )
{
	vec_remove( aSprites, spSprite );
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

extern MaterialSystem* materialsystem;

IMaterialSystem* GraphicsSystem::GetMaterialSystem(  )
{
	return materialsystem;
}

Model* GraphicsSystem::CreateModel(  )
{
	Model* model = new Model;
	aRenderer.aModels.push_back( &model->GetModelData() );
	return model;
}
