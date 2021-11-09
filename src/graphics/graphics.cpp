#include "graphics.h"
#include "util.h"

#include <cstdlib>
#include <ctime>

#include "materialsystem.h"

extern MaterialSystem* materialsystem;

extern "C"
{
	void LoadModel( const char* srModelPath, const char* srTexturePath, Model *spModel )
	{
		if ( spModel == NULL )
			return;

		spModel->GetModelData().SetPos( { 0.f, 0.f, 0.f } );
		aRenderer.InitModel( spModel->GetModelData(  ), srModelPath, srTexturePath );
	
		aModels.push_back( spModel );
	}

	void UnloadModel( Model *spModel )
	{
		vec_remove( aModels, spModel );
	}

	void LoadSprite( const char *srSpritePath, Sprite *spSprite )
	{
		if ( spSprite == NULL )
			return;

	        aRenderer.InitSprite( *spSprite, srSpritePath );
	
		aSprites.push_back( spSprite );
	}

	void FreeSprite( Sprite *spSprite )
	{
		vec_remove( aSprites, spSprite );
	}

	SDL_Window *GetWindow(  )
	{
		return aRenderer.GetWindow(  );
	}

	void SetView( View* view )
	{
		aRenderer.SetView( *view );
	}

	void GetWindowSize( uint32_t* width, uint32_t* height )
	{
		aRenderer.GetWindowSize( width, height );
	}

	void DrawFrame(  )
	{
		DrawGui(  );
		aRenderer.DrawFrame(  );
	}

	void Init(  )
	{
		aRenderer.Init(  );
		aRenderer.InitVulkan(  );
	}

	IMaterialSystem* GetMaterialSystem(  )
	{
		return materialsystem;
	}

	Model* CreateModel(  )
	{
		Model* model = new Model;
		aRenderer.aModels.push_back( &model->GetModelData() );
		return model;
	}
}
