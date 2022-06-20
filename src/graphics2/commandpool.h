/*
 *    commandpool.h    --    Wrapper for Vulkan Command Pool
 *
 *    Authored by Karl "p0lyh3dron" Kreuze on March 5, 2022
 *
 *    This file declares the wrapper for the Vulkan Command Pool.
 *    The command pool is used to allocate command buffers,
 *    and it is reccomended to use a 3 command pools per thread
 *    to allocate descriptors.
 */
#pragma once

#include <functional>

#include "gutil.hh"

class CommandPool
{
    VkCommandPool aCommandPool;
public:
     CommandPool();
    ~CommandPool();

    void Init( VkCommandPoolCreateFlags sFlags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT );
    void Reset();
    void Reset( VkCommandPoolResetFlags sFlags );

    VkCommandPool GetHandle() const;
    VkCommandPool GetHandle( VkCommandPoolResetFlags sFlags );
};

CommandPool& GetSingleTimeCommandPool();
void SingleCommand( std::function< void( VkCommandBuffer ) > sFunc );
