#include "graphics.h"

#include "instance.h"
#include "present.h"

#include "gui/gui.h"

LOG_REGISTER_CHANNEL( Graphics2, LogColor::DarkYellow );

extern GuiSystem* gui;

extern "C" 
{
    void *cframework_get()
    {
        static Graphics graphics;
        return &graphics;
    }
}

/*
 *    Draws a line from one point to another.
 *
 *    @param glm::vec3    The starting point.
 *    @param glm::vec3    The ending point.
 *    @param glm::vec3    The color of the line.
 */
void Graphics::DrawLine( glm::vec3 sX, glm::vec3 sY, glm::vec3 sColor ) 
{

}

/*
 *    Loads a model from a path.
 *
 *    @param const std::string &    The path to the model.
 *
 *    @return HModel                The handle to the model.
 */
HModel Graphics::LoadModel( const std::string& srModelPath )
{

}

/*
 *    Frees a model.
 *
 *    @param HModel     The handle to the model.
 */
void Graphics::FreeModel( HModel sModel )
{

}

/*
 *    Loads a sprite from a path.
 *
 *    @param const std::string &    The path to the sprite.
 *
 *    @return HSprite               The handle to the sprite.
 */
HSprite Graphics::LoadSprite( const std::string& srSpritePath )
{

}

/*
 *    Frees a sprite.
 *
 *    @param HSprite    The handle to the sprite.
 */
void Graphics::FreeSprite( HSprite sSprite )
{

}

/*
 *    Sets the view.
 *
 *    @param View &    The view to set.
 */
void Graphics::SetView( View& view )
{

}

/*
 *    Returns the surface that the Graphics system is using.
 *
 *    @return Surface *    The surface that the Graphics system is using.
 */
Window *Graphics::GetSurface()
{

}

/*
 *    Creates a material.
 *
 *	  @return HMaterial    The handle to the material.
 */
HMaterial Graphics::CreateMat()
{

}

/*
 *    Frees a material.
 *
 *	  @param HMaterial    The handle to the material.
 */
void Graphics::FreeMat( HMaterial sMat )
{

}

/*
 *    Loads a material.
 *
 *	  @param const std::string &     The path to the material.
 *
 *	  @return HMaterial              The handle to the material.
 */
HMaterial Graphics::LoadMat( const std::string& srMatPath )
{

}

/*
 *    Creates a texture from a file located by path specified.
 *
 *    @param const std::string &    The path to the texture.
 * 
 *	  @return HTexture              The handle to the texture.
 */
HTexture Graphics::CreateTexture( const std::string& srTexturePath )
{

}

/*
 *   Creates a texture from pixel data.
 *
 *   @param void *      The pixel data.
 *   @param uint32_t    The width of the texture.
 *   @param uint32_t    The height of the texture.
 *   @param uint32_t    The pixel format of the texture.
 */
HTexture Graphics::CreateTexture( void *pData, uint32_t width, uint32_t height, uint32_t format )
{

}

/*
 *    Frees a texture.
 *
 *    @param HTexture    The handle to the texture.
 */
void Graphics::FreeTexture( HTexture sTexture )
{

}

/*
 *    Initializes the API.
 */
void Graphics::Init() 
{
    CreateDrawThreads();
    gui->AssignWindow( GetGInstance().GetWindow().apWindow );
}

/*
 *    Updates the API.
 *
 *	  @param float    The time since the last update.
 */
void Graphics::Update( float sDT )
{
    gui->StartFrame();
    RecordCommands();
    Present();
}