#include "descriptormanager.h"

#include "instance.h"
#include "swapchain.h"
#include "materialsystem.h"

void DescriptorManager::CreateDescriptorPool()
{
    VkDescriptorPoolSize aPoolSizes[] = { { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 4096 }, { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 4096 } };

    VkDescriptorPoolCreateInfo aDescriptorPoolInfo = {};
    aDescriptorPoolInfo.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    aDescriptorPoolInfo.poolSizeCount = ARR_SIZE( aPoolSizes );
    aDescriptorPoolInfo.pPoolSizes    = aPoolSizes;
    aDescriptorPoolInfo.maxSets       = aDescriptorCount;

    CheckVKResult( vkCreateDescriptorPool( GetDevice(), &aDescriptorPoolInfo, nullptr, &aDescriptorPool ), "Failed to create descriptor pool!" );
}

// TODO: Rethink this
void DescriptorManager::CreateDescriptorSetLayouts()
{
    VkDescriptorSetLayoutBinding aLayoutBindings[] = {
        { 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,         1, VK_SHADER_STAGE_VERTEX_BIT   | VK_SHADER_STAGE_FRAGMENT_BIT,   nullptr },
        { 0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_VERTEX_BIT,     nullptr }
    };

    VkDescriptorSetLayoutCreateInfo bufferLayout = {};
    bufferLayout.sType                           = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    bufferLayout.pNext                           = nullptr;
    bufferLayout.flags                           = 0;
    bufferLayout.bindingCount                    = 1;
    bufferLayout.pBindings                       = &aLayoutBindings[ 0 ];

    CheckVKResult( vkCreateDescriptorSetLayout( GetDevice(), &bufferLayout, nullptr, &aBufferLayout ), "Failed to create descriptor set layout!" );

    VkDescriptorSetLayoutCreateInfo imageLayout = {};
    imageLayout.sType                           = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    imageLayout.pNext                           = nullptr;
    imageLayout.flags                           = 0;
    imageLayout.bindingCount                    = 1;
    imageLayout.pBindings                       = &aLayoutBindings[ 1 ];

    CheckVKResult( vkCreateDescriptorSetLayout( GetDevice(), &imageLayout, nullptr, &aImageLayout ), "Failed to create descriptor set layout!" );    
}

DescriptorManager::DescriptorManager( uint32_t sSets, VkDescriptorPoolCreateFlags sFlags )
{
    aDescriptorCount   = sSets;

    CreateDescriptorPool();
    CreateDescriptorSetLayouts();
}

DescriptorManager::~DescriptorManager()
{
    vkDestroyDescriptorSetLayout( GetDevice(), aBufferLayout, nullptr );
    vkDestroyDescriptorSetLayout( GetDevice(), aImageLayout, nullptr );
    vkDestroyDescriptorPool( GetDevice(), aDescriptorPool, nullptr );
}

bool DescriptorManager::UpdateBuffer( Handle shBuf )
{
    return false;
}

/*
 *    unfinished
 */
void DescriptorManager::UpdateImage( Handle shImage )
{
    auto image = matsys.GetTexture( shImage );

    VkDescriptorImageInfo imageInfo = {};
    imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    imageInfo.imageView   = image->GetImageView();
    imageInfo.sampler     = image->GetSampler();

    for ( u32 i = 0; i < GetSwapchain().GetImageCount(); ++i ) {
        VkWriteDescriptorSet writeDescriptor = {};
        writeDescriptor.sType                  = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writeDescriptor.pNext                  = nullptr;
        writeDescriptor.dstSet                 = aDescriptorSets[ i ];
        writeDescriptor.dstBinding             = GET_HANDLE_INDEX( shImage );
        writeDescriptor.dstArrayElement        = GET_HANDLE_INDEX( shImage );
        
        writeDescriptor.descriptorType         = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        writeDescriptor.descriptorCount        = 1;
        writeDescriptor.pImageInfo             = &imageInfo;
        writeDescriptor.pBufferInfo            = nullptr;
        writeDescriptor.pTexelBufferView       = nullptr;

        vkUpdateDescriptorSets( GetDevice(), 1, &writeDescriptor, 0, nullptr );
    }
}

DescriptorManager &GetDescriptorManager()
{
    static DescriptorManager aDescriptorManager( 1024, 0 );
    
    return aDescriptorManager;
}