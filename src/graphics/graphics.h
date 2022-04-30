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


LOG_CHANNEL( Graphics );

class GraphicsSystem : public BaseGraphicsSystem
{
public:
	typedef BaseSystem BaseClass;

protected:
	typedef std::vector< Sprite* > 	SpriteList;
	typedef std::vector< Model* >	ModelList;

	int 		aRandom = time( 0 );
	SpriteList 	aSprites;
	ModelList 	aModels;
	Renderer 	aRenderer;

	std::unordered_map< std::string, Model* > aModelPaths;

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

	void            DrawLine( const glm::vec3& sX, const glm::vec3& sY, const glm::vec3& sColor ) override;

	/* Loads a model given a path */
	Model* 		LoadModel( const std::string& srModelPath ) override;
	/* Unload a model.  */
	void 		FreeModel( Model *spModel ) override;

	/* Loads a sprite given a path to the sprite */
	Sprite*    LoadSprite( const std::string& srSpritePath ) override;

	/* Unload a sprite.  */
	void 		FreeSprite( Sprite *spSprite ) override;

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
