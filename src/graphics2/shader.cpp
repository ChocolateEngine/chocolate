#include "shader.h"

#include "libspirv/include/spirv-tools/libspirv.hpp"

#include "instance.h"
#include "swapchain.h"
#include "renderpass.h"
#include "descriptormanager.h"

#include "vertex.hh"

#include "core/core.h"

struct ShaderVar
{
    int         aIndex  = -1;
    int         aOffset = -1;
    std::string aName;
};

enum
{
    SHADER_IN,
    SHADER_OUT,
};

struct LayoutObject
{
    int         aLocation  = -1;
    int         aSet       = -1;
    int         aBinding   = -1;
    int         aInFlags   = -1;
    std::string aType;
    std::string aName;
    bool        aIsUBO     = false;
    std::vector< ShaderVar > aVars;
    LayoutObject *aOther   = nullptr;
};

struct ShaderRequirements
{
    std::vector< LayoutObject > aVertObjects;
    std::vector< LayoutObject > aFragObjects;
};

const std::vector< std::string > &GetOpCodeVars( const std::string &srLine ) 
{
    static std::vector< std::string > aVars;
    aVars.clear();
    std::stringstream ss( srLine );
    std::string s;
    while ( std::getline( ss, s, ' ' ) )
    {
        if ( s.length() > 0 )
            aVars.push_back( s );
    }
    return aVars;
}

const std::string &RemovePercent( const std::string &srString )
{
    static std::string aString;
    aString = srString;
    aString.erase( std::remove( aString.begin(), aString.end(), '%' ), aString.end() );
    return aString;
}

const std::string &RemoveQuotes( const std::string &srString )
{
    static std::string aString;
    aString = srString;
    aString.erase( std::remove( aString.begin(), aString.end(), '"' ), aString.end() );
    return aString;
}

LayoutObject *GetLayoutObject( std::vector< LayoutObject > &srObjects, const std::string &srName )
{
    for ( auto &object : srObjects )
    {
        if ( object.aName == srName )
            return &object;
    }
    LogWarn( gGraphics2Channel, "Couldn't find layout object %s\n", srName.c_str() );
    return nullptr;
}

ShaderVar *GetVar( std::vector< ShaderVar > &srObjects, const int sIndex )
{
    for ( auto &object : srObjects )
    {
        if ( object.aIndex == sIndex )
            return &object;
    }
    LogWarn( gGraphics2Channel, "Couldn't find var %d\n", sIndex );
    return nullptr;
}

bool IsVar( const std::string &srLine, const std::vector< LayoutObject > &srObjects )
{
    for ( const auto &object : srObjects )
    {
        if ( object.aName == srLine )
            return true;
    }
    return false;
}

std::vector< LayoutObject > GetRequirements( const std::string &srDissassembly )
{
    std::vector< LayoutObject > obj;
    std::vector< std::string > aLines;
    std::stringstream ss( srDissassembly );
    std::string str;
    while ( std::getline( ss, str ) )
    {
        aLines.push_back( str );
    }

    for ( auto &line : aLines )
    {
        auto aVars = GetOpCodeVars( line );
        if ( aVars[ 0 ] == "OpName" ) {
            LayoutObject lo{};
            lo.aName = RemovePercent( aVars[ 1 ] );
            obj.push_back( lo );
        }
        else if ( aVars[ 0 ] == "OpDecorate" ) {
            auto lo = GetLayoutObject( obj, RemovePercent( aVars[ 1 ] ) );

            for ( int i = 0; i < aVars.size(); ++i ) {
                if ( aVars[ i ] == "Location" ) {
                    lo->aLocation = std::stoi( aVars[ i + 1 ] );
                }
                else if ( aVars[ i ] == "DescriptorSet" ) {
                    lo->aSet = std::stoi( aVars[ i + 1 ] );
                }
                else if ( aVars[ i ] == "Binding" ) {
                    lo->aBinding = std::stoi( aVars[ i + 1 ] );
                }
                else if ( aVars[ i ] == "In" ) {
                    lo->aInFlags = SHADER_IN;
                }
                else if ( aVars[ i ] == "Block" ) {
                    lo->aIsUBO = true;
                }
            }
        }
        else if ( aVars[ 0 ] == "OpMemberName" ) {
            auto lo = GetLayoutObject( obj, RemovePercent( aVars[ 1 ] ) );

            if ( !lo )
                continue;

            ShaderVar sv{};
            sv.aName  = RemoveQuotes( aVars[ 3 ] );
            sv.aIndex = std::stoi( aVars[ 2 ] );
            lo->aVars.push_back( sv );
        }
        else if ( aVars[ 0 ] == "OpMemberDecorate" ) {
            auto lo = GetLayoutObject( obj, RemovePercent( aVars[ 1 ] ) );

            if ( !lo )
                continue;

            auto sv = GetVar( lo->aVars, std::stoi( aVars[ 2 ] ) );

            for ( int i = 0; i < aVars.size(); ++i ) {
                if ( sv && aVars[ i ] == "Offset" ) {
                    sv->aOffset = std::stoi( aVars[ i + 1 ] );
                }
            }
        }
        else if ( IsVar( RemovePercent( aVars[ 0 ] ), obj ) ) {
            /*
             *    Get the types of the variables
             */
            auto lo = GetLayoutObject( obj, RemovePercent( aVars[ 0 ] ) );

            if ( !lo )
                continue;

            for ( int i = 0; i < aVars.size(); ++i ) {
                if ( aVars[ i ] == "OpVariable" ) {
                    /*
                     *    The value that follows is the string %_ptr_<storage>_<name>.
                     *    We need to remove the %_ptr_ and deduce the storage type
                     *    and tie this variable to the layout object with the same name.
                     */
                    auto ptr = RemovePercent( aVars[ i + 1 ] );

                    ptr = ptr.substr( ptr.find( "_" ) + 4 );

                    auto store = ptr.substr( 0, ptr.find( "_" ) );
                    ptr = ptr.substr( ptr.find( "_" ) + 1 );

                    auto name = ptr;

                    if ( IsVar( name, obj ) ) {
                        auto lo2 = GetLayoutObject( obj, name );

                        if ( lo2 ) {
                            lo->aOther = lo2;
                        }
                    }

                    if ( aVars[ i + 2 ] == "Input" ) {
                        lo->aInFlags = SHADER_IN;
                    }
                    else if ( aVars[ i + 2 ] == "Output" ) {
                        lo->aInFlags = SHADER_OUT;
                    }
                    else if ( aVars[ i + 2 ] == "Uniform" ) {
                        lo->aIsUBO = true;
                    }
                }
            }
        }
    }

    return obj;
}

VkShaderModule CreateModule( const std::vector< char > &srData ) 
{
    VkShaderModuleCreateInfo shaderModuleCreateInfo = {};
    shaderModuleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    shaderModuleCreateInfo.pNext = nullptr;
    shaderModuleCreateInfo.flags = 0;
    shaderModuleCreateInfo.codeSize = srData.size();
    shaderModuleCreateInfo.pCode = reinterpret_cast< const uint32_t * >( srData.data() );

    VkShaderModule shaderModule;
    VkResult result = vkCreateShaderModule( GetLogicDevice(), &shaderModuleCreateInfo, nullptr, &shaderModule );
    if ( result != VK_SUCCESS )
    {
        LogError( gGraphics2Channel, "Failed to create shader module: %d\n", result );
        return VK_NULL_HANDLE;
    }

    return shaderModule;
}

enum 
{
    REQUIRES_IMAGES,
    REQUIRES_BUFFERS,
};

VkPipelineLayout CreateLayout( const ShaderRequirements &srReqs )
{
    int flags     = 0;
    int imageSet  = -1;
    int bufferSet = -1;

    for ( auto &req : srReqs.aVertObjects ) {
        if ( req.aType == "_ptr_UniformConstant__runtimearr_14" && req.aSet > -1 ) {
            flags    |= REQUIRES_IMAGES;
            imageSet  = req.aSet;
            LogDev( gGraphics2Channel, 1, "Shader uses image samplers\n" );
        }
        else if ( ( req.aIsUBO || ( req.aOther && req.aOther->aIsUBO ) ) && req.aSet > -1 ) {
            flags     |= REQUIRES_BUFFERS;
            bufferSet  = req.aSet;
            LogDev( gGraphics2Channel, 1, "Shader uses buffers\n" );
        }
    }

    for ( auto &req : srReqs.aFragObjects ) {
        if ( req.aType == "_ptr_UniformConstant__runtimearr_14" && req.aSet > -1 ) {
            flags    |= REQUIRES_IMAGES;
            imageSet  = req.aSet;
            LogDev( gGraphics2Channel, 1, "Shader uses image samplers\n" );
        }
        else if ( ( req.aIsUBO || ( req.aOther && req.aOther->aIsUBO ) ) && req.aSet > -1 ) {
            flags     |= REQUIRES_BUFFERS;
            bufferSet  = req.aSet;
            LogDev( gGraphics2Channel, 1, "Shader uses buffers\n" );
        }
    }
    
    VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = {};
    pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutCreateInfo.pNext = nullptr;
    pipelineLayoutCreateInfo.flags = 0;
    pipelineLayoutCreateInfo.setLayoutCount = 0;
    pipelineLayoutCreateInfo.pSetLayouts = nullptr;
    pipelineLayoutCreateInfo.pushConstantRangeCount = 0;
    pipelineLayoutCreateInfo.pPushConstantRanges = nullptr;

    VkPipelineLayout pipelineLayout;
    VkResult result = vkCreatePipelineLayout( GetLogicDevice(), &pipelineLayoutCreateInfo, nullptr, &pipelineLayout );
    if ( result != VK_SUCCESS )
    {
        LogError( gGraphics2Channel, "Failed to create pipeline layout: %d\n", result );
        return VK_NULL_HANDLE;
    }

    return pipelineLayout;
}

VkPipeline CreatePipeline( const std::string &srVertPath, const std::string &srFragPath )
{
    std::vector< char > vertSpriv = filesys->ReadFile( srVertPath );
    if ( vertSpriv.empty() )
    {
        LogError( gGraphics2Channel, "failed to read shader file: %s\n", srVertPath.c_str() );
        return VK_NULL_HANDLE;
    }

    std::vector< uint32_t > aSpirvBuf;
    aSpirvBuf.resize( vertSpriv.size() / sizeof( uint32_t ) );
    memcpy( aSpirvBuf.data(), vertSpriv.data(), vertSpriv.size() );

    std::string text = "";

    spvtools::Context context( SPV_ENV_VULKAN_1_2 );

    spvtools::SpirvTools tools( SPV_ENV_VULKAN_1_2 );
    
    tools.Disassemble( aSpirvBuf, &text );

    auto vert = GetRequirements( text );

    LogDev( gGraphics2Channel, 1, "shader disassembly:\n%s\n", text.c_str() );

    std::vector< char > fragSpriv = filesys->ReadFile( srFragPath );
    if ( fragSpriv.empty() )
    {
        LogError( gGraphics2Channel, "failed to read shader file: %s\n", srFragPath.c_str() );
        return VK_NULL_HANDLE;
    }

    aSpirvBuf.resize( fragSpriv.size() / sizeof( uint32_t ) );
    memcpy( aSpirvBuf.data(), fragSpriv.data(), fragSpriv.size() );

    text = "";

    tools.Disassemble( aSpirvBuf, &text );

    LogDev( gGraphics2Channel, 1, "shader disassembly:\n%s\n", text.c_str() );

    auto frag = GetRequirements( text );

    ShaderRequirements reqs = { vert, frag };

    VkPipeline pipeline = VK_NULL_HANDLE;
    VkPipelineLayout pipelineLayout = CreateLayout( reqs );

    VkPipelineShaderStageCreateInfo vertShaderStageInfo = {};
    vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertShaderStageInfo.module = CreateModule( vertSpriv );
    vertShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo fragShaderStageInfo = {};
    fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragShaderStageInfo.module = CreateModule( fragSpriv );
    fragShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

    VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

    auto bindingDescription    = Vertex::GetBindingDesc();
    auto attributeDescriptions = Vertex::GetAttributeDesc();

    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.vertexAttributeDescriptionCount = static_cast< uint32_t >( attributeDescriptions.size() );
    vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
    vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

    VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    VkViewport viewport = {};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = ( float )GetSwapchain().GetExtent().width;
    viewport.height = ( float )GetSwapchain().GetExtent().height;
    viewport.minDepth = 0.0f;

    VkRect2D scissor = {};
    scissor.offset = { 0, 0 };
    scissor.extent = GetSwapchain().GetExtent();

    VkPipelineViewportStateCreateInfo viewportState = {};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.pViewports = &viewport;
    viewportState.scissorCount = 1;
    viewportState.pScissors = &scissor;
    
    VkPipelineRasterizationStateCreateInfo rasterizer = {};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
    rasterizer.depthBiasEnable = VK_FALSE;

    VkPipelineMultisampleStateCreateInfo multisampling = {};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_FALSE;

    VkPipelineColorBlendStateCreateInfo colorBlending = {};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.logicOp = VK_LOGIC_OP_COPY;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;
    colorBlending.blendConstants[ 0 ] = 0.0f;
    colorBlending.blendConstants[ 1 ] = 0.0f;
    colorBlending.blendConstants[ 2 ] = 0.0f;
    colorBlending.blendConstants[ 3 ] = 0.0f;

    VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 0;
    pipelineLayoutInfo.pushConstantRangeCount = 0;

    VkPipelineDepthStencilStateCreateInfo depthStencil = {};
    depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencil.depthTestEnable = VK_TRUE;
    depthStencil.depthWriteEnable = VK_TRUE;
    depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
    depthStencil.depthBoundsTestEnable = VK_FALSE;
    depthStencil.minDepthBounds = 0.0f;
    depthStencil.maxDepthBounds = 1.0f;
    depthStencil.stencilTestEnable = VK_FALSE;
    depthStencil.front = {};
    depthStencil.back = {};

    VkGraphicsPipelineCreateInfo pipelineInfo = {};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages;
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDepthStencilState = &depthStencil;
    pipelineInfo.layout = pipelineLayout;
    pipelineInfo.renderPass = GetRenderPass( RenderPass_Color | RenderPass_Depth | RenderPass_Resolve );
    pipelineInfo.subpass = 0;
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

    CheckVKResult( vkCreateGraphicsPipelines( GetLogicDevice(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline ), "Failed to create graphics pipeline!" );

    return pipeline;
}

Shader::Shader( const std::string &srName, const std::string &srVertPath, const std::string &srFragPath )
{
    aPipeline = CreatePipeline( srVertPath, srFragPath );
}

Shader::~Shader()
{

}