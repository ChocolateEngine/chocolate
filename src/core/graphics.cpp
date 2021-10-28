#include "../../inc/core/graphics.h"
#include "../../inc/shared/util.h"

#include <cstdlib>
#include <ctime>

void GraphicsSystem::LoadModel( const std::string& srModelPath, const std::string& srTexturePath, Model *spModel )
{
	if ( spModel == NULL )
		spModel = new Model;

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
		spSprite = new Sprite;

	spSprite->SetPosition( 0.f, 0.f );
	aRenderer.InitSprite( spSprite->GetSpriteData(  ), srSpritePath );
	
	aSprites.push_back( spSprite );
}

void GraphicsSystem::UnloadSprite( Sprite *spSprite )
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

void GraphicsSystem::InitCommands(  )
{
	auto loadSprite = std::bind( &GraphicsSystem::LoadSprite, this, std::placeholders::_1, std::placeholders::_2 );
	apCommandManager->Add( GraphicsSystem::Commands::INIT_SPRITE, Command< const std::string&, Sprite* >( loadSprite ) );
	
	auto loadModel = std::bind( &GraphicsSystem::LoadModel, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3 );       
	apCommandManager->Add( GraphicsSystem::Commands::INIT_MODEL, Command< const std::string&, const std::string&, Model* >( loadModel ) );
}

void GraphicsSystem::InitConsoleCommands(  )
{
	
}

void GraphicsSystem::DrawFrame(  )
{
	aRenderer.DrawFrame(  );
}

void GraphicsSystem::SyncRenderer(  )
{
	aRenderer.apMsgs        	= apMsgs;
 	aRenderer.apConsole 		= apConsole;
	aRenderer.apCommandManager	= apCommandManager;
}

GraphicsSystem::GraphicsSystem(  ) : BaseGraphicsSystem(  )
{
	aSystemType = GRAPHICS_C;
}

void GraphicsSystem::Init(  )
{
	BaseClass::Init(  );
	aRenderer.InitVulkan(  );
}

void GraphicsSystem::Update( float dt )
{
	BaseClass::Update( dt );
	aRenderer.Update( dt );
	DrawFrame(  );
}

void GraphicsSystem::SendMessages(  )
{
	aRenderer.SendMessages(  );
}

void GraphicsSystem::InitSubsystems(  )
{
	SyncRenderer(  );
	aRenderer.Init(  );
	aRenderer.SendMessages(  );
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
