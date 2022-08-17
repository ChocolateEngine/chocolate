#include "graphics.h"

#include "materialsystem.h"
#include "commandpool.h"
#include "instance.h"
#include "present.h"
#include "renderpass.h"
#include "descriptormanager.h"
#include "swapchain.h"
#include "rendertarget.h"
#include "rendergraph.h"
#include "config.hh"

#include "textures/missingtexture.h"
#include "textures/itextureloader.h"

// TEMP
#include "freemans_tshirt.hpp"

freemans_tshirt_t freemans_tshirt;

#include "imgui/imgui_impl_vulkan.h"
#include "imgui/imgui_impl_sdl.h"

LOG_REGISTER_CHANNEL( Graphics2, LogColor::DarkYellow );

Graphics graphics;

extern "C" 
{
    DLL_EXPORT void *cframework_get()
    {
        return &graphics;
    }
}


std::vector< ITextureLoader* > gTextureLoaders;
constexpr u32 MAX_IMAGES = 1000;


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
    // new render graph version
	

	// old below

    // RecordCommands();
    Present();
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
    // Have we loaded this model already?
    auto it = aModelPaths.find( srModelPath );

    if ( it != aModelPaths.end() )
    {
        // We have, use that model instead
        // it->second->AddRef();
        return it->second;
    }

    // We have not, so try to load this model in
    std::string fullPath = filesys->FindFile( srModelPath );

    if ( fullPath.empty() )
    {
        LogDev( gGraphics2Channel, 1, "LoadModel: Failed to Find Model: %s\n", srModelPath.c_str() );
        return InvalidHandle;
    }

    std::string fileExt = filesys->GetFileExt( srModelPath );

    Model_t* model = new Model_t;

    if ( fileExt == "obj" )
    {
        LoadObj( fullPath, model );
    }
    else if ( fileExt == "glb" || fileExt == "gltf" )
    {
        LoadGltf( fullPath, fileExt, model );
    }
    else
    {
        LogDev( gGraphics2Channel, 1, "Unknown Model File Extension: %s\n", fileExt.c_str() );
        return InvalidHandle;
    }

    //sModel->aRadius = glm::distance( mesh->aMinSize, mesh->aMaxSize ) / 2.0f;

    // TODO: load in an error model here instead
    if ( model->aSurfaces.size() == 0 )
    {
        delete model;
        return InvalidHandle;
    }

    HModel hModel = aModels.Add( model );
    aModelPaths[srModelPath] = hModel;

    return hModel;
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
 *   @param TextureCreateInfo_t&    The information about the texture.
 */
HTexture Graphics::CreateTexture( const TextureCreateInfo_t& srTextureCreateInfo )
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


// --------------------------------------------------------------------------------------------
// Render List
// --------------------------------------------------------------------------------------------


bool Graphics::CreateRenderGraph()
{
    if ( apRenderGraph )
    {
		LogWarn( gGraphics2Channel, "CreateRenderGraph(): RenderGraph already created\n" );
		return false;
    }

    apRenderGraph = new RenderGraph;
    return apRenderGraph != nullptr;
}


bool Graphics::FinishRenderGraph()
{
    if ( !apRenderGraph )
    {
		LogWarn( gGraphics2Channel, "FinishRenderGraph(): RenderGraph not created\n" );
        return;
    }

    if ( !apRenderGraph->Bake() )
    {
		LogWarn( gGraphics2Channel, "FinishRenderGraph(): RenderGraph bake failed\n" );
		return false;
    }

    // move render graph pointer
    apRenderGraphDraw = apRenderGraph;
    apRenderGraph = nullptr;

	return true;
}


HRenderPass Graphics::CreateRenderPass( const std::string& srName, RenderGraphQueueFlagBits sStage )
{
	if ( !apRenderGraph )
	{
		LogWarn( gGraphics2Channel, "CreateRenderPass(): RenderGraph not created\n" );
		return InvalidHandle;
	}

    return aPasses.Add( new RenderGraphPass( srName, sStage ) );
}


void Graphics::FreeRenderPass( HRenderPass sRenderPass )
{
    aPasses.Remove( sRenderPass );
}


void Graphics::AddRenderPass( HRenderPass sRenderPass )
{
	if ( !apRenderGraph )
	{
		LogWarn( gGraphics2Channel, "AddRenderPass(): RenderGraph not created\n" );
		return;
	}

	// get render pass data
	RenderGraphPass* renderPass = aPasses.Get( sRenderPass );
	if ( !renderPass )
	{
		LogWarn( gGraphics2Channel, "AddRenderPass(): Invalid RenderPass handle\n" );
		return;
	}

	apRenderGraph->AddRenderPass( sRenderPass, renderPass );
}


// helper check for valid render graph and render pass
RenderGraphPass* Graphics::GetRenderGraphPass( HRenderPass sRenderPass, const char* sFunc, const char* sError )
{
	if ( !apRenderGraph )
	{
		LogWarn( gGraphics2Channel, "%s: RenderGraph not created\n", sFunc );
		return nullptr;
	}

    RenderGraphPass* renderPass = apRenderGraph->GetRenderPass( sRenderPass );

    // check if renderPass is nullptr
    if ( renderPass == nullptr )
    {
        LogWarn( gGraphics2Channel, "%s: RenderPass is nullptr\n", sError );
        return nullptr;
    }

    return renderPass;
}


void Graphics::AddModel( HRenderPass sRenderPass, HModel sModel, const glm::mat4& sModelMatrix )
{
    RenderGraphPass* renderPass = GetRenderGraphPass( sRenderPass, "AddModel()", "Failed to add model to render pass" );
    if ( !renderPass )
        return;

	Model_t* model = aModels.Get( sModel );
	
    // check if model is nullptr
    if ( model == nullptr )
    {
        LogWarn( gGraphics2Channel, "Failed to add model to render list: Model_t is nullptr\n" );
        return;
    }

	// check if model has no surfaces
	if ( model->aSurfaces.size() == 0 )
    {
		LogWarn( gGraphics2Channel, "Failed to add model to render list: Model has no surfaces\n" );
		return;
	}

    // maybe use this idea of storing any type of data here? idk
    // maybe it could be some sort of "material var" like thing, but for shaders, "RenderVar" maybe idk
    // internally, each draw command can store pointers or indices to RenderVars,
    // then the shader could read the current RenderVars, and potentially get what it needs from them
    // ...downside is that it could be quite slow
	renderPass->SetData( "ModelMatrix", sModelMatrix );
	
    // aCmdHelper.AddPushConstants( &sModelMatrix, 0, sizeof( glm::mat4 ) );
}


void Graphics::AddViewportCamera( HRenderPass sRenderPass, const ViewportCamera_t& sViewportCamera )
{
    RenderGraphPass* renderPass = GetRenderGraphPass( sRenderPass, "AddViewportCamera()", "Failed to add viewport camera to render pass" );
    if ( !renderPass )
        return;

    // renderPass->AddViewportCamera( sViewportCamera );
}


// --------------------------------------------------------------------------------------------
// other
// --------------------------------------------------------------------------------------------

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

// --------------------------------------------------------------------------------------
// RANDOM IDEAS AND NOTES:
// 
// 
// // need to figure this out, hmm
// HScene scene = graphics->LoadScene( "protogen_wip_25d.glb" );
// 
// std::vector< HMesh > sceneMeshes;
// graphics->GetSceneMeshes( scene, sceneMeshes );
// 
// 
// // note: if the obj/glb has multiple meshes in it, this merges it all together
// // the LoadScene function will not, and keep it all intact
// HMesh mesh = graphics->LoadMesh( "protogen_wip_25d.obj" );
// 
// 
// HRenderable renderable = graphics->CreateRenderable();
// 
// 
// // something something set renderable model matrix idfk
// 
// 
// std::vector< std::string > meshMorphs;
// graphics->GetMeshMorphs( mesh, meshMorphs );
// 
// u16 visorMorph = graphics->GetMeshMorph( mesh, "LongVisor" );  // use std::string_view
// graphics->SetMeshMorphWeights( renderable, visorMorph, 1.f );
// 
// 
// graphics->AddToRenderGroup( renderGroup, mesh );
// 
// HRenderLayer renderLayer = graphics->CreateRenderLayer();
// graphics->AddToRenderLayer( renderLayer, renderGroup );
// 
// // probably drawn in order of everything being added?
// graphics->AddToRenderList( renderGroup );  // maybe, or just need to create one render layer to add stuff to
// graphics->AddToRenderList( renderLayer );
// 
// 
// 


// have part of the mesh be only what's needed to be loaded in the cpu cache for actually cmd buffer drawing
// and another part be for creating the render graph?


// No models maybe?
// in the hydra rendering thing, there is no "model" or "mesh", just the parts that make it up, buffers, textures, materials, etc.
// but how do you make this simple from a user-end standpoint (game code)?


// expose all parts needed to make a model in graphics, and not actually have any concept of a model in here
// instead, have the game roll it's own system for handling it
// (though you could make it a feature in graphics to make it easy, but have them not required)
// so the things to make your own "mesh format" is vertex/index buffer, and a material
// optionally allow other data to be added onto this mesh format
// technically could be put into another dll, like ch_modelloader.dll, idfk, probably not
// 
// the game will have a system to build a render graph for a model
// i need to look into how a "render graph" actually works, cause i still am very unsure on it
// or, we do keep graphics have a concept of a model, but ONLY use it in the high level abstraction
// so it's broken down into a bunch of render stages, and each stage has a list of resources it needs
// 

