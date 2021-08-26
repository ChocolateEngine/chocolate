/*
graphics.h ( Authored by p0lyh3dron )

Declares the graphics system along with 
higher level abstrations of types used
by the renderer.  
*/
#pragma once

#include "../shared/system.h"
#include "renderer/renderer.h"

class Sprite
{
private:
        SpriteData aSpriteData;
public:
	/* Returns aSpriteData, in case it is needed, usually not.  */
        SpriteData      &GetSpriteData(  ) { return aSpriteData; }
	/* Sets the position of the sprite to a given x and y.  */
	void 		SetPosition( float sX, float sY ){ aSpriteData.aPosX = sX;
						   aSpriteData.aPosY = sY; }
	/* Translates the sprite by x and y.  */
	void 		Translate( float sX, float sY ){ aSpriteData.aPosX += sX;
					         aSpriteData.aPosY += sY; }
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
	void 		SetPosition( float sX, float sY, float sZ ){ aModelData.aPosX = sX;
						   	     aModelData.aPosY = sY;
							     aModelData.aPosZ = sZ; }
	/* Translates the model by x y, and z.  */
	void 		Translate( float sX, float sY, float sZ ){ aModelData.aPosX += sX;
					         	   aModelData.aPosY += sY;
							   aModelData.aPosZ += sZ; }
	/* Sets the visibility of the model.  */
	void 		SetVisibility( bool sVisible ){ aModelData.aNoDraw = !sVisible; }
};

class GraphicsSystem : public BaseSystem
{
protected:
        typedef std::vector< Sprite* > 	SpriteList;
	typedef std::vector< Model* >	ModelList;

	int 		aRandom = time( 0 );
        SpriteList 	aSprites;
        ModelList 	aModels;
	Renderer 	aRenderer;

	/* Loads a model given a path for the model and texture, 
	   spModel is optional for external management.  */
	void 		LoadModel( const std::string& srModelPath,
				   const std::string& srTexturePath,
				   Model *spModel = NULL );
	/* Loads a sprite given a path to the sprite,
	   spSprite is optional for external management.  */
	void 		LoadSprite( const std::string& srSpritePath,
				    Sprite *spSprite = NULL );
	/* Initializes all commands the system can respond to.  */
	void 		InitCommands(  );
	/* Initializes all console commands the system can respond to.  */
	void 		InitConsoleCommands(  );
	/* Initializes any member systems of the base system.  */
	void    	InitSubsystems(  );
public:
	/* Draws all loaded models and sprites to screen.  */
	void 		DrawFrame(  );
	/* Syncronizes communication with renderer.  */
	void 		SyncRenderer(  );

	/* Sets some renderer parameters.  */
	explicit 	GraphicsSystem(  );
};
