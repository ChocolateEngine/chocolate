#include "core/platform.h"
#include "core/log.h"
#include "util.h"

#include "render/irender.h"
#include "render_vk.h"


static VkRenderPass                                         gMainRenderPass = VK_NULL_HANDLE;
static ResourceList< VkRenderPass >                         gRenderPasses;

static std::unordered_map< VkRenderPass, RenderPassInfoVK > gRenderPassInfo;

// From Section 7.1 of Vulkan API Spec v1.1.148
constexpr VkPipelineStageFlags                              gDefaultAccessMask = VK_ACCESS_INPUT_ATTACHMENT_READ_BIT |
                                                    VK_ACCESS_COLOR_ATTACHMENT_READ_BIT |
                                                    VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT |
                                                    VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT |
                                                    VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;


VkFormat VK_GetColorFormat()
{
	return VK_GetSwapFormat();
}


VkFormat VK_GetDepthFormat()
{
	return VK_FORMAT_D32_SFLOAT;
	// return VK_FORMAT_D32_SFLOAT_S8_UINT;
}


void VK_DestroyRenderPass( VkRenderPass& srRenderPass )
{
	vkDestroyRenderPass( VK_GetDevice(), srRenderPass, nullptr );
}


VkRenderPass VK_CreateMainRenderPass()
{
	if ( gMainRenderPass != VK_NULL_HANDLE )
		return gMainRenderPass;

	/*
     *    Create the default color and depth render pass.
     *    This is the standard draw pass, and later, other
     *    passes may do deferred for example.
     */

	VkAttachmentDescription* attachments = new VkAttachmentDescription[ VK_UseMSAA() ? 3 : 2 ];
	if ( attachments == nullptr )
		Log_Fatal( gLC_Render, "Failed to Allocate Main RenderPass Attachments!\n" );

	VkAttachmentReference* attachRefs = new VkAttachmentReference[ VK_UseMSAA() ? 3 : 2 ];
	if ( attachRefs == nullptr )
		Log_Fatal( gLC_Render, "Failed to Allocate Main RenderPass Attachment References!\n" );

	VkSubpassDependency* dependencies = new VkSubpassDependency[ 2 ];
	if ( dependencies == nullptr )
		Log_Fatal( gLC_Render, "Failed to Allocate Main RenderPass Subpass Dependencies!\n" );

	// Color Attachment
	attachments[ 0 ].flags          = 0;
	attachments[ 0 ].format         = VK_GetSwapFormat();
	attachments[ 0 ].samples        = VK_GetMSAASamples();
	attachments[ 0 ].loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
	attachments[ 0 ].storeOp        = VK_ATTACHMENT_STORE_OP_STORE;
	attachments[ 0 ].stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attachments[ 0 ].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachments[ 0 ].initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
	attachments[ 0 ].finalLayout    = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	// Depth Stencil Attachment
	attachments[ 1 ].flags          = 0;
	attachments[ 1 ].format         = VK_GetDepthFormat();
	attachments[ 1 ].samples        = VK_GetMSAASamples();
	attachments[ 1 ].loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
	attachments[ 1 ].storeOp        = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachments[ 1 ].stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attachments[ 1 ].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachments[ 1 ].initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
	attachments[ 1 ].finalLayout    = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	if ( VK_UseMSAA() )
	{
		// Color Resolve Attachment
		attachments[ 2 ].flags          = 0;
		attachments[ 2 ].format         = VK_GetSwapFormat();
		attachments[ 2 ].samples        = VK_SAMPLE_COUNT_1_BIT;
		attachments[ 2 ].loadOp         = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		attachments[ 2 ].storeOp        = VK_ATTACHMENT_STORE_OP_STORE;
		attachments[ 2 ].stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		attachments[ 2 ].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		attachments[ 2 ].initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
		attachments[ 2 ].finalLayout    = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
	}

	attachRefs[ 0 ].attachment      = 0;
	attachRefs[ 0 ].layout          = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	attachRefs[ 1 ].attachment      = 1;
	attachRefs[ 1 ].layout          = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpass    = {};
	subpass.pipelineBindPoint       = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount    = 1;
	subpass.pColorAttachments       = &attachRefs[ 0 ];
	subpass.pDepthStencilAttachment = &attachRefs[ 1 ];

	if ( VK_UseMSAA() )
	{
		attachRefs[ 2 ].attachment  = 2;
		attachRefs[ 2 ].layout      = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		subpass.pResolveAttachments = &attachRefs[ 2 ];
	}

	// subpass.pResolveAttachments                         = &colorAttachmentResolveRef;

	// VkSubpassDependency dependency                      = {};
	// dependency.srcSubpass                               = VK_SUBPASS_EXTERNAL;
	// dependency.dstSubpass                               = 0;
	// dependency.srcStageMask                             = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	// // dependency.srcStageMask                = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	// dependency.srcAccessMask                            = 0;
	// dependency.dstStageMask                             = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	// // dependency.dstStageMask                = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	// dependency.dstAccessMask                            = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	// // dependency.dstAccessMask               = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

	dependencies[ 0 ].srcSubpass      = VK_SUBPASS_EXTERNAL;
	dependencies[ 0 ].dstSubpass      = 0;
	dependencies[ 0 ].srcStageMask    = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	dependencies[ 0 ].dstStageMask    = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependencies[ 0 ].srcAccessMask   = VK_ACCESS_SHADER_READ_BIT;
	dependencies[ 0 ].dstAccessMask   = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	dependencies[ 0 ].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

	dependencies[ 1 ].srcSubpass      = 0;
	dependencies[ 1 ].dstSubpass      = VK_SUBPASS_EXTERNAL;
	dependencies[ 1 ].srcStageMask    = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependencies[ 1 ].dstStageMask    = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	dependencies[ 1 ].srcAccessMask   = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	dependencies[ 1 ].dstAccessMask   = VK_ACCESS_SHADER_READ_BIT;
	dependencies[ 1 ].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

	VkRenderPassCreateInfo renderPassInfo{ VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO };
	renderPassInfo.attachmentCount = VK_UseMSAA() ? 3 : 2;
	renderPassInfo.pAttachments    = attachments;
	renderPassInfo.subpassCount    = 1;
	renderPassInfo.pSubpasses      = &subpass;
	renderPassInfo.dependencyCount = 2;
	renderPassInfo.pDependencies   = dependencies;

	VK_CheckResult( vkCreateRenderPass( VK_GetDevice(), &renderPassInfo, nullptr, &gMainRenderPass ), "Failed to create main render pass!" );

	gRenderPassInfo[ gMainRenderPass ] = { VK_UseMSAA() };

	delete[] attachments;
	delete[] attachRefs;
	delete[] dependencies;

	return gMainRenderPass;
}


Handle VK_CreateRenderPass( const RenderPassCreate_t& srPass )
{
	ChVector< VkAttachmentDescription > attachments( srPass.aAttachments.size() );
	ChVector< VkSubpassDescription >    subpasses( srPass.aSubpasses.size() );

	// um
	ChVector< VkSubpassDependency >     dependencies( 2 );

	ChVector< VkAttachmentReference >   colorRefs;
	VkAttachmentReference               depthRef;

	// set it to this, if it changes, we have a depth attachment reference
	depthRef.layout = VK_IMAGE_LAYOUT_MAX_ENUM;

	bool useMSAA    = false;

	for ( u32 i = 0; i < srPass.aAttachments.size(); i++ )
	{
		const RenderPassAttachment_t& attach = srPass.aAttachments[ i ];
		useMSAA |= attach.aUseMSAA && VK_UseMSAA();

		attachments[ i ].format              = VK_ToVkFormat( attach.aFormat );
		attachments[ i ].samples             = attach.aUseMSAA ? VK_GetMSAASamples() : VK_SAMPLE_COUNT_1_BIT;
		attachments[ i ].loadOp              = VK_ToVkLoadOp( attach.aLoadOp );
		attachments[ i ].storeOp             = VK_ToVkStoreOp( attach.aStoreOp );
		attachments[ i ].stencilLoadOp       = VK_ToVkLoadOp( attach.aStencilLoadOp );
		attachments[ i ].stencilStoreOp      = VK_ToVkStoreOp( attach.aStencilStoreOp );

		attachments[ i ].storeOp             = VK_ATTACHMENT_STORE_OP_STORE;
		attachments[ i ].stencilLoadOp       = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		attachments[ i ].stencilStoreOp      = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		attachments[ i ].initialLayout       = VK_IMAGE_LAYOUT_UNDEFINED;

		if ( attach.aType == EAttachmentType_Depth )
		{
			if ( depthRef.layout != VK_IMAGE_LAYOUT_MAX_ENUM )
			{
				Log_Error( "VK_CreateRenderPass: Already have a depth attachment!\n" );
				return InvalidHandle;
			}

			depthRef.attachment = i;
			depthRef.layout              = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
			attachments[ i ].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		}
		else
		{
			colorRefs.push_back( { i, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL } );
			attachments[ i ].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
		}
	}

	for ( size_t i = 0; i < srPass.aSubpasses.size(); i++ )
	{
		const RenderPassSubpass_t& subpass  = srPass.aSubpasses[ i ];

		subpasses[ i ].pipelineBindPoint    = VK_ToPipelineBindPoint( subpass.aBindPoint );
		subpasses[ i ].colorAttachmentCount = static_cast< u32 >( colorRefs.size() );
		subpasses[ i ].pColorAttachments    = colorRefs.data();

		if ( depthRef.layout != VK_IMAGE_LAYOUT_MAX_ENUM )
			subpasses[ i ].pDepthStencilAttachment    = &depthRef;
	}

	// not a fan of this, feels like this shouldn't be static values
	dependencies[ 0 ].srcSubpass       = VK_SUBPASS_EXTERNAL;
	dependencies[ 0 ].dstSubpass       = 0;
	dependencies[ 0 ].srcStageMask     = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
	dependencies[ 0 ].dstStageMask     = VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT;
	dependencies[ 0 ].srcAccessMask    = 0;
	dependencies[ 0 ].dstAccessMask    = gDefaultAccessMask;
	dependencies[ 0 ].dependencyFlags  = 0;

	dependencies[ 1 ].srcSubpass       = 0;
	dependencies[ 1 ].dstSubpass       = VK_SUBPASS_EXTERNAL;
	dependencies[ 1 ].srcStageMask     = VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT;
	dependencies[ 1 ].dstStageMask     = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
	dependencies[ 1 ].srcAccessMask    = gDefaultAccessMask;
	dependencies[ 1 ].dstAccessMask    = 0;
	dependencies[ 1 ].dependencyFlags  = 0;

	VkRenderPassCreateInfo renderPassInfo{ VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO };
	renderPassInfo.attachmentCount = static_cast< uint32_t >( attachments.size() );
	renderPassInfo.pAttachments    = attachments.data();
	renderPassInfo.subpassCount    = static_cast< uint32_t >( subpasses.size() );
	renderPassInfo.pSubpasses      = subpasses.data();
	renderPassInfo.dependencyCount = static_cast< uint32_t >( dependencies.size() );
	renderPassInfo.pDependencies   = dependencies.data();

	VkRenderPass renderPass        = VK_NULL_HANDLE;
	VK_CheckResult( vkCreateRenderPass( VK_GetDevice(), &renderPassInfo, nullptr, &renderPass ), "Failed to create main render pass!" );

	gRenderPassInfo[ renderPass ] = { useMSAA };

	return gRenderPasses.Add( renderPass );
}


void VK_DestroyRenderPass( Handle shHandle )
{
	VkRenderPass renderPass = VK_NULL_HANDLE;

	if ( !gRenderPasses.Get( shHandle, &renderPass ) )
	{
		Log_Error( gLC_Render, "VK_DestroyRenderPass: Unable to find Render Pass!\n" );
		return;
	}

	gRenderPassInfo.erase( renderPass );
	vkDestroyRenderPass( VK_GetDevice(), renderPass, nullptr );
	gRenderPasses.Remove( shHandle );
}


void VK_DestroyRenderPasses()
{
	VK_DestroyMainRenderPass();

	VkRenderPass renderPass = VK_NULL_HANDLE;
	for ( size_t i = 0; i < gRenderPasses.size(); i++ )
	{
		if ( gRenderPasses.GetByIndex( i, &renderPass ) )
		{
			// gRenderPassInfo.erase( renderPass );
			vkDestroyRenderPass( VK_GetDevice(), renderPass, nullptr );
		}
	}

	gRenderPassInfo.clear();
	gRenderPasses.clear();
}


void VK_DestroyMainRenderPass()
{
	if ( gMainRenderPass != VK_NULL_HANDLE )
	{
		gRenderPassInfo.erase( gMainRenderPass );
		vkDestroyRenderPass( VK_GetDevice(), gMainRenderPass, nullptr );
	}

	gMainRenderPass = VK_NULL_HANDLE;
}


VkRenderPass VK_GetRenderPass()
{
	return VK_CreateMainRenderPass();
}


VkRenderPass VK_GetRenderPass( Handle shHandle )
{
	// fallback to main renderpass
	if ( shHandle == InvalidHandle )
		return VK_CreateMainRenderPass();

	VkRenderPass renderPass = VK_NULL_HANDLE;

	if ( gRenderPasses.Get( shHandle, &renderPass ) )
		return renderPass;

	Log_Error( gLC_Render, "Failed to Find RenderPass!\n" );
	return VK_NULL_HANDLE;
}


RenderPassInfoVK* VK_GetRenderPassInfo( VkRenderPass renderPass )
{
	auto it = gRenderPassInfo.find( renderPass );

	if ( it == gRenderPassInfo.end() )
	{
		Log_Error( gLC_Render, "Failed to find RenderPass info!\n" );
		return nullptr;
	}

	return &it->second;
}

