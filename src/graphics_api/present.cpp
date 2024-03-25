#include "core/platform.h"
#include "core/log.h"
#include "util.h"

#include "render_vk.h"

#include "imgui_impl_vulkan.h"

#include <thread>
#include <mutex>

#include "imgui.h"
#include "imgui_impl_vulkan.h"


constexpr u32                  MAX_FRAMES_IN_FLIGHT = 2;

// Primary Command Buffers
std::vector< VkCommandBuffer >  gCommandBuffers;
ResourceList< VkCommandBuffer > gCommandBufferHandles;  // wtf
VkCommandBuffer                 gSingleCommandBuffer;
VkCommandBuffer                 gSingleCommandBufferTransfer;

std::vector< VkSemaphore >      gImageAvailableSemaphores;
std::vector< VkSemaphore >      gRenderFinishedSemaphores;
std::vector< VkFence >          gFences;
std::vector< VkFence >          gInFlightFences;

u8                              gFrameIndex      = 0;

bool                            gInTransferQueue = false;
bool                            gInGraphicsQueue = false;

std::mutex                      gGraphicsMutex;


VkCommandBuffer VK_GetCommandBuffer( Handle cmd )
{
	return *gCommandBufferHandles.Get( cmd );
}


void VK_CheckFenceStatus()
{
	// VK_WaitForPresentQueue();

	VkResult result = VK_SUCCESS;

	result          = vkGetFenceStatus( VK_GetDevice(), gFences[ 0 ] );
	CH_ASSERT( result != VK_ERROR_DEVICE_LOST );

	if ( result == VK_ERROR_DEVICE_LOST )
		Log_Fatal( "fence 0 status device lost\n" );

	result = vkGetFenceStatus( VK_GetDevice(), gFences[ 1 ] );
	CH_ASSERT( result != VK_ERROR_DEVICE_LOST );

	if ( result == VK_ERROR_DEVICE_LOST )
		Log_Fatal( "fence 1 status device lost\n" );

	VK_CheckResult( vkDeviceWaitIdle( VK_GetDevice() ), "Failed waiting for device on fence status" );
}


void VK_CreateFences()
{
	gFences.resize( MAX_FRAMES_IN_FLIGHT );
	gInFlightFences.resize( VK_GetSwapImageCount() );

	VkFenceCreateInfo info{ VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
	info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	for ( u32 i = 0; i < MAX_FRAMES_IN_FLIGHT; i++ )
	{
		VK_CheckResult( vkCreateFence( VK_GetDevice(), &info, nullptr, &gFences[ i ] ), "Failed to create fence!" );
	}
}


void VK_DestroyFences()
{
	for ( auto& fence : gFences )
	{
		vkDestroyFence( VK_GetDevice(), fence, nullptr );
	}

	gFences.clear();
	gInFlightFences.clear();
}


void VK_CreateSemaphores()
{
	gImageAvailableSemaphores.resize( MAX_FRAMES_IN_FLIGHT );
	gRenderFinishedSemaphores.resize( MAX_FRAMES_IN_FLIGHT );

	VkSemaphoreCreateInfo info{ VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };

	for ( u32 i = 0; i < MAX_FRAMES_IN_FLIGHT; i++ )
	{
		VK_CheckResult( vkCreateSemaphore( VK_GetDevice(), &info, nullptr, &gImageAvailableSemaphores[ i ] ), "Failed to create semaphore!" );
		VK_CheckResult( vkCreateSemaphore( VK_GetDevice(), &info, nullptr, &gRenderFinishedSemaphores[ i ] ), "Failed to create semaphore!" );
	}
}


void VK_DestroySemaphores()
{
	for ( auto& semaphore : gImageAvailableSemaphores )
	{
		vkDestroySemaphore( VK_GetDevice(), semaphore, nullptr );
	}

	for ( auto& semaphore : gRenderFinishedSemaphores )
	{
		vkDestroySemaphore( VK_GetDevice(), semaphore, nullptr );
	}

	gImageAvailableSemaphores.clear();
	gRenderFinishedSemaphores.clear();
}


void VK_AllocateCommands()
{
	gCommandBuffers.resize( VK_GetSwapImageCount() );

	// Allocate primary command buffers
	VkCommandBufferAllocateInfo primAlloc{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
	primAlloc.pNext              = nullptr;
	primAlloc.commandPool        = VK_GetPrimaryCommandPool();
	primAlloc.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	primAlloc.commandBufferCount = gCommandBuffers.size();

	VK_CheckResult( vkAllocateCommandBuffers( VK_GetDevice(), &primAlloc, gCommandBuffers.data() ), "Failed to allocate primary command buffers" );

	// Allocate one time command buffer
	{
		VkCommandBufferAllocateInfo aCommandBufferAllocateInfo{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
		aCommandBufferAllocateInfo.pNext              = nullptr;
		aCommandBufferAllocateInfo.commandPool        = VK_GetSingleTimeCommandPool();
		aCommandBufferAllocateInfo.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		aCommandBufferAllocateInfo.commandBufferCount = 1;

		VK_CheckResult( vkAllocateCommandBuffers( VK_GetDevice(), &aCommandBufferAllocateInfo, &gSingleCommandBuffer ), "Failed to allocate command buffer!" );
	}
	
	// Allocate one time transfer command buffer
	{
		VkCommandBufferAllocateInfo aCommandBufferAllocateInfo{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
		aCommandBufferAllocateInfo.pNext              = nullptr;
		aCommandBufferAllocateInfo.commandPool        = VK_GetTransferCommandPool();
		aCommandBufferAllocateInfo.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		aCommandBufferAllocateInfo.commandBufferCount = 1;

		VK_CheckResult( vkAllocateCommandBuffers( VK_GetDevice(), &aCommandBufferAllocateInfo, &gSingleCommandBufferTransfer ), "Failed to allocate command buffer!" );
	}

	for ( const auto& cmd : gCommandBuffers )
		gCommandBufferHandles.Add( cmd );
}


void VK_FreeCommands()
{
	if ( !gCommandBuffers.empty() )
	{
		vkFreeCommandBuffers( VK_GetDevice(), VK_GetPrimaryCommandPool(), gCommandBuffers.size(), gCommandBuffers.data() );
		gCommandBuffers.clear();
		gCommandBufferHandles.clear();
	}

	if ( gSingleCommandBuffer )
		vkFreeCommandBuffers( VK_GetDevice(), VK_GetSingleTimeCommandPool(), 1, &gSingleCommandBuffer );

	if ( gSingleCommandBufferTransfer )
		vkFreeCommandBuffers( VK_GetDevice(), VK_GetTransferCommandPool(), 1, &gSingleCommandBufferTransfer );
}


VkCommandBuffer VK_BeginOneTimeTransferCommand()
{
	VkCommandBufferBeginInfo aCommandBufferBeginInfo{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
	aCommandBufferBeginInfo.pNext = nullptr;
	aCommandBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	VK_CheckResult( vkBeginCommandBuffer( gSingleCommandBufferTransfer, &aCommandBufferBeginInfo ), "Failed to begin command buffer!" );

	return gSingleCommandBufferTransfer;
}


void VK_EndOneTimeTransferCommand( VkCommandBuffer c )
{
	PROF_SCOPE();

	VK_CheckResult( vkEndCommandBuffer( c ), "Failed to end command buffer!" );

	VkSubmitInfo submitInfo{ VK_STRUCTURE_TYPE_SUBMIT_INFO };
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers    = &c;

	VK_WaitForTransferQueue();
	gGraphicsMutex.lock();

	VK_CheckResult( vkQueueSubmit( VK_GetTransferQueue(), 1, &submitInfo, VK_NULL_HANDLE ), "Failed to Submit OneTimeTransferCommand" );
	gInTransferQueue = true;

	VK_WaitForTransferQueue();
	gGraphicsMutex.unlock();

	VK_ResetCommandPool( VK_GetTransferCommandPool() );
}


VkCommandBuffer VK_BeginOneTimeCommand()
{
	VkCommandBufferBeginInfo aCommandBufferBeginInfo{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
	aCommandBufferBeginInfo.pNext = nullptr;
	aCommandBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	VK_CheckResult( vkBeginCommandBuffer( gSingleCommandBuffer, &aCommandBufferBeginInfo ), "Failed to begin command buffer!" );

	return gSingleCommandBuffer;
}


void VK_EndOneTimeCommand( VkCommandBuffer c )
{
	PROF_SCOPE();

	VK_CheckResult( vkEndCommandBuffer( c ), "Failed to end command buffer!" );

	VkSubmitInfo submitInfo{ VK_STRUCTURE_TYPE_SUBMIT_INFO };
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers    = &c;

	VK_WaitForGraphicsQueue();
	gGraphicsMutex.lock();

	VK_CheckResult( vkQueueSubmit( VK_GetGraphicsQueue(), 1, &submitInfo, VK_NULL_HANDLE ), "Failed to Submit OneTimeCommand" );
	gInGraphicsQueue = true;

	VK_WaitForGraphicsQueue();
	gGraphicsMutex.unlock();

	VK_ResetCommandPool( VK_GetSingleTimeCommandPool() );
}


// legacy?
void VK_OneTimeCommand( std::function< void( VkCommandBuffer ) > sFunc )
{
	VkCommandBuffer c = VK_BeginOneTimeCommand();
	sFunc( c );
	VK_EndOneTimeCommand( c );
}


VkCommandBuffer VK_BeginCommandBuffer( CommandBufferGroup_t& srGroup )
{
	return VK_NULL_HANDLE;
}


void VK_EndCommandBuffer( VkCommandBuffer sCmd )
{
}


void VK_ResetCommandBuffers( CommandBufferGroup_t& srGroup )
{
}


void VK_WaitForTransferQueue()
{
	PROF_SCOPE();

	if ( gInTransferQueue )
		VK_CheckResult( vkQueueWaitIdle( VK_GetTransferQueue() ), "Failed waiting for transfer queue" );

	gInTransferQueue = false;
}


void VK_WaitForGraphicsQueue()
{
	PROF_SCOPE();

	if ( gInGraphicsQueue )
		VK_CheckResult( vkQueueWaitIdle( VK_GetGraphicsQueue() ), "Failed waiting for graphics queue" );

	gInGraphicsQueue = false;
}


void VK_RecordCommands()
{
#if 0
	gGraphicsMutex.lock();

	VK_WaitForPresentQueue();
	VK_WaitForGraphicsQueue();
	VK_ResetCommandPool( VK_GetPrimaryCommandPool() );

	// For each framebuffer, begin a primary
    // command buffer, and record the commands.
	for ( gCmdIndex = 0; gCmdIndex < gCommandBuffers.size(); gCmdIndex++ )
	{
		VkCommandBufferBeginInfo begin{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
		begin.pNext            = nullptr;
		begin.flags            = 0;
		begin.pInheritanceInfo = nullptr;

		VK_CheckResult( vkBeginCommandBuffer( gCommandBuffers[ gCmdIndex ], &begin ), "Failed to begin recording command buffer!" );

		// Run Filter if needed
		// VK_RunFilterShader();

		VkRenderPassBeginInfo renderPassBeginInfo{ VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO };
		renderPassBeginInfo.pNext             = nullptr;
		renderPassBeginInfo.renderPass        = VK_GetRenderPass();
		renderPassBeginInfo.framebuffer       = VK_GetBackBuffer()->aFrameBuffers[ gCmdIndex ];
		renderPassBeginInfo.renderArea.offset = { 0, 0 };
		renderPassBeginInfo.renderArea.extent = VK_GetSwapExtent();

		VkClearValue clearValues[ 2 ];
		clearValues[ 0 ].color              = { gClearR, gClearG, gClearB, 1.0f };
		clearValues[ 1 ].depthStencil       = { 1.0f, 0 };

		renderPassBeginInfo.clearValueCount = ARR_SIZE( clearValues );
		renderPassBeginInfo.pClearValues    = clearValues;

		vkCmdBeginRenderPass( gCommandBuffers[ gCmdIndex ], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE );

		// Render Image
		// VK_BindImageShader();
		// VK_DrawImageShader();

		// Render ImGui
		ImGui::Render();
		ImGui_ImplVulkan_RenderDrawData( ImGui::GetDrawData(), gCommandBuffers[ gCmdIndex ] );

		vkCmdEndRenderPass( gCommandBuffers[ gCmdIndex ] );

		VK_CheckResult( vkEndCommandBuffer( gCommandBuffers[ gCmdIndex ] ), "Failed to end recording command buffer!" );
	}
#endif
}


u32 VK_GetNextImage()
{
	PROF_SCOPE();

	VK_CheckResult( vkWaitForFences( VK_GetDevice(), 1, &gFences[ gFrameIndex ], VK_TRUE, UINT64_MAX ), "Failed waiting for fences" );

	u32      imageIndex;
	VkResult res = vkAcquireNextImageKHR( VK_GetDevice(), VK_GetSwapchain(), UINT64_MAX, gImageAvailableSemaphores[ gFrameIndex ], VK_NULL_HANDLE, &imageIndex );

	if ( res == VK_ERROR_OUT_OF_DATE_KHR )
	{
		// Recreate all resources.
		printf( "VK_Reset - vkAcquireNextImageKHR\n" );
		VK_Reset();

		return VK_GetNextImage();
	}

	else if ( res != VK_SUCCESS && res != VK_SUBOPTIMAL_KHR )
	{
		// Classic typo must remain.
		VK_CheckResult( res, "Failed ot acquire swapchain image!" );
		return UINT_MAX;
	}

	return imageIndex;
}


void VK_Present( u32 sImageIndex )
{
	PROF_SCOPE();

	gInFlightFences[ sImageIndex ]          = gFences[ gFrameIndex ];

	VkSemaphore          waitSemaphores[]   = { gImageAvailableSemaphores[ gFrameIndex ] };
	VkSemaphore          signalSemaphores[] = { gRenderFinishedSemaphores[ gFrameIndex ] };
	VkPipelineStageFlags waitStages[]       = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	VkSubmitInfo         submitInfo{ VK_STRUCTURE_TYPE_SUBMIT_INFO };

	submitInfo.waitSemaphoreCount   = ARR_SIZE( waitSemaphores );
	submitInfo.pWaitSemaphores      = waitSemaphores;
	submitInfo.pWaitDstStageMask    = waitStages;
	submitInfo.commandBufferCount   = 1;
	submitInfo.pCommandBuffers      = &gCommandBuffers[ sImageIndex ];
	submitInfo.signalSemaphoreCount = ARR_SIZE( signalSemaphores );
	submitInfo.pSignalSemaphores    = signalSemaphores;

	VK_CheckResult( vkResetFences( VK_GetDevice(), 1, &gFences[ gFrameIndex ] ), "Failed to Reset Fences" );

	VK_CheckResult( vkQueueSubmit( VK_GetGraphicsQueue(), 1, &submitInfo, gFences[ gFrameIndex ] ), "Failed to submit draw command buffer!" );

	VkSwapchainKHR   swapChains[] = { VK_GetSwapchain() };

	VkPresentInfoKHR presentInfo{ VK_STRUCTURE_TYPE_PRESENT_INFO_KHR };
	presentInfo.pNext              = nullptr;
	presentInfo.waitSemaphoreCount = ARR_SIZE( signalSemaphores );
	presentInfo.pWaitSemaphores    = signalSemaphores;
	presentInfo.swapchainCount     = 1;
	presentInfo.pSwapchains        = swapChains;
	presentInfo.pImageIndices      = &sImageIndex;
	presentInfo.pResults           = nullptr;

	VkResult res                   = vkQueuePresentKHR( VK_GetGraphicsQueue(), &presentInfo );

	if ( res == VK_ERROR_OUT_OF_DATE_KHR || res == VK_SUBOPTIMAL_KHR )
	{
		printf( "VK_Reset - vkQueuePresentKHR\n" );
		VK_CheckResult( vkDeviceWaitIdle( VK_GetDevice() ), "Failed to wait for device to be idle" );
		VK_Reset();
	}
	else if ( res != VK_SUCCESS && res != VK_SUBOPTIMAL_KHR )
	{
		VK_CheckResult( res, "Failed to present swapchain image!" );
	}

	// gGraphicsMutex.unlock();

	gInGraphicsQueue = true;
	gFrameIndex      = ( gFrameIndex + 1 ) % MAX_FRAMES_IN_FLIGHT;
}

