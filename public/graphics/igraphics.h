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

class BaseGraphicsSystem : public BaseSystem
{
public:
	virtual void            DrawLine( const glm::vec3& sX, const glm::vec3& sY, const glm::vec3& sColor ) = 0;

	/* Loads a model given a path for the model and texture, 
	   spModel is optional for external management.  */
	virtual Model*          LoadModel( const std::string& srModelPath ) = 0;

	/* Unload a model.  */
	virtual void 		FreeModel( Model *spModel ) = 0;

	/* Loads a sprite given a path to the sprite,
	   spSprite is optional for external management.  */
	virtual Sprite*     LoadSprite( const std::string& srSpritePath ) = 0;

	/* Fre a sprite. */
	virtual void 		FreeSprite( Sprite *spSprite ) = 0;

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

	/* Draws to the screen.  */
	virtual void   DrawFrame() = 0;
};

#if NEW_NUTS

#endif
