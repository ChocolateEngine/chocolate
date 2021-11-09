/*
graphics.h ( Authored by p0lyh3dron )

Declares the graphics system along with 
higher level abstrations of types used
by the renderer.  
*/
#pragma once

#include "system.h"
#include "renderer.h"
#include "graphics/igraphics.h"



typedef BaseSystem BaseClass;

typedef std::vector< Sprite* > 	SpriteList;
typedef std::vector< Model* >	ModelList;

int 		aRandom = time( 0 );
SpriteList 	aSprites;
ModelList 	aModels;
Renderer 	aRenderer;

extern "C"
{
/*   */
	void 		Init(  );
/* Draws all loaded models and sprites to screen.  */
	void 		DrawFrame(  );

/* Loads a model given a path for the model and texture, 
   spModel is optional for external management.  */
	void 		LoadModel( const char* srModelPath,
				   const char* srTexturePath,
				   Model *spModel = NULL );
/* Unload a model.  */
	void 		UnloadModel( Model *spModel );
/* Loads a sprite given a path to the sprite,
   spSprite is optional for external management.  */
	void 		LoadSprite( const char *srSpritePath,
				    Sprite *spSprite = NULL );
/* Unload a sprite.  */
	void 		FreeSprite( Sprite *spSprite );

/* Sets the view  */
	void        SetView( View* view );
/* Get the window width and height  */
	void        GetWindowSize( uint32_t* width, uint32_t* height );
/* Get the SDL_Window  */
	SDL_Window  *GetWindow(  );

	IMaterialSystem *GetMaterialSystem(  );
	Model *CreateModel(  );
}
