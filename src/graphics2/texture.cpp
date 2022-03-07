#include "texture.h"

#include "instance.h"

Texture2::Texture2()
{

}

Texture2::~Texture2()
{

}

VkSampler gSampler = VK_NULL_HANDLE;

VkSampler &GetSampler()
{
    if ( gSampler )
        return gSampler;

    VkSamplerCreateInfo samplerInfo{  };
	samplerInfo.sType  	     = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	samplerInfo.magFilter    = VK_FILTER_LINEAR;
	samplerInfo.minFilter    = VK_FILTER_LINEAR;
	samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;

	VkPhysicalDeviceProperties properties{  };
	vkGetPhysicalDeviceProperties( GetPhysicalDevice(), &properties );

	samplerInfo.anisotropyEnable 		    = VK_TRUE;
	samplerInfo.maxAnisotropy 		        = properties.limits.maxSamplerAnisotropy;
	samplerInfo.borderColor 		        = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	samplerInfo.unnormalizedCoordinates 	= VK_FALSE;
	samplerInfo.compareEnable 		        = VK_FALSE;
	samplerInfo.compareOp 			        = VK_COMPARE_OP_ALWAYS;
	samplerInfo.mipmapMode 			        = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	samplerInfo.mipLodBias 			        = 0.0f;
	samplerInfo.minLod 			            = 0.0f;
	samplerInfo.maxLod 			            = 1000.0f;

	CheckVKResult( vkCreateSampler( GetLogicDevice(), &samplerInfo, NULL, &gSampler ), "Failed to create sampler!" );

    return gSampler;
}