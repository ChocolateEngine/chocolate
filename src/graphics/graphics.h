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

#if SPRITES
typedef std::vector< Sprite* > 	SpriteList;
#endif
typedef std::vector< Model* >	ModelList;

int 		aRandom = time( 0 );
#if SPRITES
SpriteList 	aSprites;
#endif
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

#if SPRITES
/* Loads a sprite given a path to the sprite,
   spSprite is optional for external management.  */
	void 		LoadSprite( const std::string& srSpritePath,
				    Sprite *spSprite = NULL );
/* Unload a sprite.  */
	void 		UnloadSprite( Sprite *spSprite );
#endif

/* Sets the view  */
	void        SetView( View* view );
/* Get the window width and height  */
	void        GetWindowSize( uint32_t* width, uint32_t* height );
/* Get the SDL_Window  */
	SDL_Window  *GetWindow(  );

	IMaterialSystem *GetMaterialSystem(  );
	Model *CreateModel(  );
}
