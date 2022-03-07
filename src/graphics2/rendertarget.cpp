#include "rendertarget.h"

#include "swapchain.h"

static std::vector< RenderTarget* > gRenderTargets;

RenderTarget::RenderTarget( const std::vector< Texture2 >& srImages, const glm::uvec2 &srExtent, RenderPassStage sStage, const std::vector< VkImageView > &srSwapImages )
{
    aImages.resize( srImages.size() );
    for ( int i = 0; i < srImages.size(); ++i )
        aImages[ i ] = srImages[ i ];

    for ( int i = 0; i < GetSwapchain().GetImageCount(); ++i )
    {
        std::vector< VkImageView > attachments;
        for ( auto image : srImages )
            attachments.push_back( image.GetImageView() );

        if ( srSwapImages.size() )
            attachments.push_back( GetSwapchain().GetImageViews()[ i ] );

        VkFramebufferCreateInfo framebufferInfo = {};
        framebufferInfo.sType                   = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass              = GetRenderPass( sStage );
        framebufferInfo.attachmentCount         = attachments.size();
        framebufferInfo.pAttachments            = attachments.data();
        framebufferInfo.width                   = srExtent.x;
        framebufferInfo.height                  = srExtent.y;
        framebufferInfo.layers                  = 1;

        CheckVKResult( vkCreateFramebuffer( GetLogicDevice(), &framebufferInfo, nullptr, &aFrameBuffers[ i ] ), "Failed to create framebuffer" );
    }

    gRenderTargets.push_back( this );
}

RenderTarget::~RenderTarget()
{
    for ( auto frameBuffer : aFrameBuffers )
        vkDestroyFramebuffer( GetLogicDevice(), frameBuffer, nullptr );
}

RenderTarget &CreateBackBuffer()
{
    /*
     *    Our backbuffer contains 3 render operations: Color, Depth, and Resolve,
     *    so we'll make those now.
     */
    Texture2 colorTex{};
    VkImageCreateInfo color;
    color.sType                 = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    color.pNext                 = nullptr;
    color.flags                 = 0;
    color.imageType             = VK_IMAGE_TYPE_2D;
    color.format                = GetSwapchain().GetFormat();
    color.extent.width          = GetSwapchain().GetExtent().width;
    color.extent.height         = GetSwapchain().GetExtent().height;
    color.extent.depth          = 1;
    color.mipLevels             = 1;
    color.arrayLayers           = 1;
    color.samples               = VK_SAMPLE_COUNT_1_BIT;
    color.tiling                = VK_IMAGE_TILING_OPTIMAL;
    color.usage                 = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    color.sharingMode           = VK_SHARING_MODE_EXCLUSIVE;
    color.queueFamilyIndexCount = 0;
    color.pQueueFamilyIndices   = nullptr;
    color.initialLayout         = VK_IMAGE_LAYOUT_UNDEFINED;

    CheckVKResult( vkCreateImage( GetLogicDevice(), &color, nullptr, &colorTex.GetImage() ), "Failed to create color image!" );

    VkMemoryRequirements memReqs;
    vkGetImageMemoryRequirements( GetLogicDevice(), colorTex.GetImage(), &memReqs );

    VkMemoryAllocateInfo allocInfo = {};
    allocInfo.sType                = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.pNext                = nullptr;
    allocInfo.allocationSize       = memReqs.size;
    allocInfo.memoryTypeIndex      = GetGInstance().GetMemoryType( memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT );

    CheckVKResult( vkAllocateMemory( GetLogicDevice(), &allocInfo, nullptr, &colorTex.GetMemory() ), "Failed to allocate color image memory!" );
    
    VkImageViewCreateInfo colorView;
    colorView.sType         = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    colorView.pNext         = nullptr;
    colorView.flags         = 0;
    colorView.image         = colorTex.GetImage();
    colorView.viewType      = VK_IMAGE_VIEW_TYPE_2D;
    colorView.format        = color.format;
    colorView.components    = { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A };
    colorView.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
    colorView.subresourceRange.baseMipLevel   = 0;
    colorView.subresourceRange.levelCount     = 1;
    colorView.subresourceRange.baseArrayLayer = 0;
    colorView.subresourceRange.layerCount     = 1;

    CheckVKResult( vkCreateImageView( GetLogicDevice(), &colorView, nullptr, &colorTex.GetImageView() ), "Failed to create color image view!" );

    Texture2 depthTex{};
    VkImageCreateInfo depth;
    depth.sType                 = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    depth.pNext                 = nullptr;
    depth.flags                 = 0;
    depth.imageType             = VK_IMAGE_TYPE_2D;
    depth.format                = VK_FORMAT_D32_SFLOAT;
    depth.extent.width          = GetSwapchain().GetExtent().width;
    depth.extent.height         = GetSwapchain().GetExtent().height;
    depth.extent.depth          = 1;
    depth.mipLevels             = 1;
    depth.arrayLayers           = 1;
    depth.samples               = VK_SAMPLE_COUNT_1_BIT;
    depth.tiling                = VK_IMAGE_TILING_OPTIMAL;
    depth.usage                 = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    depth.sharingMode           = VK_SHARING_MODE_EXCLUSIVE;
    depth.queueFamilyIndexCount = 0;
    depth.pQueueFamilyIndices   = nullptr;
    depth.initialLayout         = VK_IMAGE_LAYOUT_UNDEFINED;

    CheckVKResult( vkCreateImage( GetLogicDevice(), &depth, nullptr, &depthTex.GetImage() ), "Failed to create depth image!" );

    vkGetImageMemoryRequirements( GetLogicDevice(), depthTex.GetImage(), &memReqs );
    
    allocInfo.allocationSize = memReqs.size;
    allocInfo.memoryTypeIndex = GetGInstance().GetMemoryType( memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT );
    
    CheckVKResult( vkAllocateMemory( GetLogicDevice(), &allocInfo, nullptr, &depthTex.GetMemory() ), "Failed to allocate depth image memory!" );
    
    VkImageViewCreateInfo depthView;
    depthView.sType         = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    depthView.pNext         = nullptr;
    depthView.flags         = 0;
    depthView.image         = depthTex.GetImage();
    depthView.viewType      = VK_IMAGE_VIEW_TYPE_2D;
    depthView.format        = depth.format;
    depthView.components    = { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A };
    depthView.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_DEPTH_BIT;
    depthView.subresourceRange.baseMipLevel   = 0;
    depthView.subresourceRange.levelCount     = 1;
    depthView.subresourceRange.baseArrayLayer = 0;
    depthView.subresourceRange.layerCount     = 1;

    CheckVKResult( vkCreateImageView( GetLogicDevice(), &depthView, nullptr, &depthTex.GetImageView() ), "Failed to create depth image view!" );

    std::vector< Texture2 > textures = { colorTex, depthTex };

    glm::uvec2 swapchainSize = { GetSwapchain().GetExtent().width, GetSwapchain().GetExtent().height };

    static RenderTarget backBuffer( textures, swapchainSize, RenderPass_Color | RenderPass_Depth | RenderPass_Resolve, GetSwapchain().GetImageViews() );
    return backBuffer;
}

const std::vector< RenderTarget* > &GetRenderTargets()
{
    return gRenderTargets;
}