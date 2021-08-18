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
	sprite_data_t aSpriteData;
public:
	/* Sets the position of the sprite to a given x and y.  */
	void 	SetPosition( float sX, float sY ){ aSpriteData.posX = sX;
						   aSpriteData.posY = sY; }
	/* Translates the sprite by x and y.  */
	void 	Translate( float sX, float sY ){ aSpriteData.posX += sX;
					         aSpriteData.posY += sY; }
	/* Sets the visibility of the sprite.  */
	void 	SetVisibility( bool sVisible ){ aSpriteData.noDraw = !sVisible; }
};

class Model
{
private:
	model_data_t aModelData;
};

class GraphicsSystem : public BaseSystem
{
	SYSTEM_OBJECT( GraphicsSystem )
protected:
        typedef std::vector< Sprite* > 	SpriteList;
	typedef std::vector< Model* >	ModelList;

	int 		random = time( 0 );
        SpriteList 	aSprites;
        ModelList 	aModels;
	static std::vector< model_data_t* > modelData;
	static std::vector< sprite_data_t* > spriteData;
	renderer_c renderer;

	/* Loads a model given a path for the model and texture, 
	   spModel is optional for external management.  */
	void 		LoadModel( const std::string& srModelPath,
				   const std::string& srTexturePath,
				   Model *spModel = NULL );
	/* Loads a sprite given a path to the sprite,
	   spSprite is optional for external management.  */
	void 		LoadSprite( const std::string& srSpritePath,
				    Model *spSprite = NULL );
public:
	/* Draws all loaded models and sprites to screen.  */
	void 		DrawFrame(  );
	/* Syncronizes communication with renderer.  */
	void 		SyncRenderer(  );

	/* Sets some renderer parameters.  */
	explicit 	GraphicsSystem(  );
};
