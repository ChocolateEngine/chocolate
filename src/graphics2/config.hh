#pragma once

#include "core/console.h"

#include <string.h>
#include <vulkan/vulkan.h>

extern VkSampleCountFlagBits gMaxSamples;

struct GOption{
    char const* apName;
    int         aVal;
};

static GOption gOptions[] = {
    { "Width",              1280                  },
    { "Height",             720                   },
    { "MSAA Samples",       VK_SAMPLE_COUNT_1_BIT },
    { "VSync",              1                     },
    { "Buffered Swapchain", 1                     }
};

constexpr int GetOption( char const* apName ) {
    for( auto& option : gOptions ) {
        if( strcmp( apName, option.apName ) == 0 ) {
            return option.aVal;
        }
    }
    return 0;
}

constexpr void SetOption( char const* apName, int aVal ) {
    for( auto& option : gOptions ) {
        if( strcmp( apName, option.apName ) == 0 ) {
            option.aVal = aVal;
            return;
        }
    }
}

constexpr VkSampleCountFlagBits GetMSAASamples() {
    int samples = GetOption( "MSAA Samples" );
    if( samples < VK_SAMPLE_COUNT_2_BIT && samples <= gMaxSamples ) {
        return VK_SAMPLE_COUNT_1_BIT;
    }
    if( samples < VK_SAMPLE_COUNT_4_BIT && samples <= gMaxSamples ) {
        return VK_SAMPLE_COUNT_2_BIT;
    }
    if( samples < VK_SAMPLE_COUNT_8_BIT && samples <= gMaxSamples ) {
        return VK_SAMPLE_COUNT_4_BIT;
    }
    if( samples < VK_SAMPLE_COUNT_16_BIT && samples <= gMaxSamples ) {
        return VK_SAMPLE_COUNT_8_BIT;
    }
    if( samples < VK_SAMPLE_COUNT_32_BIT && samples <= gMaxSamples ) {
        return VK_SAMPLE_COUNT_16_BIT;
    }
    if( samples < VK_SAMPLE_COUNT_64_BIT && samples <= gMaxSamples ) {
        return VK_SAMPLE_COUNT_32_BIT;
    }
    return VK_SAMPLE_COUNT_1_BIT;
}

static CONVAR( g_width,  1280 );
static CONVAR( g_height, 720 );