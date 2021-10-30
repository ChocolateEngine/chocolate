/*
graphics.h ( Authored by p0lyh3dron )

Declares the graphics system along with 
higher level abstrations of types used
by the renderer.  
*/
#pragma once

#include "system.h"
#include "graphics/imesh.h"
#include "graphics/imaterialsystem.h"

#include <SDL2/SDL.h>

// uhhh
#if SPRITES
class Sprite
{
private:
	SpriteData aSpriteData;
public:
	/* Returns aSpriteData, in case it is needed, usually not.  */
	SpriteData      &GetSpriteData(  ) { return aSpriteData; }
	/* Sets the position of the sprite to a given x and y.  */
	void 		SetPosition( float sX, float sY ){ aSpriteData.aPos.x = sX;
						   aSpriteData.aPos.y = sY; }
	/* Translates the sprite by x and y.  */
	void 		Translate( float sX, float sY ){ aSpriteData.aPos.x += sX;
					         aSpriteData.aPos.y += sY; }
	/* Sets the visibility of the sprite.  */
	void 		SetVisibility( bool sVisible ){ aSpriteData.aNoDraw = !sVisible; }
};
#endif

class Model
{
private:
	ModelData aModelData;
public:
	/* Returns aModelData, in case it is needed, usually not.  */
	ModelData	&GetModelData(  ){ return aModelData; }
	/* Sets the visibility of the model.  */
	void 		SetVisibility( bool sVisible ){ aModelData.aNoDraw = !sVisible; }
};

class BaseGraphicsSystem : public BaseSystem
{
public:
	/* Loads a model given a path for the model and texture, 
	   spModel is optional for external management.  */
	virtual void 		LoadModel( const std::string& srModelPath,
				   const std::string& srTexturePath,
				   Model *spModel = NULL ) = 0;

	/* Unload a model. (Change to FreeModel)  */
	virtual void 		UnloadModel( Model *spModel ) = 0;

#if SPRITES
	/* Loads a sprite given a path to the sprite,
	   spSprite is optional for external management.  */
	virtual void 		LoadSprite( const std::string& srSpritePath,
				    Sprite *spSprite = NULL ) = 0;

	/* Unload a sprite. (Change to FreeSprite) */
	virtual void 		UnloadSprite( Sprite *spSprite ) = 0;
#endif

	/* Sets the view  */
	virtual void        SetView( View& view ) = 0;

	/* Get the window width and height  */
	virtual void        GetWindowSize( uint32_t* width, uint32_t* height ) = 0;

	/* Get the SDL_Window  */
	virtual SDL_Window  *GetWindow(  ) = 0;

	/*  */
	virtual IMaterialSystem *GetMaterialSystem(  ) = 0;

	/* Creates a model and stores it for drawing use */
	virtual Model *CreateModel(  ) = 0;
};
