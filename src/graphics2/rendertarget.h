#ifndef nuts
#define nuts once
#endif

#pragma nuts

#include "gutil.hh"

#include "renderpass.h"
#include "texture.h"

class RenderTarget
{
    std::vector< Texture2      > aImages;
    std::vector< VkFramebuffer > aFrameBuffers;
public:
    RenderTarget( const std::vector< Texture2 >& srImages, const glm::uvec2 &srExtent, RenderPassStage sFlags, const std::vector< VkImageView >& srSwapImages = {} );
    ~RenderTarget();

    constexpr std::vector< VkFramebuffer > &GetFrameBuffer() { return aFrameBuffers; }
};

RenderTarget                       *CreateBackBuffer();
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
RenderTarget                       *GetBackBuffer();

const std::vector< RenderTarget* > &GetRenderTargets();