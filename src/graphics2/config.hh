#pragma once

#include "core/console.h"

#include <string.h>
#include <vulkan/vulkan.h>

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

static CONVAR( g_width,  1280 );
static CONVAR( g_height, 720 );