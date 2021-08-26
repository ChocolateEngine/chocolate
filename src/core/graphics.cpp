#include "../../inc/core/graphics.h"

#include <cstdlib>
#include <ctime>

void GraphicsSystem::LoadModel( const std::string& srModelPath, const std::string& srTexturePath, Model *spModel )
{
	if ( spModel == NULL )
	        spModel = new Model;
	spModel->SetPosition( 0.f, 0.f, 0.f );
	aRenderer.InitModel( spModel->GetModelData(  ), srModelPath, srTexturePath );
	
	aModels.push_back( spModel );
	AddFreeFunction( [ = ](  ){ delete spModel; } );
}

void GraphicsSystem::LoadSprite( const std::string& srSpritePath, Sprite *spSprite )
{
	if ( spSprite == NULL )
		spSprite = new Sprite;

	spSprite->SetPosition( 0.f, 0.f );
	aRenderer.InitSprite( spSprite->GetSpriteData(  ), srSpritePath );
	
	aSprites.push_back( spSprite );
	AddFreeFunction( [ = ](  ){ delete spSprite; } );
}

void GraphicsSystem::InitCommands(  )
{
        auto loadSprite = std::bind( &GraphicsSystem::LoadSprite, this, std::placeholders::_1, std::placeholders::_2 );
	apMsgs->aCmdManager.Add( "_load_sprite", Command< const std::string&, Sprite* >( loadSprite ) );
	auto loadModel = std::bind( &GraphicsSystem::LoadModel, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3 );
	apMsgs->aCmdManager.Add( "_load_model", Command< const std::string&, const std::string&, Model* >( loadModel ) );
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
	aRenderer.apMsgs 		= this->apMsgs;
	aRenderer.apConsole 	= this->apConsole;
}

GraphicsSystem::GraphicsSystem(  ) : BaseSystem(  )
{
	aSystemType = GRAPHICS_C;
	AddUpdateFunction( [ & ](  ){ aRenderer.Update(  ); } );
        AddUpdateFunction( [ & ](  ){ DrawFrame(  ); } );

	aRenderer.InitVulkan(  );

	LoadModel( "materials/models/protogen_wip_22/protogen_wip_22.obj", "materials/textures/blue_mat.png" );
}

void GraphicsSystem::InitSubsystems(  )
{
        SyncRenderer(  );
	aRenderer.SendMessages(  );
}