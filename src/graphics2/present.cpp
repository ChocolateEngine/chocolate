#include "present.h"

#include <thread>

#include "gutil.hh"

#include "commandpool.h"
#include "swapchain.h"
#include "rendertarget.h"
#include "renderpass.h"

#include "imgui/imgui.h"
#include "imgui/imgui_impl_vulkan.h"

class DrawThread
{
public:
    CommandPool                    aCommandPool;
    std::vector< VkCommandBuffer > aCommandBuffers;
};

constexpr u32 MAX_FRAMES_IN_FLIGHT = 2;
constexpr u32 DRAW_THREADS         = 1;

/*
 *    All draw commands in threads are in secondary
 *    commands buffers.
 */
std::vector< DrawThread >      gDrawThreads;
/*
 *    These are the primary command buffers,
 *    allocated by a primary command pool.
 */
std::vector< VkCommandBuffer > gCommandBuffers;

CommandPool& GetPrimaryCommandPool()
{
	static CommandPool sPrimaryCommandPool;
	return sPrimaryCommandPool;
}

std::vector< VkCommandBuffer > gImGuiCommandBuffers; 

std::vector< VkSemaphore > gImageAvailableSemaphores;
std::vector< VkSemaphore > gRenderFinishedSemaphores;
std::vector< VkFence >     gFences;
std::vector< VkFence >     gInFlightFences;

u32 gFrameIndex = 0;

void CreateFences() 
{
    gFences.resize( MAX_FRAMES_IN_FLIGHT );
    gInFlightFences.resize( GetSwapchain().GetImageCount() );

    VkFenceCreateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for( u32 i = 0; i < MAX_FRAMES_IN_FLIGHT; i++ )
    {
        CheckVKResult( vkCreateFence( GetDevice(), &info, nullptr, &gFences[ i ] ), "Failed to create fence!" );
    }
}

void CreateSemaphores()
{
    gImageAvailableSemaphores.resize( MAX_FRAMES_IN_FLIGHT );
    gRenderFinishedSemaphores.resize( MAX_FRAMES_IN_FLIGHT );

    VkSemaphoreCreateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    for( u32 i = 0; i < MAX_FRAMES_IN_FLIGHT; i++ )
    {
        CheckVKResult( vkCreateSemaphore( GetDevice(), &info, nullptr, &gImageAvailableSemaphores[ i ] ), "Failed to create semaphore!" );
        CheckVKResult( vkCreateSemaphore( GetDevice(), &info, nullptr, &gRenderFinishedSemaphores[ i ] ), "Failed to create semaphore!" );
    }
}

void CreateDrawThreads()
{
    gDrawThreads.resize( DRAW_THREADS );

    CreateFences();
    CreateSemaphores();
}

void RecordImGuiCommands( u32 sCmdIndex,  VkCommandBufferInheritanceInfo sInfo )
{
    // TEMP
    ImGui::Render();
	
    VkCommandBufferAllocateInfo allocInfo = {};
    allocInfo.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool        = GetPrimaryCommandPool().GetHandle();
    allocInfo.level              = VK_COMMAND_BUFFER_LEVEL_SECONDARY;
    allocInfo.commandBufferCount = 1;

    CheckVKResult( vkAllocateCommandBuffers( GetDevice(), &allocInfo, &gImGuiCommandBuffers[ sCmdIndex ] ), "Failed to allocate command buffer!" );

    VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT;
    beginInfo.pInheritanceInfo = &sInfo;

    CheckVKResult( vkBeginCommandBuffer( gImGuiCommandBuffers[ sCmdIndex ], &beginInfo ), "Failed to begin command buffer!" );

    auto stuff = ImGui::GetDrawData();
    stuff->DisplaySize = ImVec2( GetSwapchain().GetExtent().width, GetSwapchain().GetExtent().height );

    ImGui_ImplVulkan_RenderDrawData( stuff, gImGuiCommandBuffers[ sCmdIndex ] );

    CheckVKResult( vkEndCommandBuffer( gImGuiCommandBuffers[ sCmdIndex ] ), "Failed to end command buffer!" );
}

void RecordSecondaryCommands( u32 sThreadIndex, u32 cmdIndex, VkCommandBufferInheritanceInfo sInfo )
{
    VkCommandBufferBeginInfo begin{};
    begin.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin.pNext = nullptr;
    begin.flags = VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT;
    begin.pInheritanceInfo = &sInfo;

    CheckVKResult( vkBeginCommandBuffer( gDrawThreads[ sThreadIndex ].aCommandBuffers[ cmdIndex ], &begin ), "Failed to begin secondary command buffer" );

    /*
     *    Add viewports and scissors to the command buffer in the future.
     */

    /*
     *    Record draw commands.
     *    For each material, bind the pipeline, push constants, descriptor sets,
     *    then group draw commands and submit an indirect draw call using one vertex buffer.
     */

    CheckVKResult( vkEndCommandBuffer( gDrawThreads[ sThreadIndex ].aCommandBuffers[ cmdIndex ] ), "Failed to end secondary command buffer" );
}

void RecordCommands() 
{
    gCommandBuffers.resize( GetSwapchain().GetImageCount() );
    gImGuiCommandBuffers.resize( GetSwapchain().GetImageCount() );

    /*
     *    Allocate primary command buffers
     */
    VkCommandBufferAllocateInfo primAlloc{};
    primAlloc.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    primAlloc.pNext              = nullptr;
    primAlloc.commandPool        = GetPrimaryCommandPool().GetHandle();
    primAlloc.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    primAlloc.commandBufferCount = gCommandBuffers.size();

    CheckVKResult( vkAllocateCommandBuffers( GetDevice(), &primAlloc, gCommandBuffers.data() ), "Failed to allocate primary command buffers" );

    /*
     *    For each draw thread, allocate secondary
     *    command buffers.
     * 
     *    Will be deprecated in the future.
     */
    for ( u32 i = 0; i < DRAW_THREADS; i++ )
    {
        gDrawThreads[ i ].aCommandBuffers.resize( GetSwapchain().GetImageCount() );

        VkCommandBufferAllocateInfo aCommandBufferAllocateInfo = {};
        aCommandBufferAllocateInfo.sType                    = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        aCommandBufferAllocateInfo.pNext                    = nullptr;
        aCommandBufferAllocateInfo.commandPool              = gDrawThreads[ i ].aCommandPool.GetHandle();
        aCommandBufferAllocateInfo.level                    = VK_COMMAND_BUFFER_LEVEL_SECONDARY;
        aCommandBufferAllocateInfo.commandBufferCount       = gDrawThreads[ i ].aCommandBuffers.size();

        CheckVKResult( vkAllocateCommandBuffers( GetDevice(), &aCommandBufferAllocateInfo, gDrawThreads[ i ].aCommandBuffers.data() ), "Failed to allocate command buffers!" );
    }
    
    /*
     *    For each framebuffer, allocate a primary
     *    command buffer, and record the commands.
     */
    for ( u32 i = 0; i < gCommandBuffers.size(); ++i ) 
    {
        VkCommandBufferBeginInfo begin{};
        begin.sType            = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        begin.pNext            = nullptr;
        begin.flags            = 0;
        begin.pInheritanceInfo = nullptr;

        CheckVKResult( vkBeginCommandBuffer( gCommandBuffers[ i ], &begin ), "Failed to begin recording command buffer!" );

        VkRenderPassBeginInfo renderPassBeginInfo{};
        renderPassBeginInfo.sType             = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassBeginInfo.pNext             = nullptr;
        renderPassBeginInfo.renderPass        = GetRenderPass( RenderPass_Color | RenderPass_Depth | RenderPass_Resolve );
        renderPassBeginInfo.framebuffer       = GetBackBuffer()->GetFrameBuffer()[ i ];
        renderPassBeginInfo.renderArea.offset = { 0, 0 };
        renderPassBeginInfo.renderArea.extent = GetSwapchain().GetExtent();

        VkClearValue clearValues[ 2 ];
        clearValues[ 0 ].color        = { 0.0f, 0.0f, 0.0f, 1.0f };
        clearValues[ 1 ].depthStencil = { 1.0f, 0 };

        renderPassBeginInfo.clearValueCount = ARR_SIZE( clearValues );
        renderPassBeginInfo.pClearValues    = clearValues;

        vkCmdBeginRenderPass( gCommandBuffers[ i ], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS );

        /*
         *    Record commmands in secondary command buffers.
         */
        VkCommandBufferInheritanceInfo inherit{};
        inherit.sType                   = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO;
        inherit.pNext                   = nullptr;
        inherit.renderPass              = GetRenderPass( RenderPass_Color | RenderPass_Depth | RenderPass_Resolve );
        inherit.subpass                 = 0;
        inherit.framebuffer             = GetBackBuffer()->GetFrameBuffer()[ i ];

        for ( u32 j = 0; j < DRAW_THREADS; j++ )
        {
            /*
             *    Split resources and async this.
             */
            RecordSecondaryCommands( j, i, inherit );
        }

        /*
         *    Consolidate all commands into a vector for submission.
         */
        std::vector< VkCommandBuffer > cmds;
        for ( u32 j = 0; j < DRAW_THREADS; j++ )
        {
            cmds.push_back( gDrawThreads[ j ].aCommandBuffers[ i ] );
        }

        /*
         *    Render UI.
         */
        // ImGui::Render();
        /*
         *    Run ImGui commands.
         */
        RecordImGuiCommands( i, inherit );
        cmds.push_back( gImGuiCommandBuffers[ i ] );

        vkCmdExecuteCommands( gCommandBuffers[ i ], cmds.size(), cmds.data() );

        vkCmdEndRenderPass( gCommandBuffers[ i ] );

        CheckVKResult( vkEndCommandBuffer( gCommandBuffers[ i ] ), "Failed to end recording command buffer!" );
    }
}

void Present()
{
    vkWaitForFences( GetDevice(), 1, &gFences[ gFrameIndex ], VK_TRUE, UINT64_MAX );

    u32 imageIndex;
    
    VkResult res = vkAcquireNextImageKHR( GetDevice(), GetSwapchain().GetHandle(), UINT64_MAX, gImageAvailableSemaphores[ gFrameIndex ], VK_NULL_HANDLE, &imageIndex );

    if ( res == VK_ERROR_OUT_OF_DATE_KHR )
    {
        /*
         *    Recreate all resources.
         */
        return;
    }
    else if ( res != VK_SUCCESS && res != VK_SUBOPTIMAL_KHR )
    {
        /*
         *    Classic typo must remain.
         */
        CheckVKResult( res, "Failed ot acquire swapchain image!" );
    }

    if ( gInFlightFences[ imageIndex ] != VK_NULL_HANDLE )
    {
        vkWaitForFences( GetDevice(), 1, &gInFlightFences[ imageIndex ], VK_TRUE, UINT64_MAX );
    }

    gInFlightFences[ imageIndex ] = gFences[ gFrameIndex ];

    VkSubmitInfo submitInfo{};
    submitInfo.sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    VkSemaphore          waitSemaphores[]     = { gImageAvailableSemaphores[ gFrameIndex ] };
    VkSemaphore          signalSemaphores[]   = { gRenderFinishedSemaphores[ gFrameIndex ] };
    VkPipelineStageFlags waitStages[]         = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };

    submitInfo.waitSemaphoreCount   = ARR_SIZE( waitSemaphores );
    submitInfo.pWaitSemaphores      = waitSemaphores;
    submitInfo.pWaitDstStageMask    = waitStages;
    submitInfo.commandBufferCount   = 1;
    submitInfo.pCommandBuffers      = &gCommandBuffers[ imageIndex ];
    submitInfo.signalSemaphoreCount = ARR_SIZE( signalSemaphores );
    submitInfo.pSignalSemaphores    = signalSemaphores;

    vkResetFences( GetDevice(), 1, &gFences[ gFrameIndex ] );

    CheckVKResult( vkQueueSubmit( GetGInstance().GetGraphicsQueue(), 1, &submitInfo, gFences[ gFrameIndex ] ), "Failed to submit draw command buffer!" );

    VkSwapchainKHR swapChains[] = { GetSwapchain().GetHandle() };

    VkPresentInfoKHR presentInfo{};
    presentInfo.sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.pNext              = nullptr;
    presentInfo.waitSemaphoreCount = ARR_SIZE( signalSemaphores );
    presentInfo.pWaitSemaphores    = signalSemaphores;
    presentInfo.swapchainCount     = ARR_SIZE( swapChains );
    presentInfo.pSwapchains        = swapChains;
    presentInfo.pImageIndices      = &imageIndex;
    presentInfo.pResults           = nullptr;

    res = vkQueuePresentKHR( GetGInstance().GetGraphicsQueue(), &presentInfo );

    if ( res == VK_ERROR_OUT_OF_DATE_KHR || res == VK_SUBOPTIMAL_KHR )
    {
        /*
         *    Recreate all resources.
         */
        return;
    }
    else if ( res != VK_SUCCESS && res != VK_SUBOPTIMAL_KHR )
    {
        CheckVKResult( res, "Failed to present swapchain image!" );
    }

    vkQueueWaitIdle( GetGInstance().GetPresentQueue() );

    gFrameIndex = ( gFrameIndex + 1 ) % MAX_FRAMES_IN_FLIGHT;
    
    vkResetCommandPool( GetDevice(), GetPrimaryCommandPool().GetHandle(), 0);
    for ( u32 i = 0; i < DRAW_THREADS; i++ )
    {
        vkResetCommandPool( GetDevice(), gDrawThreads[ i ].aCommandPool.GetHandle(), 0 );
    }
}