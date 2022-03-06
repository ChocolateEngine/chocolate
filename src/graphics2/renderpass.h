/*
 *    renderpass.h    --    Wrapper for Vulkan Render Pass
 *
 *    Authored by Karl "p0lyh3dron" Kreuze on March 5, 2022
 *
 *    This file declares the wrapper for the Vulkan Render Pass.
 *    The render pass is used to create a render pipeline,
 *    and it is reccomended to use one for each independent draw
 *    stage (i.e color and depth first, then deferred, etc)
 */
#pragma once

#include "gutil.hh"

class RenderPass
{
    VkRenderPass       aRenderPass;
    VkFormat           aFormat;
    VkExtent2D         aExtent;
    VkImageViewType    aViewType;
    VkImageAspectFlags aAspectFlags;
    VkImageView        aView;
public:
    RenderPass( const std::vector< VkAttachmentDescription > &srAttachments, 
                const std::vector< VkSubpassDescription >    &srSubpasses, 
                const std::vector< VkSubpassDependency >     &srDependencies );
    ~RenderPass();

    const VkRenderPass& GetRenderPass() const;
};

const std::vector< RenderPass > &GetRenderPasses();