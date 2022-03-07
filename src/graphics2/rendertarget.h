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

RenderTarget                       &CreateBackBuffer();

const std::vector< RenderTarget* > &GetRenderTargets();