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

using HModel    = size_t;
using HMaterial = size_t;
using HTexture  = size_t;
using HSprite   = size_t;

using Window    = size_t;

class BaseGraphicsSystem : public BaseSystem
{
public:
	virtual void            DrawLine( glm::vec3 sX, glm::vec3 sY, glm::vec3 sColor ) = 0;

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

class IGraphics : public BaseSystem {
	/*
     *    Draws a line from one point to another.
     *
     *    @param glm::vec3    The starting point.
     *    @param glm::vec3    The ending point.
     *    @param glm::vec3    The color of the line.
     */
    virtual void             DrawLine( glm::vec3 sX, glm::vec3 sY, glm::vec3 sColor ) = 0;

	/*
     *    Loads a model from a path.
     *
     *    @param const std::string &    The path to the model.
     *
     *    @return HModel                The handle to the model.
     */
	virtual HModel           LoadModel( const std::string& srModelPath ) = 0;

	/*
     *    Frees a model.
     *
     *    @param HModel     The handle to the model.
     */
	virtual void 		     FreeModel( HModel sModel ) = 0;

	/*
     *    Loads a sprite from a path.
     *
     *    @param const std::string &    The path to the sprite.
     *
     *    @return HSprite               The handle to the sprite.
     */
	virtual HSprite          LoadSprite( const std::string& srSpritePath ) = 0;

	/*
     *    Frees a sprite.
     *
     *    @param HSprite    The handle to the sprite.
     */
	virtual void 		     FreeSprite( HSprite sSprite ) = 0;

	/*
     *    Sets the view.
     *
     *    @param View &    The view to set.
     */
	virtual void             SetView( View& view ) = 0;

	/*
     *    Returns the surface that the Graphics system is using.
     *
     *    @return Surface *    The surface that the Graphics system is using.
     */
	virtual Surface         *GetSurface() = 0;

	/*
	 *    Creates a material.
	 *
	 *	  @return HMaterial    The handle to the material.
	 */
	virtual HMaterial        CreateMat() = 0;

	/*
	 *    Frees a material.
	 *
	 *	  @param HMaterial    The handle to the material.
	 */
	virtual void 		     FreeMat( HMaterial sMat ) = 0;

	/*
	 *    Loads a material.
	 *
	 *	  @param const std::string &     The path to the material.
	 *
	 *	  @return HMaterial              The handle to the material.
	 */
	virtual HMaterial        LoadMat( const std::string& srMatPath ) = 0;

	/*
	 *    Creates a texture from a file located by path specified.
	 *
	 *    @param const std::string &    The path to the texture.
	 * 
	 *	  @return HTexture              The handle to the texture.
	 */
	virtual HTexture         CreateTexture( const std::string& srTexturePath ) = 0;
	
	/*
	 *   Creates a texture from pixel data.
	 *
	 *   @param void *      The pixel data.
	 *   @param uint32_t    The width of the texture.
	 *   @param uint32_t    The height of the texture.
	 *   @param uint32_t    The pixel format of the texture.
	 */
	virtual HTexture         CreateTexture( void *pData, uint32_t width, uint32_t height, uint32_t format ) = 0;

	/*
	 *    Frees a texture.
	 *
	 *    @param HTexture    The handle to the texture.
	 */
	virtual void 		     FreeTexture( HTexture sTexture ) = 0;
};
