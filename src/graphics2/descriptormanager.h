#pragma once

#include "gutil.hh"

#include "core/core.h"

class DescriptorManager
{
    uint32_t aDescriptorCount;

    VkDescriptorPool aDescriptorPool;
    std::vector< VkDescriptorSet > aDescriptorSets;
    VkDescriptorSetLayout aImageLayout;
    VkDescriptorSetLayout aBufferLayout;

    void CreateDescriptorPool();
    void CreateDescriptorSetLayouts();
public:
     DescriptorManager( uint32_t sSets, VkDescriptorPoolCreateFlags sFlags = 0 );
    ~DescriptorManager();

    bool UpdateBuffer( Handle shBuf );
    void UpdateImage( Handle shImage );

    constexpr VkDescriptorPool      GetHandle()    { return aDescriptorPool; };

    constexpr VkDescriptorSetLayout GetImageSet()  { return aImageLayout;    };
    constexpr VkDescriptorSetLayout GetBufferSet() { return aBufferLayout;   };
};

DescriptorManager &GetDescriptorManager();