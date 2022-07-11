/*
 *    graphics.h    --    Header for Chocolate Graphics
 *
 *    Authored by Karl "p0lyh3dron" Kreuze on February 24, 2022
 *
 *    This header declares the graphics API and all of its
 *    functions.
 */
#pragma once

#include "core/core.h"

#include "graphics/igraphics2.h"

LOG_CHANNEL( Graphics2 );

class Texture2;

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
	Window          *GetSurface() override;

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
	HTexture         LoadTexture( const std::string& srTexturePath ) override;
	
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
	 *    Initializes ImGui.
	 */
	void             InitImGui();

    /*
	 *    Initializes the API.
	 */
	void             Init() override;
	
	/*
	 *    Updates the API.
	 *
	 *	  @param float    The time since the last update.
	 */
	void             Update( float sDT ) override;
	
private:
	// TEMP TEMP TEMP TEMP !!!!!!!!!!!!!!!!!!!!!
	void             AllocImageSets();
	void             UpdateImageSets();

public:
	
	HTexture  aMissingTexHandle = InvalidHandle;
	Texture2* apMissingTex = nullptr;

	std::unordered_map< std::string, HTexture > aTexturePaths;
	ResourceManager< Texture2 >                 aTextures;

private:
	
	// TODO: MOVE THIS TO DESCRIPTOR MANAGER (PROBABLY)
	std::vector< VkDescriptorSet > aImageSets;
	VkDescriptorSetLayout          aImageLayout;
};
