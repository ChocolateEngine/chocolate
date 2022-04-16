/*
 *    commandpool.cpp    --    Wrapper for Vulkan Command Pool
 *
 *    Authored by Karl "p0lyh3dron" Kreuze on March 5, 2022
 *
 *    This file defines the wrapper for the Vulkan Command Pool.
 *    The command pool is used to allocate command buffers,
 *    and it is reccomended to use a 3 command pools per thread
 *    to allocate descriptors.
 */
#include "commandpool.h"

#include "gutil.hh"
#include "instance.h"

CommandPool gSingleTimeCommandPool;

CommandPool::CommandPool( VkCommandPoolCreateFlags sFlags )
{
    QueueFamilyIndices q = GetGInstance().FindQueueFamilies( GetPhysicalDevice() );

    VkCommandPoolCreateInfo aCommandPoolInfo = {};
    aCommandPoolInfo.sType                = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    aCommandPoolInfo.pNext                = nullptr;
    aCommandPoolInfo.flags                = sFlags;
    aCommandPoolInfo.queueFamilyIndex     = q.aGraphicsFamily;

    CheckVKResult( vkCreateCommandPool( GetLogicDevice(), &aCommandPoolInfo, nullptr, &aCommandPool ), "Failed to create command pool!" );
}

CommandPool::~CommandPool()
{
    vkDestroyCommandPool( GetLogicDevice(), aCommandPool, nullptr );
}

void CommandPool::Reset()
{
    Reset( 0 );
}
void CommandPool::Reset( VkCommandPoolResetFlags sFlags )
{
    CheckVKResult( vkResetCommandPool( GetLogicDevice(), aCommandPool, sFlags ), "Failed to reset command pool!" );
}

VkCommandPool CommandPool::GetHandle() const
{
    return aCommandPool;
}

VkCommandPool CommandPool::GetHandle( VkCommandPoolResetFlags sFlags )
{
    return aCommandPool;
}

void SingleCommand( std::function< void( VkCommandBuffer ) > sFunc )
{
    VkCommandBufferAllocateInfo aCommandBufferAllocateInfo = {};
    aCommandBufferAllocateInfo.sType                          = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    aCommandBufferAllocateInfo.pNext                          = nullptr;
    aCommandBufferAllocateInfo.commandPool                   = gSingleTimeCommandPool.GetHandle();
    aCommandBufferAllocateInfo.level                          = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    aCommandBufferAllocateInfo.commandBufferCount             = 1;

    VkCommandBuffer aCommandBuffer;
    CheckVKResult( vkAllocateCommandBuffers( GetLogicDevice(), &aCommandBufferAllocateInfo, &aCommandBuffer ), "Failed to allocate command buffer!" );

    VkCommandBufferBeginInfo aCommandBufferBeginInfo = {};
    aCommandBufferBeginInfo.sType                    = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    aCommandBufferBeginInfo.pNext                    = nullptr;
    aCommandBufferBeginInfo.flags                    = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    CheckVKResult( vkBeginCommandBuffer( aCommandBuffer, &aCommandBufferBeginInfo ), "Failed to begin command buffer!" );

    sFunc( aCommandBuffer );

    CheckVKResult( vkEndCommandBuffer( aCommandBuffer ), "Failed to end command buffer!" );
}