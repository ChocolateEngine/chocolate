#include "shader.h"

#include "libspirv/include/spirv-tools/libspirv.hpp"

#include "instance.h"
#include "swapchain.h"
#include "renderpass.h"
#include "descriptormanager.h"

#include "vertex.hh"

#include "core/core.h"

enum
{
    SHADER_IN,
    SHADER_OUT,
};

enum
{
    OP_NAME,
    OP_MEMBER_NAME,
    OP_DECORATE,
    OP_MEMBER_DECORATE,
    OP_ENTRY_POINT,
    OP_VARIABLE,
    OP_TYPE_POINTER,
    OP_TYPE_STRUCT,
    OP_TYPE_RUNTIME_ARRAY,
    OP_TYPE_SAMPLED_IMAGE,
    OP_TYPE_IMAGE,
};

enum
{
    DECO_LOCATION,
    DECO_SET,
    DECO_BINDING,
    DECO_OFFSET,
    DECO_BLOCK,
};

enum
{
    VIS_IN       = 1 << 0,
    VIS_OUT      = 1 << 1,
    VIS_UNIFORM  = 1 << 2,
    VIS_ARR      = 1 << 3,
    VIS_IMAGE    = 1 << 4,
};

struct ShaderVar
{
    int aId                 = -1;
    int aMembIndex          = -1;
    int aLocation           = -1;
    int aSet                = -1;
    int aBinding            = -1;
    int aOffset             = -1;
    int aFlags              =  0;
    std::string aName       = "";
    std::string aIdentifier = "";
    std::string aType       = "";

    ShaderVar *apNext     = nullptr;
    ShaderVar *apChildren = nullptr;
    ShaderVar *apPointsTo = nullptr;
};

struct SpirvDisassembly
{
    ShaderVar  *apLayoutVars;
    ShaderVar  *apVars;

    ShaderVar   aEntry      = {};
    std::string aShaderType = "";
};

struct ShaderRequirements
{
    SpirvDisassembly aDisassembly;
};

void SkipWS( const std::string &srSource, size_t &sIndex )
{
    while( sIndex < srSource.size() && srSource[ sIndex ] <= ' ' )
        ++sIndex;
}

void SkipLine( const std::string &srSource, size_t &sIndex )
{
    while( sIndex < srSource.size() && srSource[ sIndex ] != '\n' )
        ++sIndex;
    
    ++sIndex;
}

int SwitchOperation( const std::string &srSource, size_t &sIndex )
{
    SkipWS( srSource, sIndex );
    if ( srSource.substr( sIndex, 6 ) == "OpName" )
    {
        sIndex += 6;
        return OP_NAME;
    }
    else if ( srSource.substr( sIndex, 12 ) == "OpMemberName" )
    {
        sIndex += 12;
        return OP_MEMBER_NAME;
    }
    else if ( srSource.substr( sIndex, 10 ) == "OpDecorate" )
    {
        sIndex += 10;
        return OP_DECORATE;
    }
    else if ( srSource.substr( sIndex, 16 ) == "OpMemberDecorate" )
    {
        sIndex += 16;
        return OP_MEMBER_DECORATE;
    }
    else if ( srSource.substr( sIndex, 12 ) == "OpEntryPoint" )
    {
        sIndex += 12;
        return OP_ENTRY_POINT;
    }
    else if ( srSource.substr( sIndex, 10 ) == "OpVariable" ) 
    {
        sIndex += 10;
        return OP_VARIABLE;
    }
    else if ( srSource.substr( sIndex, 13 ) == "OpTypePointer" )
    {
        sIndex += 13;
        return OP_TYPE_POINTER;
    }
    else if ( srSource.substr( sIndex, 12 ) == "OpTypeStruct" )
    {
        sIndex += 12;
        return OP_TYPE_STRUCT;
    }
    else if ( srSource.substr( sIndex, 18 ) == "OpTypeRuntimeArray" )
    {
        sIndex += 18;
        return OP_TYPE_RUNTIME_ARRAY;
    }
    else if ( srSource.substr( sIndex, 18 ) == "OpTypeSampledImage" )
    {
        sIndex += 18;
        return OP_TYPE_SAMPLED_IMAGE;
    }
    else if ( srSource.substr( sIndex,  11 ) == "OpTypeImage" )
    {
        sIndex += 11;
        return OP_TYPE_IMAGE;
    }
    else
    {
        return -1;
    }
}

int SwitchDecorate( const std::string &srSource, size_t &sIndex )
{
    SkipWS( srSource, sIndex );
    if ( srSource.substr( sIndex, 8 ) == "Location" )
    {
        sIndex += 8;
        return DECO_LOCATION;
    }
    else if ( srSource.substr( sIndex, 13 ) == "DescriptorSet" )
    {
        sIndex += 13;
        return DECO_SET;
    }
    else if ( srSource.substr( sIndex, 7 ) == "Binding" )
    {
        sIndex += 7;
        return DECO_BINDING;
    }
    else if ( srSource.substr( sIndex, 6 ) == "Offset" )
    {
        sIndex += 6;
        return DECO_OFFSET;
    }
    else if ( srSource.substr( sIndex, 5 ) == "Block" )
    {
        sIndex += 5;
        return DECO_BLOCK;
    }
    else
    {
        return -1;
    }
}

std::string OpGetStr( const std::string &srSource, size_t &srIndex )
{
    SkipWS( srSource, srIndex );
    std::string var;
    while( srIndex < srSource.size() && srSource[ srIndex ] > ' ' )
        var += srSource[ srIndex++ ];

    return var;
}

std::string OpSeekStr( const std::string &srSource, size_t sIndex )
{
    SkipWS( srSource, sIndex );
    std::string var;
    while( sIndex < srSource.size() && srSource[ sIndex ] > ' ' )
        var += srSource[ sIndex++ ];

    return var;
}

int OpGetInt( const std::string &srSource, size_t &srIndex )
{
    SkipWS( srSource, srIndex );
    std::string var;
    while( srIndex < srSource.size() && srSource[ srIndex ] > ' ' )
        var += srSource[ srIndex++ ];

    return atoi( var.c_str() );
}

struct DecomposedVar
{
    std::string aVarType;
    std::string aVisibility;
    std::string aIdentifier;
};

DecomposedVar DecomposeVar( const std::string &srSource, size_t &srIndex )
{
    SkipWS( srSource, srIndex );
    DecomposedVar var;
    size_t len = srIndex;

    ++srIndex;

    if ( srSource[ srIndex ] == '_' ) {
        var.aVarType += srSource[ srIndex++ ];
    }
    
    while ( srIndex < srSource.size() && ( srSource[ srIndex ] != '_' || srIndex <= len ) ) {
        var.aVarType += srSource[ srIndex++ ];
    }
    srIndex++;
    while ( srIndex < srSource.size() && srSource[ srIndex ] != '_' ) {
        var.aVisibility += srSource[ srIndex++ ];
    }
    srIndex++;
    while ( srIndex < srSource.size() && srSource[ srIndex ] > ' ' ) {
        var.aIdentifier += srSource[ srIndex++ ];
    }

    return var;
}

ShaderVar *GetVar( SpirvDisassembly &srDisass, const std::string &srVar )
{
    if ( srVar == srDisass.aEntry.aName )
        return &srDisass.aEntry;

    for ( auto var = srDisass.apLayoutVars; var; var = var->apNext )
        if ( var->aName == srVar )
            return var;
        
    for ( auto var = srDisass.apVars; var; var = var->apNext )
        if ( var->aName == srVar )
            return var;

    return nullptr;
}

ShaderVar *GetVar( SpirvDisassembly &srDisass, const int &srId )
{
    if ( srId == srDisass.aEntry.aId )
        return &srDisass.aEntry;

    for ( auto var = srDisass.apLayoutVars; var; var = var->apNext )
        if ( var->aId == srId )
            return var;
        
    for ( auto var = srDisass.apVars; var; var = var->apNext )
        if ( var->aId == srId )
            return var;

    return nullptr;
}

ShaderVar *GetMembVar( ShaderVar *spParent, const std::string &srVar )
{
    for ( auto var = spParent->apChildren; var; var = var->apNext )
        if ( var->aName == srVar )
            return var;

    return nullptr;
}

ShaderVar *GetMembVar( ShaderVar *spParent, const int &srId )
{
    for ( auto var = spParent->apChildren; var; var = var->apNext )
        if ( var->aId == srId )
            return var;

    return nullptr;
}

ShaderVar *AddVar( SpirvDisassembly &srDisass, const std::string& srVar )
{
    ShaderVar *pVar = GetVar( srDisass, srVar );
    if ( pVar )
        return pVar;

    ShaderVar *pNewVar = new ShaderVar;
    
    if ( !pNewVar )
        return nullptr;

    if ( srDisass.apVars == nullptr ) {
        srDisass.apVars = pNewVar;
        pNewVar->apNext = nullptr;

        return pNewVar;
    }

    ShaderVar *pMemb = srDisass.apVars;
    while ( pMemb && pMemb->apNext != nullptr )
        pMemb = pMemb->apNext;

    pMemb->apNext   = pNewVar;
    pNewVar->aName  = srVar;

    return pNewVar;
}

ShaderVar *AddVar( SpirvDisassembly &srDisass, const int &srId )
{
    ShaderVar *pVar = GetVar( srDisass, srId );
    if ( pVar )
        return pVar;

    ShaderVar *pNewVar = new ShaderVar;

    if ( !pNewVar )
        return nullptr;

    if ( srDisass.apVars == nullptr ) {
        srDisass.apVars = pNewVar;
        pNewVar->apNext = nullptr;

        return pNewVar;
    }

    ShaderVar *pMemb = srDisass.apVars;
    while ( pMemb && pMemb->apNext != nullptr )
        pMemb = pMemb->apNext;

    pMemb->apNext   = pNewVar;
    pNewVar->aId    = srId;

    return pNewVar;
}

ShaderVar *AddMembVar( ShaderVar *spParent, const std::string &srVar )
{
    ShaderVar *pVar = GetMembVar( spParent, srVar );
    if ( pVar )
        return pVar;

    ShaderVar *pNewVar = new ShaderVar;

    if ( !pNewVar )
        return nullptr;

    if ( spParent->apChildren == nullptr ) {
        spParent->apChildren = pNewVar;
        pNewVar->apNext = nullptr;

        return pNewVar;
    }

    ShaderVar *pMemb = spParent->apChildren;
    while ( pMemb && pMemb->apNext != nullptr )
        pMemb = pMemb->apNext;

    pMemb->apNext   = pNewVar;
    pNewVar->aName  = srVar;

    return pNewVar;
}

ShaderVar *AddMembVar( ShaderVar *spParent, const int &srId )
{
    ShaderVar *pVar = GetMembVar( spParent, srId );
    if ( pVar )
        return pVar;

    ShaderVar *pNewVar = new ShaderVar;

    if ( !pNewVar )
        return nullptr;

    if ( spParent->apChildren == nullptr ) {
        spParent->apChildren = pNewVar;
        pNewVar->apNext = nullptr;

        return pNewVar;
    }

    ShaderVar *pMemb = spParent->apChildren;
    while ( pMemb && pMemb->apNext != nullptr )
        pMemb = pMemb->apNext;

    pMemb->apNext   = pNewVar;
    pNewVar->aId    = srId;

    return pNewVar;
}

bool UnderlyingHasFlag( ShaderVar *spParent, const int sFlag )
{
    if ( spParent->aFlags & sFlag )
        return true;

    for ( auto var = spParent->apPointsTo; var; var = var->apPointsTo )
        if ( var->aFlags & sFlag )
            return true;

    return false;
}

/*
 *    Parses the requirements from a disassembled shader.
 *    
 *    TODO: Make use of C-style strings, substr is slow.
 *
 *    @param  const std::string &    The shader disassembly.
 *
 *    @return ShaderRequirements     The requirements of the shader.
 */
ShaderRequirements GetRequirements( const std::string &srDisassembly )
{
    ShaderRequirements reqs{};
    size_t c = 0;
    
    SkipWS( srDisassembly, c );
    while ( c < srDisassembly.size() )
    {
        /*
         *    If our line starts with Op, then we're looking at a new instruction.
         */
        if ( srDisassembly.substr( c, 2 ) == "Op" )
        {
            int op = SwitchOperation( srDisassembly, c );
            switch ( op ) {
                case OP_NAME: {
                    std::string var  = OpGetStr( srDisassembly, c );

                    if ( var[ 0 ] != '%' ) {
                        LogError( "Invalid variable identifier: %s\n", var.c_str() );
                        SkipLine( srDisassembly, c );
                        break;
                    } 

                    std::string name = OpGetStr( srDisassembly, c );

                    ShaderVar *pVar = AddVar( reqs.aDisassembly, var );
                    if ( !pVar ) {
                        LogError( "Could not add variable: %s\n", var.c_str() );
                        SkipLine( srDisassembly, c );
                        break;
                    }

                    pVar->aIdentifier = name;
                    break;
                }
                case OP_MEMBER_NAME: {
                    std::string var  = OpGetStr( srDisassembly, c );

                    if ( var[ 0 ] != '%' ) {
                        LogError( "Invalid variable identifier: %s\n", var.c_str() );
                        SkipLine( srDisassembly, c );
                        break;
                    }
                    
                    ShaderVar *pParent = AddVar( reqs.aDisassembly, var );
                    if ( !pParent ) {
                        LogError( "Failed to find parent variable '%s' for member name.\n", var.c_str() );
                        SkipLine( srDisassembly, c );
                        break;
                    }

                    int         memb = OpGetInt( srDisassembly, c );
                    std::string name = OpGetStr( srDisassembly, c );

                    ShaderVar *pVar = AddMembVar( pParent, memb );
                    if ( !pVar ) {
                        LogError( "Failed to allocate memory for member variable '%s' of '%s'.\n", name.c_str(), var.c_str() );
                        SkipLine( srDisassembly, c );
                        break;
                    }

                    pVar->aIdentifier      = name;
                    pVar->aMembIndex       = memb;
                    break;
                }
                case OP_DECORATE: {
                    std::string var  = OpGetStr( srDisassembly, c );

                    if ( var[ 0 ] != '%' ) {
                        LogError( "Invalid variable identifier: %s\n", var.c_str() );
                        SkipLine( srDisassembly, c );
                        break;
                    }

                    ShaderVar *pVar = GetVar( reqs.aDisassembly, var );
                    if ( !pVar ) {
                        LogError( "Failed to find variable '%s' for decoration.\n", var.c_str() );
                        SkipLine( srDisassembly, c );
                        break;
                    }

                    int decor = SwitchDecorate( srDisassembly, c );

                    switch ( decor ) {
                        case DECO_LOCATION: {
                            int loc = OpGetInt( srDisassembly, c );
                            pVar->aLocation = loc;
                            break;
                        }
                        case DECO_BINDING: {
                            int bind = OpGetInt( srDisassembly, c );
                            pVar->aBinding = bind;
                            break;
                        }
                        case DECO_SET: {
                            int set = OpGetInt( srDisassembly, c );
                            pVar->aSet = set;
                            break;
                        }
                        case DECO_OFFSET: {
                            int offset = OpGetInt( srDisassembly, c );
                            pVar->aOffset = offset;
                            break;
                        }
                        case DECO_BLOCK: {
                            pVar->aFlags |= VIS_UNIFORM;
                            break;
                        }
                        default: {
                            LogWarn( "Unknown decoration: %d\n", decor );
                            SkipLine( srDisassembly, c );
                            break;
                        }
                    }
                    break;
                }
                case OP_MEMBER_DECORATE: {
                    std::string var  = OpGetStr( srDisassembly, c );

                    if ( var[ 0 ] != '%' ) {
                        LogError( "Invalid variable identifier: %s\n", var.c_str() );
                        SkipLine( srDisassembly, c );
                        break;
                    }

                    ShaderVar *pParent = GetVar( reqs.aDisassembly, var );
                    if ( !pParent ) {
                        LogError( "Failed to find parent variable '%s' for member decoration.\n", var.c_str() );
                        SkipLine( srDisassembly, c );
                        break;
                    }

                    int memb  = OpGetInt( srDisassembly, c );
                    int decor = SwitchDecorate( srDisassembly, c );

                    ShaderVar *pVar = AddMembVar( pParent, memb );

                    if ( !pVar ) {
                        LogError( "Failed to find member variable in parent '%s' for decoration.\n", var.c_str() );
                        SkipLine( srDisassembly, c );
                        break;
                    }

                    switch ( decor ) {
                        case DECO_LOCATION: {
                            int loc = OpGetInt( srDisassembly, c );
                            pVar->aLocation = loc;
                            break;
                        }
                        case DECO_BINDING: {
                            int bind = OpGetInt( srDisassembly, c );
                            pVar->aBinding = bind;
                            break;
                        }
                        case DECO_SET: {
                            int set = OpGetInt( srDisassembly, c );
                            pVar->aSet = set;
                            break;
                        }
                        case DECO_OFFSET: {
                            int offset = OpGetInt( srDisassembly, c );
                            pVar->aOffset = offset;
                            break;
                        }
                        case DECO_BLOCK: {
                            pVar->aFlags |= VIS_UNIFORM;
                            break;
                        }
                        default: {
                            LogWarn( "Unknown decoration: %d\n", decor );
                            SkipLine( srDisassembly, c );
                            break;
                        }
                    }
                    break;
                }
                case OP_ENTRY_POINT: {
                    std::string type = OpGetStr( srDisassembly, c );
                    reqs.aDisassembly.aShaderType = type;

                    std::string entry = OpGetStr( srDisassembly, c );
                    reqs.aDisassembly.aEntry.aName = entry;
                    
                    std::string identifier = OpGetStr( srDisassembly, c );
                    reqs.aDisassembly.aEntry.aIdentifier = identifier;

                    std::string layoutVar = OpSeekStr( srDisassembly, c );
                    while ( layoutVar[ 0 ] == '%' ) {
                        layoutVar = OpGetStr( srDisassembly, c );
                        ShaderVar *pVar = AddVar( reqs.aDisassembly, layoutVar );
                        if ( !pVar ) {
                            LogError( "Failed to add variable '%s' to layout.\n", layoutVar.c_str() );
                            SkipLine( srDisassembly, c );
                            break;
                        }
                        layoutVar = OpSeekStr( srDisassembly, c );
                    }
                    break;
                }
                default: {
                    LogWarn( "Unknown operation\n" );
                    SkipLine( srDisassembly, c );
                    break;
                }
            }
        }
        else
        {
            /*
             *    Otherwise, we're looking at a variable declaration.
             */
            if ( srDisassembly[ c ] == '%' )
            {
                std::string var = OpGetStr( srDisassembly, c );
                ShaderVar *pVar = AddVar( reqs.aDisassembly, var );

                if ( !pVar ) {
                    LogError( "Failed to find variable '%s' for declaration.\n", var.c_str() );
                    SkipLine( srDisassembly, c );
                    break;
                }

                SkipWS( srDisassembly, c );
                if ( srDisassembly[ c ] != '=' ) {
                    LogError( "Expected '=' after variable declaration.\n" );
                    SkipLine( srDisassembly, c );
                    break;
                }
                ++c;
                int op = SwitchOperation( srDisassembly, c );
                switch ( op ) {
                    case OP_VARIABLE: {
                        auto [ type, vis, name ] = DecomposeVar( srDisassembly, c );
                        if ( type == "_ptr" ) {
                            auto other = GetVar( reqs.aDisassembly, "%" + name );

                            if ( !other ) {
                                LogError( "Failed to find variable '%s' for pointer declaration.\n", name.c_str() );
                                SkipLine( srDisassembly, c );
                                break;
                            }

                            pVar->apPointsTo = other;

                            if ( vis == "Input" )
                                pVar->aFlags |= VIS_IN;
                            else if ( vis == "Output" )
                                pVar->aFlags |= VIS_OUT;
                            /*
                             *    Fair warning, this is a hack.
                             *    
                             *    We're assuming this follows the pattern:
                             *    %var = _ptr %vis %other
                             *    However, the disassembly may not be in this format,
                             *    and instead refer directly to the other variable.
                             */
                            /*else if ( vis == "UniformConstant" )
                                pVar->aFlags |= VIS_UNIFORM;*/
                        }
                        SkipLine( srDisassembly, c );
                        break;
                    }
                    case OP_TYPE_RUNTIME_ARRAY: {
                        auto var   = OpGetStr( srDisassembly, c );
                        auto other = AddVar( reqs.aDisassembly, var );
                        if ( !other ) {
                            LogError( "Failed to get variable '%s' for array declaration.\n", var.c_str() );
                            SkipLine( srDisassembly, c );
                            break;
                        }
                        pVar->apPointsTo = other;
                        break;
                    }
                    case OP_TYPE_SAMPLED_IMAGE: {
                        auto var   = OpGetStr( srDisassembly, c );
                        auto other = AddVar( reqs.aDisassembly, var );
                        if ( !other ) {
                            LogError( "Failed to get variable '%s' for image declaration.\n", var.c_str() );
                            SkipLine( srDisassembly, c );
                            break;
                        }
                        pVar->apPointsTo = other;
                        break;
                    }
                    case OP_TYPE_IMAGE: {
                        pVar->aFlags |= VIS_IMAGE;
                        /*
                         *    Parse this later.
                         */
                        SkipLine( srDisassembly, c );
                        break;
                    }
                    default: {
                        LogError( "Unknown operation\n" );
                        SkipLine( srDisassembly, c );
                        break;
                    }
                }
            }
            else
            {
                LogError( "Expected variable identifier\n" );
                SkipLine( srDisassembly, c );
                continue;
            }
        }
        SkipWS( srDisassembly, c );
    }

    return reqs;
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

VkPipelineLayout CreateLayout( const ShaderRequirements &srVert, const ShaderRequirements &srFrag )
{
    int flags     = 0;
    int imageSet  = -1;
    int bufferSet = -1;

    for ( auto var = srVert.aDisassembly.apVars; var; var = var->apNext ) {
        if ( var->aSet > -1 ) {
            if ( UnderlyingHasFlag( var, VIS_IMAGE ) ) {
                if ( imageSet == -1 ) {
                    imageSet = var->aSet;
                    flags   |= REQUIRES_IMAGES;
                    LogDev( 1, "Image set %d\n", imageSet );
                }
            }
            else if ( UnderlyingHasFlag( var, VIS_UNIFORM ) ) {
                if ( bufferSet == -1 ) {
                    bufferSet = var->aSet;
                    flags    |= REQUIRES_BUFFERS;
                    LogDev( 1, "Buffer set %d\n", bufferSet );
                }
            }
        }
    }

    for ( auto var = srFrag.aDisassembly.apVars; var; var = var->apNext ) {
        if ( var->aSet > -1 ) {
            if ( UnderlyingHasFlag( var, VIS_IMAGE ) ) {
                if ( imageSet == -1 ) {
                    imageSet = var->aSet;
                    flags   |= REQUIRES_IMAGES;
                    LogDev( 1, "Image set %d\n", imageSet );
                }
            }
            else if ( UnderlyingHasFlag( var, VIS_UNIFORM ) ) {
                if ( bufferSet == -1 ) {
                    bufferSet = var->aSet;
                    flags    |= REQUIRES_BUFFERS;
                    LogDev( 1, "Buffer set %d\n", bufferSet );
                }
            }
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

    //ShaderRequirements reqs = { vert, frag };

    VkPipeline pipeline = VK_NULL_HANDLE;
    VkPipelineLayout pipelineLayout = CreateLayout( vert, frag );

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