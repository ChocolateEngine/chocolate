/*
 *    renderpass.cpp    --    Wrapper for Vulkan Render Pass
 *
 *    Authored by Karl "p0lyh3dron" Kreuze on March 6, 2022
 *
 *    This file defines the wrapper for the Vulkan Render Pass.
 *    The render pass is used to create a render pipeline,
 *    and it is reccomended to use one for each independent draw
 *    stage (i.e color and depth first, then deferred, etc)
 */
#include "renderpass.h"

#include "config.hh"

#include "instance.h"
#include "swapchain.h"

VkFormat GetColorFormat()
{
    return GetSwapchain().GetFormat();
}

VkFormat GetDepthFormat()
{
    return VK_FORMAT_D32_SFLOAT;
}

RenderPass::RenderPass( const std::vector< VkAttachmentDescription >& srAttachments, 
                        const std::vector< VkSubpassDescription >&    srSubpasses, 
                        const std::vector< VkSubpassDependency >&     srDependencies,
                        RenderPassStage                               sStage )
{
    VkRenderPassCreateInfo renderPassInfo = {};
    renderPassInfo.sType                  = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount        = static_cast< uint32_t >( srAttachments.size() );
    renderPassInfo.pAttachments           = srAttachments.data();
    renderPassInfo.subpassCount           = static_cast< uint32_t >( srSubpasses.size() );
    renderPassInfo.pSubpasses             = srSubpasses.data();
    renderPassInfo.dependencyCount        = static_cast< uint32_t >( srDependencies.size() );
    renderPassInfo.pDependencies          = srDependencies.data();

    CheckVKResult( vkCreateRenderPass( GetLogicDevice(), &renderPassInfo, nullptr, &aRenderPass ), "Failed to create render pass!" );

    aStage = sStage;
}

RenderPass::~RenderPass()
{
    vkDestroyRenderPass( GetLogicDevice(), aRenderPass, nullptr );
}

std::vector< RenderPass* > &GetRenderPasses()
{
    static std::vector< RenderPass* > renderPasses;

    if ( renderPasses.size() )
        return renderPasses;

    /*
     *    Create the default color and depth render pass.
     *    This is the standard draw pass, and later, other
     *    passes may do deferred for example.
     */
    VkAttachmentDescription colorAttachment = {};
    colorAttachment.format                  = GetSwapchain().GetFormat();
    colorAttachment.samples                 = GetMSAASamples();
    colorAttachment.loadOp                  = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp                 = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp           = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp          = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout           = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout             = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    colorAttachment.samples                 = GetMSAASamples();

    VkAttachmentDescription depthAttachment = {};
    depthAttachment.format                  = GetDepthFormat();
    depthAttachment.samples                 = GetMSAASamples();
    depthAttachment.loadOp                  = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAttachment.storeOp                 = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.stencilLoadOp           = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depthAttachment.stencilStoreOp          = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.initialLayout           = VK_IMAGE_LAYOUT_UNDEFINED;
    depthAttachment.finalLayout             = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    depthAttachment.samples                 = GetMSAASamples();

    VkAttachmentDescription colorAttachmentResolve = {};
    colorAttachmentResolve.format                  = GetSwapchain().GetFormat();
    colorAttachmentResolve.samples                 = VK_SAMPLE_COUNT_1_BIT;
    colorAttachmentResolve.loadOp                  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachmentResolve.storeOp                 = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachmentResolve.stencilLoadOp           = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachmentResolve.stencilStoreOp          = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachmentResolve.initialLayout           = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachmentResolve.finalLayout             = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference colorAttachmentRef = {};
    colorAttachmentRef.attachment            = 0;
    colorAttachmentRef.layout                = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentReference depthAttachmentRef = {};
    depthAttachmentRef.attachment            = 1;
    depthAttachmentRef.layout                = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentReference colorAttachmentResolveRef = {};
    colorAttachmentResolveRef.attachment            = 2;
    colorAttachmentResolveRef.layout                = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass = {};
    subpass.pipelineBindPoint              = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount           = 1;
    subpass.pColorAttachments              = &colorAttachmentRef;
    subpass.pDepthStencilAttachment        = &depthAttachmentRef;
    subpass.pResolveAttachments            = &colorAttachmentResolveRef;

    VkSubpassDependency dependency = {};
    dependency.srcSubpass                  = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass                  = 0;
    dependency.srcStageMask                = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask               = 0;
    dependency.dstStageMask                = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstAccessMask               = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    std::vector< VkAttachmentDescription > attachments  = { colorAttachment, depthAttachment, colorAttachmentResolve };
    std::vector< VkSubpassDescription    > subpasses    = { subpass };
    std::vector< VkSubpassDependency     > dependencies = { dependency };

    /*
     *    TODO:
     *
     *    Either make this somehow not destructable or find a better way
     *    to do this.
     */
    auto pRenderPass = new RenderPass( attachments, subpasses, dependencies, RenderPass_Color | RenderPass_Depth | RenderPass_Resolve );

    renderPasses.push_back( pRenderPass );

    return renderPasses;
}

VkRenderPass GetRenderPass( RenderPassStage sStage )
{
    for ( auto &renderPass : GetRenderPasses() )
    {
        if ( renderPass->GetStage() == sStage )
            return renderPass->GetRenderPass();
    }
}