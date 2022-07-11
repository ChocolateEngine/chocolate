#include "rendertarget.h"

#include "config.hh"

#include "swapchain.h"
#include "commandpool.h"

static std::vector< RenderTarget* >  gRenderTargets;
static RenderTarget                 *gpBackBuffer = nullptr;

RenderTarget::RenderTarget( const std::vector< Texture2 >& srImages, const glm::uvec2 &srExtent, RenderPassStage sStage, const std::vector< VkImageView > &srSwapImages )
{
    aImages.resize( srImages.size() );
    aFrameBuffers.resize( srSwapImages.size() );

    for ( size_t i = 0; i < srImages.size(); ++i )
        aImages[ i ] = srImages[ i ];

    for ( u32 i = 0; i < GetSwapchain().GetImageCount(); ++i )
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

        CheckVKResult( vkCreateFramebuffer( GetDevice(), &framebufferInfo, nullptr, &aFrameBuffers[ i ] ), "Failed to create framebuffer" );
    }

    gRenderTargets.push_back( this );
}

RenderTarget::~RenderTarget()
{
    for ( auto frameBuffer : aFrameBuffers )
        vkDestroyFramebuffer( GetDevice(), frameBuffer, nullptr );
}


// TEMP !!!!
// void SetImageLayout( VkImage sImage, VkImageLayout sOldLayout, VkImageLayout sNewLayout, VkImageAspectFlags sAspectMask, uint32_t sMipLevels )
void SetImageLayout( VkImage sImage, VkImageLayout sOldLayout, VkImageLayout sNewLayout, VkImageSubresourceRange& sSubresourceRange )
{
    VkImageMemoryBarrier barrier{};
    barrier.sType                           = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout                       = sOldLayout;
    barrier.newLayout                       = sNewLayout;
    barrier.srcQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
    barrier.image                           = sImage;
    barrier.subresourceRange 	            = sSubresourceRange;

    VkPipelineStageFlags 	sourceStage;
    VkPipelineStageFlags 	destinationStage;

#if 1
    switch ( sOldLayout )
    {
        case VK_IMAGE_LAYOUT_UNDEFINED:
		    barrier.srcAccessMask = 0;
		    sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		    break;
			
        case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
			break;
			
        default:
            LogFatal( "Unsupported old layout transition!\n" );
		    break;
    }

    switch ( sNewLayout )
    {
        case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
            barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
            break;

        case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
            destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
            break;

        case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
			barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
			destinationStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
			break;

        default:
            LogFatal( "Unsupported new layout transition!\n" );
            break;
    }

#else

    if ( sOldLayout == VK_IMAGE_LAYOUT_UNDEFINED && sNewLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL )
    {
        barrier.srcAccessMask 	= VK_ACCESS_NONE_KHR;
        barrier.dstAccessMask 	= VK_ACCESS_TRANSFER_WRITE_BIT;

        sourceStage 		= VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destinationStage	= VK_PIPELINE_STAGE_TRANSFER_BIT;
    }
    else if ( sOldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && sNewLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL )
    {
        barrier.srcAccessMask 	= VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask 	= VK_ACCESS_SHADER_READ_BIT;

        sourceStage 		= VK_PIPELINE_STAGE_TRANSFER_BIT;
        destinationStage 	= VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    }
    else if ( sOldLayout == VK_IMAGE_LAYOUT_UNDEFINED && sNewLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL )
    {
        barrier.srcAccessMask 	= VK_ACCESS_NONE_KHR;
        barrier.dstAccessMask 	= VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

        sourceStage 		= VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destinationStage 	= VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    }
    else
        LogFatal( "Unsupported layout transition!\n" );
#endif
	
    /* Submit to the GPU.  */
    SingleCommand( [ & ]( VkCommandBuffer c ){ vkCmdPipelineBarrier( c, sourceStage, destinationStage, 0, 0, NULL, 0, NULL, 1, &barrier ); } );
}


RenderTarget *CreateBackBuffer()
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
    color.samples               = GetMSAASamples();
    color.tiling                = VK_IMAGE_TILING_OPTIMAL;
    // DEMEZ TEST
    color.usage                 = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    // color.usage                 = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    // color.usage                 = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    // color.usage                 = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT;
    color.sharingMode           = VK_SHARING_MODE_EXCLUSIVE;
    color.queueFamilyIndexCount = 0;
    color.pQueueFamilyIndices   = nullptr;
    color.initialLayout         = VK_IMAGE_LAYOUT_UNDEFINED;

    CheckVKResult( vkCreateImage( GetDevice(), &color, nullptr, &colorTex.GetImage() ), "Failed to create color image!" );

    VkMemoryRequirements memReqs;
    vkGetImageMemoryRequirements( GetDevice(), colorTex.GetImage(), &memReqs );

    VkMemoryAllocateInfo allocInfo = {};
    allocInfo.sType                = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.pNext                = nullptr;
    allocInfo.allocationSize       = memReqs.size;
    allocInfo.memoryTypeIndex      = GetGInstance().GetMemoryType( memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT );

    CheckVKResult( vkAllocateMemory( GetDevice(), &allocInfo, nullptr, &colorTex.GetMemory() ), "Failed to allocate color image memory!" );

    vkBindImageMemory( GetDevice(), colorTex.GetImage(), colorTex.GetMemory(), 0 );
    
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

    CheckVKResult( vkCreateImageView( GetDevice(), &colorView, nullptr, &colorTex.GetImageView() ), "Failed to create color image view!" );

	// ------------------------------------------------------
	// Create Depth Texture

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
    depth.samples               = GetMSAASamples();
    depth.tiling                = VK_IMAGE_TILING_OPTIMAL;
    depth.usage                 = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    depth.sharingMode           = VK_SHARING_MODE_EXCLUSIVE;
    depth.queueFamilyIndexCount = 0;
    depth.pQueueFamilyIndices   = nullptr;
    depth.initialLayout         = VK_IMAGE_LAYOUT_UNDEFINED;

    CheckVKResult( vkCreateImage( GetDevice(), &depth, nullptr, &depthTex.GetImage() ), "Failed to create depth image!" );

    vkGetImageMemoryRequirements( GetDevice(), depthTex.GetImage(), &memReqs );
    
    allocInfo.allocationSize = memReqs.size;
    allocInfo.memoryTypeIndex = GetGInstance().GetMemoryType( memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT );
    
    CheckVKResult( vkAllocateMemory( GetDevice(), &allocInfo, nullptr, &depthTex.GetMemory() ), "Failed to allocate depth image memory!" );

    vkBindImageMemory( GetDevice(), depthTex.GetImage(), depthTex.GetMemory(), 0 );
    
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

    CheckVKResult( vkCreateImageView( GetDevice(), &depthView, nullptr, &depthTex.GetImageView() ), "Failed to create depth image view!" );

    // TEMP (doesn't actually seem to be needed in graphics 1?)
    // SetImageLayout( depthTex.GetImage(), VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, VK_IMAGE_ASPECT_DEPTH_BIT, depth.mipLevels );
    // SetImageLayout( colorTex.GetImage(), VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT, depth.mipLevels );
    // SetImageLayout( colorTex.GetImage(), VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT, color.mipLevels );

    std::vector< Texture2 > textures = { colorTex, depthTex };

    glm::uvec2 swapchainSize = { GetSwapchain().GetExtent().width, GetSwapchain().GetExtent().height };

	RenderTarget* rt = new RenderTarget( textures, swapchainSize, RenderPass_Color | RenderPass_Depth | RenderPass_Resolve, GetSwapchain().GetImageViews() );

    // SetImageLayout( depthTex.GetImage(), VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, depthView.subresourceRange );
    // SetImageLayout( colorTex.GetImage(), VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, colorView.subresourceRange );

    return rt;
}

/*
 *    Returns the backbuffer.
 *    The returned backbuffer contains framebuffers which
 *    are to be drawn to during command buffer recording.
 *    Previously, these were wrongly assumed to be the
 *    same as the swapchain images, but it turns out that
 *    this doesn't matter, it just needs something to draw to.
 * 
 *    @return RenderTarget *    The backbuffer.
 */
RenderTarget *GetBackBuffer()
{
    if ( !gpBackBuffer ){
        gpBackBuffer = CreateBackBuffer();
    }
    return gpBackBuffer;
}

const std::vector< RenderTarget* > &GetRenderTargets()
{
    return gRenderTargets;
}