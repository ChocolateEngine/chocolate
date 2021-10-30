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


class GraphicsSystem : public BaseGraphicsSystem
{
public:
	typedef BaseSystem BaseClass;

protected:
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

	/*   */
	void 		Init(  );
	/* Initializes all commands the system can respond to.  */
	void 		InitCommands(  );
	/* Initializes all console commands the system can respond to.  */
	void 		InitConsoleCommands(  );
	/* Initializes any member systems of the base system.  */
	void    	InitSubsystems(  );

public:
	enum class	Commands{ NONE = 0, INIT_SPRITE, INIT_MODEL };
	/* Update  */
	void 		Update( float dt );
	/* Draws all loaded models and sprites to screen.  */
	void 		DrawFrame(  );

	/* Loads a model given a path for the model and texture, 
	   spModel is optional for external management.  */
	void 		LoadModel( const std::string& srModelPath,
				   const std::string& srTexturePath,
				   Model *spModel = NULL ) override;
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
	void        SetView( View& view ) override;
	/* Get the window width and height  */
	void        GetWindowSize( uint32_t* width, uint32_t* height ) override;
	/* Get the SDL_Window  */
	SDL_Window  *GetWindow(  ) override;

	IMaterialSystem *GetMaterialSystem(  ) override;
	Model *CreateModel(  ) override;

	/* Sets some renderer parameters.  */
	explicit 	GraphicsSystem(  );
};
