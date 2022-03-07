#pragma once

#include <glm/glm.hpp>

#include "gutil.hh"

class Texture2 {
    glm::ivec2     aExtent;

    VkImage        aImage;
    VkImageView    aImageView;
    VkDeviceMemory aMemory;
    VkSampler      aSampler;
public:
     Texture2();
    ~Texture2();

    constexpr glm::ivec2     &GetExtent()       { return aExtent;    }

    constexpr VkImage        &GetImage()        { return aImage;     }
    constexpr VkImageView    &GetImageView()    { return aImageView; }
    constexpr VkDeviceMemory &GetMemory()       { return aMemory;    }
    constexpr VkSampler      &GetSampler()      { return aSampler;   }
};

VkSampler &GetSampler();