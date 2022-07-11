#pragma once

#include "system.h"
#include "graphics/renderertypes.h"
#include <stdio.h>
#include <SDL.h>
#include <glm/glm.hpp>

using HModel    = Handle;
using HMaterial = Handle;
using HTexture  = Handle;

struct Window 
{
    uint32_t         aWidth;
    uint32_t         aHeight;
    SDL_Window      *apWindow;
};

class IGraphics : public BaseSystem {
protected:
public:
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
	virtual Window          *GetSurface() = 0;

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
	virtual HTexture         LoadTexture( const std::string& srTexturePath ) = 0;
	
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