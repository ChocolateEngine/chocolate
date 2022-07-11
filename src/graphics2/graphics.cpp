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

#include "textures/missingtexture.h"
#include "textures/itextureloader.h"

// TEMP
#include "freemans_tshirt.hpp"

freemans_tshirt_t freemans_tshirt;

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


std::vector< ITextureLoader* > gTextureLoaders;
constexpr u32 MAX_IMAGES = 1000;


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
HTexture Graphics::LoadTexture( const std::string& srTexturePath )
{
    // Check if texture was already loaded
    std::unordered_map< std::string, HTexture >::iterator it;

    it = aTexturePaths.find( srTexturePath );

    if ( it != aTexturePaths.end() )
        return it->second;

    // Not found, so try to load it
    std::string absPath = filesys->FindFile( srTexturePath );
    if ( absPath == "" && apMissingTex )
    {
        LogWarn( gGraphics2Channel, "Failed to Find Texture \"%s\"\n", srTexturePath.c_str() );
        return aMissingTexHandle;
    }

    std::string fileExt = filesys->GetFileExt( srTexturePath );

    for ( ITextureLoader* loader: gTextureLoaders )
    {
        if ( !loader->CheckExt( fileExt ) )
            continue;

        if ( Texture2* texture = loader->LoadTexture( absPath ) )
        {
            HTexture handle = aTextures.Add( texture );
            aTexturePaths[srTexturePath] = handle;

            UpdateImageSets();

            return handle;
        }
    }
	
    LogWarn( gGraphics2Channel, "Failed to open texture: %s\n", srTexturePath.c_str() );
	
    return aMissingTexHandle;
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

void Graphics::InitImGui() 
{
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
    init_info.CheckVkResultFn = CheckVKResult;

    // maybe don't use resolve on imgui renderpass???? idk
    ImGui_ImplVulkan_Init( &init_info, GetRenderPass( RenderPass_Color | RenderPass_Depth | RenderPass_Resolve ) );

    SingleCommand( []( VkCommandBuffer c ){ ImGui_ImplVulkan_CreateFontsTexture( c ); } );
    ImGui_ImplVulkan_DestroyFontUploadObjects();
}

/*
 *    Initializes the API.
 */
void Graphics::Init() 
{
    GetGInstance().Init();

    // create descriptor pool and layouts
    GetDescriptorManager();

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
    // GetDescriptorManager().CreateDescriptorPool();
    // CreateDescriptorSetLayouts();

    // init imgui (in between it? um ok)
    // InitImGui();
	
    CreateDrawThreads();
	
    matsys.Init();

	// TEMP TEMP:
    // Create Image Layout
    {
        VkDescriptorSetLayoutBinding imageBinding{};
        imageBinding.descriptorCount = MAX_IMAGES;
        imageBinding.descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        imageBinding.stageFlags      = VK_SHADER_STAGE_FRAGMENT_BIT;
        imageBinding.binding         = 0;

        VkDescriptorBindingFlagsEXT bindFlag = VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT_EXT | VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT_EXT;

        VkDescriptorSetLayoutBindingFlagsCreateInfoEXT extend{};
        extend.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO_EXT;
        extend.pNext = nullptr;
        extend.bindingCount = 1;
        extend.pBindingFlags = &bindFlag;

        VkDescriptorSetLayoutCreateInfo layoutInfo{  };
        layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutInfo.pNext = &extend;
        layoutInfo.bindingCount = 1;
        layoutInfo.pBindings = &imageBinding;

        CheckVKResult( vkCreateDescriptorSetLayout( GetDevice(), &layoutInfo, NULL, &aImageLayout ), "Failed to create image descriptor set layout!" );
    }
	
    AllocImageSets();

    // Create Built-In Missing Texture
    // CreateTexture();

    InitImGui();

    // TEMP
    freemans_tshirt.Init( &aImageSets, aImageLayout );
	
    AllocateCommands();
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


// other

// aTextureLoaders
void AddTextureLoader( ITextureLoader* spLoader )
{
    gTextureLoaders.push_back( spLoader );
}


// TODO: use descriptor manager instead of this?
void Graphics::AllocImageSets()
{
    size_t swapchainImageCount = GetSwapchain().GetImages().size();

    std::vector< VkDescriptorSetLayout > layouts( swapchainImageCount, aImageLayout );

    uint32_t counts[] = { MAX_IMAGES, MAX_IMAGES, MAX_IMAGES };
    VkDescriptorSetVariableDescriptorCountAllocateInfo dc{};
    dc.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_VARIABLE_DESCRIPTOR_COUNT_ALLOCATE_INFO;
    dc.descriptorSetCount = swapchainImageCount;
    dc.pDescriptorCounts  = counts;

    VkDescriptorSetAllocateInfo a{};
    a.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    a.pNext              = &dc;
    a.descriptorPool     = GetDescriptorManager().GetHandle();
    a.descriptorSetCount = swapchainImageCount;
    a.pSetLayouts        = layouts.data();

    aImageSets.resize( swapchainImageCount );
    CheckVKResult( vkAllocateDescriptorSets( GetDevice(), &a, aImageSets.data() ), "Failed to allocate descriptor sets!" );
}

void Graphics::UpdateImageSets()
{
    if ( !aTextures.size() )
        return;

    if ( aTextures.size() > MAX_IMAGES )
        LogFatal( gGraphics2Channel, "Over Max Images allocated (at %u, max is %u)", aTextures.size(), MAX_IMAGES );

    for ( uint32_t i = 0; i < aImageSets.size(); ++i )
    {
        std::vector< VkDescriptorImageInfo > infos;
        for ( uint32_t j = 0; j < aTextures.size(); ++j )
        {
            VkDescriptorImageInfo img{};
            img.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            img.imageView   = aTextures.GetByIndex( j )->GetImageView();
            img.sampler     = GetSampler();

            infos.push_back( img );
        }

        VkWriteDescriptorSet w{};
        w.sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        w.dstBinding       = 0;
        w.dstArrayElement  = 0;
        w.descriptorType   = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        w.descriptorCount  = aTextures.size();
        w.pBufferInfo      = 0;
        w.dstSet           = aImageSets[ i ];
        w.pImageInfo       = infos.data();

        vkUpdateDescriptorSets( GetDevice(), 1, &w, 0, nullptr);
    }
}

