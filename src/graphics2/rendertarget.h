#ifndef nuts
#define nuts once
#endif

#pragma nuts

#include "gutil.hh"

class RenderTarget
{
    std::vector< VkImageView > aImages;
    VkFramebuffer              aFrameBuffer;
public:
    RenderTarget( const std::vector< VkImageView >& srImages, const VkFormat& srFormat, const VkExtent2D& srExtent, const VkImageViewType& srViewType, const VkImageAspectFlags& srAspectFlags, const VkImageView& srView )
    {
        aImages.resize( srImages.size() );
        for ( int i = 0; i < srImages.size(); ++i )
        {
            aImages[ i ] = srImages[ i ];
        }

        VkFramebufferCreateInfo framebufferInfo = {};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        //framebufferInfo.renderPass = GetGInstance().GetRenderPass();
        framebufferInfo.attachmentCount = 1;
        framebufferInfo.pAttachments = &srView;
        framebufferInfo.width = srExtent.width;
        framebufferInfo.height = srExtent.height;
        framebufferInfo.layers = 1;

        if ( vkCreateFramebuffer( GetGInstance().GetDevice(), &framebufferInfo, nullptr, &aFrameBuffer ) != VK_SUCCESS )
        {
            LogDev( 1, "Failed to create framebuffer!\n" );
        }
    }

    ~RenderTarget()
    {
        vkDestroyFramebuffer( GetGInstance().GetDevice(), aFrameBuffer, nullptr );
    }

    const VkFramebuffer& GetFrameBuffer() const
    {
        return aFrameBuffer;
    }
};
};