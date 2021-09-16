/*
graphics.h ( Authored by p0lyh3dron )

Declares the graphics system along with 
higher level abstrations of types used
by the renderer.  
*/
#pragma once

#include "../shared/system.h"
#include "../core/renderer/renderer.h"

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

class Model
{
private:
	ModelData aModelData;
public:
	/* Returns aModelData, in case it is needed, usually not.  */
	ModelData	&GetModelData(  ){ return aModelData; }
	/* Sets the position of the model to a given x and y.  */
	void 		SetPosition( const glm::vec3 &srPos ){ aModelData.aTransform.position = srPos; }
	/* Translates the model by x y, and z.  */
	void 		Translate( const glm::vec3 &srPos ){ aModelData.aTransform.position += srPos; }
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

	/* Unload a model.  */
	virtual void 		UnloadModel( Model *spModel ) = 0;

	/* Loads a sprite given a path to the sprite,
	   spSprite is optional for external management.  */
	virtual void 		LoadSprite( const std::string& srSpritePath,
				    Sprite *spSprite = NULL ) = 0;

	/* Unload a sprite.  */
	virtual void 		UnloadSprite( Sprite *spSprite ) = 0;

	/* Sets the view  */
	virtual void        SetView( View& view ) = 0;

	/* Get the window width and height  */
	virtual void        GetWindowSize( uint32_t* width, uint32_t* height ) = 0;

	/* Get the SDL_Window  */
	virtual SDL_Window  *GetWindow(  ) = 0;
};
