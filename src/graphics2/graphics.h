/*
 *    graphics.h    --    Header for Chocolate Graphics
 *
 *    Authored by Karl "p0lyh3dron" Kreuze on February 24, 2022
 *
 *    This header declares the graphics API and all of its
 *    functions.
 */
#pragma once

#include "graphics/igraphics.h"

class Graphics : public IGraphics
{
protected:  
public:
    /*
     *    Draws a line from one point to another.
     *
     *    @param glm::vec3    The starting point.
     *    @param glm::vec3    The ending point.
     *    @param glm::vec3    The color of the line.
     */
    void             DrawLine( glm::vec3 sX, glm::vec3 sY, glm::vec3 sColor ) override;

	/*
     *    Loads a model from a path.
     *
     *    @param const std::string &    The path to the model.
     *
     *    @return HModel                The handle to the model.
     */
	HModel           LoadModel( const std::string& srModelPath ) override;

	/*
     *    Frees a model.
     *
     *    @param HModel     The handle to the model.
     */
	void 		     FreeModel( HModel sModel ) override;

	/*
     *    Loads a sprite from a path.
     *
     *    @param const std::string &    The path to the sprite.
     *
     *    @return HSprite               The handle to the sprite.
     */
	HSprite          LoadSprite( const std::string& srSpritePath ) override;

	/*
     *    Frees a sprite.
     *
     *    @param HSprite    The handle to the sprite.
     */
	void 		     FreeSprite( HSprite sSprite ) override;

	/*
     *    Sets the view.
     *
     *    @param View &    The view to set.
     */
	void             SetView( View& view ) override;

	/*
     *    Returns the surface that the Graphics system is using.
     *
     *    @return Surface *    The surface that the Graphics system is using.
     */
	Surface         *GetSurface() override;

	/*
	 *    Creates a material.
	 *
	 *	  @return HMaterial    The handle to the material.
	 */
	HMaterial        CreateMat() override;

	/*
	 *    Frees a material.
	 *
	 *	  @param HMaterial    The handle to the material.
	 */
	void 		     FreeMat( HMaterial sMat ) override;

	/*
	 *    Loads a material.
	 *
	 *	  @param const std::string &     The path to the material.
	 *
	 *	  @return HMaterial              The handle to the material.
	 */
	HMaterial        LoadMat( const std::string& srMatPath ) override;

	/*
	 *    Creates a texture from a file located by path specified.
	 *
	 *    @param const std::string &    The path to the texture.
	 * 
	 *	  @return HTexture              The handle to the texture.
	 */
	HTexture         CreateTexture( const std::string& srTexturePath ) override;
	
	/*
	 *   Creates a texture from pixel data.
	 *
	 *   @param void *      The pixel data.
	 *   @param uint32_t    The width of the texture.
	 *   @param uint32_t    The height of the texture.
	 *   @param uint32_t    The pixel format of the texture.
	 */
	HTexture         CreateTexture( void *pData, uint32_t width, uint32_t height, uint32_t format ) override;

	/*
	 *    Frees a texture.
	 *
	 *    @param HTexture    The handle to the texture.
	 */
	void 		     FreeTexture( HTexture sTexture ) override;

    /*
	 *    Initializes the API.
	 */
	void             Init() override;
};