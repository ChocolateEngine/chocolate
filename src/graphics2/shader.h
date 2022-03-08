#pragma once

#include "gutil.hh"

class Shader
{
    std::string      aName;

    VkPipeline       aPipeline;
    VkPipelineLayout aPipelineLayout;
public:
     Shader( const std::string &srName, const std::string &srShaderPath );
    ~Shader();

    inline    std::string      GetName()           { return aName;           }

    constexpr VkPipeline       GetPipeline()       { return aPipeline;       }
    constexpr VkPipelineLayout GetPipelineLayout() { return aPipelineLayout; }
};