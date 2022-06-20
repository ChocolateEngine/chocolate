#include "graphics.h"

#include "materialsystem.h"
#include "commandpool.h"
#include "instance.h"
#include "present.h"
#include "renderpass.h"
#include "descriptormanager.h"
#include "swapchain.h"
#include "rendertarget.h"
#include "config.hh"

#include "imgui/imgui_impl_vulkan.h"
#include "imgui/imgui_impl_sdl.h"

LOG_REGISTER_CHANNEL( Graphics2, LogColor::DarkYellow );

extern "C" 
{
    DLL_EXPORT void *cframework_get()
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
    return InvalidHandle;
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
#pragma message ("lisa and andrew came over and got a surface")
    return &GetGInstance().GetWindow();
}

/*
 *    Creates a material.
 *
 *	  @return HMaterial    The handle to the material.
 */
HMaterial Graphics::CreateMat()
{
    return InvalidHandle;
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
    return InvalidHandle;
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
    return InvalidHandle;
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
    return InvalidHandle;
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
    GetGInstance().Init();
	
    // init single time command pool
    GetSingleTimeCommandPool();

    // init swapchain
    GetSwapchain();

    // init renderpass
    GetRenderPass( RenderPass_Color | RenderPass_Depth | RenderPass_Resolve );
	
    // create color and depth backbuffer (transitions from image layout/format undefined)
    GetBackBuffer();

    // create texture sampler
    GetSampler();

    // create descriptor pool
    GetDescriptorManager().CreateDescriptorPool();
    // CreateDescriptorSetLayouts();

    // init imgui (in between it? um ok)
    ImGui::CreateContext();

    Window& window = GetGInstance().GetWindow();

    ImGui_ImplSDL2_InitForVulkan( window.apWindow );

    ImGui_ImplVulkan_InitInfo init_info = {};
    init_info.Instance        = GetInst();
    init_info.PhysicalDevice  = GetPhysicalDevice();
    init_info.Device          = GetDevice();
    init_info.Queue           = GetGInstance().GetGraphicsQueue();
    init_info.DescriptorPool  = GetDescriptorManager().GetHandle();
    init_info.MinImageCount   = GetSwapchain().GetImageCount();
    init_info.ImageCount      = GetSwapchain().GetImageCount();
    init_info.MSAASamples     = GetMSAASamples();
    //init_info.CheckVkResultFn = CheckVKResult;

    // maybe don't use resolve on imgui renderpass???? idk
    ImGui_ImplVulkan_Init( &init_info, GetRenderPass( RenderPass_Color | RenderPass_Depth | RenderPass_Resolve ) );

    SingleCommand( []( VkCommandBuffer c ){ ImGui_ImplVulkan_CreateFontsTexture( c ); } );
    ImGui_ImplVulkan_DestroyFontUploadObjects();
	
    CreateDrawThreads();
	
    GetDescriptorManager().CreateDescriptorSetLayouts();

    matsys.Init();
}

/*
 *    Updates the API.
 *
 *	  @param float    The time since the last update.
 */
void Graphics::Update( float sDT )
{
    RecordCommands();
    Present();
}

