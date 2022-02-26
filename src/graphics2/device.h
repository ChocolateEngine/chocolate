#pragma once

#include "types.h"

class Device
{
protected:
    VkSampleCountFlagBits aSampleCount;
    VkPhysicalDevice      aPhysicalDevice;
    VkDevice              aDevice;
    VkQueue               aGraphicsQueue;
    VkQueue               aPresentQueue;

    VkSampleCountFlagBits GetMaxUsableSampleCount();
    void                  SetupPhysicalDevice();
    void                  CreateDevice();
public:
     Device();
    ~Device();

    constexpr VkQueue          GetGraphicsQueue()   { return aGraphicsQueue; }
    constexpr VkQueue          GetPresentQueue()    { return aPresentQueue;  }

    constexpr VkPhysicalDevice GetPhysicalDevice() { return aPhysicalDevice; }
    constexpr VkDevice         GetDevice()         { return aDevice;         }
};

extern Device gDevice;