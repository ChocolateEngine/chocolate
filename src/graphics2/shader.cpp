#include "shader.h"

#include "libspirv/include/spirv-tools/libspirv.hpp"

#include "core/core.h"

struct ShaderVar
{
    uint32_t    aIndex;
    uint32_t    aOffset;
    std::string aName;
};

struct LayoutObject
{
    uint32_t    aLocation;
    uint32_t    aSet;
    uint32_t    aBinding;
    bool        aIn;
    std::string aType;
    std::string aName;
    bool        aIsUniform;
    std::vector< ShaderVar > aVars;
};

struct ShaderRequirements
{
    std::vector< LayoutObject > aInputs;
    std::vector< LayoutObject > aOutputs;
    std::vector< LayoutObject > aUBOS;
};

ShaderRequirements GetRequirements( const std::string &srDissassembly )
{
    ShaderRequirements s{};

    std::vector< LayoutObject > aOpNames;
    for ( int i = 0; i < srDissassembly.size(); ++i )
    {
        std::string opName = "";
        while ( i < srDissassembly.size() && srDissassembly[i] != '\n' )
        {
            opName += srDissassembly[ i ];
            ++i;
        }
        if ( opName.find( "OpName" ) == 0 )
        {
            for ( int j = 0; j < opName.size(); ++j )
            {
                if ( opName[ j ] == '\"' )
                {
                    opName = opName.substr( j + 1, opName.size() - j - 2 );
                    break;
                }
            }
            LayoutObject op;
            op.aName = opName;
            aOpNames.push_back( op );
        }
        if ( opName.find( "OpDecorate" ) == 0 )
        {
            for ( auto& op : aOpNames )
            {
                if ( opName.find( op.aName ) == 0 )
                {
                    int pos = 0;
                    if ( ( pos = opName.find( "Location" ) ) != std::string::npos )
                    {
                        op.aLocation = std::stoi( opName.substr( pos + 9 ) );
                    }
                    if ( ( pos = opName.find( "DescriptorSet" ) ) != std::string::npos )
                    {
                        op.aSet = std::stoi( opName.substr( pos + 14 ) );
                    }
                    if ( ( pos = opName.find( "Binding" ) ) != std::string::npos )
                    {
                        op.aBinding = std::stoi( opName.substr( pos + 8 ) );
                    }
                }
            }
        }
        if ( opName.find( "OpMemberName" ) == 0 )
        {
            for ( auto& op : aOpNames )
            {
                if ( opName.find( op.aName ) != std::string::npos ) 
                {
                    ShaderVar var{};
                    for ( int j = 0; j < opName.size(); ++j )
                    {
                        if ( opName[ j ] >= '0' && opName[ j ] <= '9' )
                        {
                            var.aIndex = std::stoi( opName.substr( j, opName.size() - j ) );
                        }
                        if ( opName[ j ] == '\"' )
                        {
                            var.aName = opName.substr( j + 1, opName.size() - j - 2 );
                        }
                    }
                    op.aVars.push_back( var );
                }
            }
        }
    }
    s.aInputs = aOpNames;

    return s;
}

VkPipeline CreatePipeline( const std::string &srShaderPath )
{
    VkPipeline pipeline;

    std::vector< char > aSpirv = filesys->ReadFile( srShaderPath );
    if ( aSpirv.empty() )
    {
        LogError( gGraphics2Channel, "failed to read shader file: %s\n", srShaderPath.c_str() );
        return pipeline;
    }

    std::vector< uint32_t > aSpirvBuf;
    aSpirvBuf.resize( aSpirv.size() / sizeof( uint32_t ) );
    memcpy( aSpirvBuf.data(), aSpirv.data(), aSpirv.size() );

    std::string text = "";

    spvtools::Context context( SPV_ENV_VULKAN_1_2 );

    spvtools::SpirvTools tools( SPV_ENV_VULKAN_1_2 );
    
    tools.Disassemble( aSpirvBuf, &text );

    auto aRequirements = GetRequirements( text );

    sys_wait_for_debugger();

    //LogDev( gGraphics2Channel, 1, "shader disassembly:\n%s\n", text.c_str() );

    return pipeline;
}

Shader::Shader( const std::string &srName, const std::string &srShaderPath )
{
    aName = srName;

    aPipeline = CreatePipeline( srShaderPath );
}

Shader::~Shader()
{

}