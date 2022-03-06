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
#include "device.h"

CommandPool::CommandPool( VkCommandPoolCreateFlags sFlags )
{
    QueueFamilyIndices q = FindQueueFamilies( GetGDevice().GetPhysicalDevice() );

    VkCommandPoolCreateInfo aCommandPoolInfo = {};
    aCommandPoolInfo.sType                = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    aCommandPoolInfo.pNext                = nullptr;
    aCommandPoolInfo.flags                = sFlags;
    aCommandPoolInfo.queueFamilyIndex     = q.aGraphicsFamily;

    CheckVKResult( vkCreateCommandPool( GetGDevice().GetDevice(), &aCommandPoolInfo, nullptr, &aCommandPool ), "Failed to create command pool!" );
}

CommandPool::~CommandPool()
{
    vkDestroyCommandPool( GetGDevice().GetDevice(), aCommandPool, nullptr );
}

void CommandPool::Reset()
{
    Reset( 0 );
}
void CommandPool::Reset( VkCommandPoolResetFlags sFlags )
{
    CheckVKResult( vkResetCommandPool( GetGDevice().GetDevice(), aCommandPool, sFlags ), "Failed to reset command pool!" );
}

VkCommandPool CommandPool::GetHandle() const
{
    return aCommandPool;
}

VkCommandPool CommandPool::GetHandle( VkCommandPoolResetFlags sFlags )
{
    return aCommandPool;
}