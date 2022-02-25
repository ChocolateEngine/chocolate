#pragma once

#include "types.h"
#include "graphics.h"

#ifdef NDEBUG
    constexpr bool 	gEnableValidationLayers = false;
#else
    constexpr bool 	gEnableValidationLayers = true;
#endif

struct Window 
{
    SDL_Window *apWindow;
};

class GInstance
{
protected:
    VkInstance aInstance;
    Window     aWindow;

    std::vector< const char* > InitRequiredExtensions();
    void                       CreateSurface();
public:
     GInstance();
    ~GInstance();
};