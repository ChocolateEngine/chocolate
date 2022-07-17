#include "rendergraph.h"
#include "renderpass.h"
#include "rendertarget.h"
#include "swapchain.h"
#include "graphics.h"


// in the future, you could use this to setup a render graph with these stages (somewhat varying order, some could be in parallel):
// this would also be built per frame, with visibility tests in game code (probably) for what to render, and other stuff
// also it most likely missing some things, like more lighting things probably
// 
// - if needed, run skeleton compute shader to update the vertex buffer for morphs and bone weights
// - deferred lighting shadowmapping pass (push new render target, render on that, and use as texture in game? not sure)
// - set viewport depth for viewmodel
// - draw viewmodel
// - reset viewport depth
// - shader passes that draw models with that unordered transparency thing maybe
// - set viewport depth for skybox (maybe not needed?)
// - skybox shader pass
// - reset viewport depth
// - draw screenspace effects (like screenspace reflections maybe?)
// - draw imgui (if it wasn't transparent, we could technically move it to before viewmodel drawing, unless it's possible in some way to prevent overdraw)
// - present to screen
//


RenderBufferResource& RenderGraphPass::AddBufferInput(
	const std::string& srName, VkPipelineStageFlags sStages, VkAccessFlags sAccess, VkBufferUsageFlags sUsage )
{
	RenderBufferResource& res = apGraph->GetBufferResource( srName );
	res.aUsedQueues |= aQueue;
	res.aBufferUsage |= sUsage;
	res.aReadInPasses.insert( aIndex );

	aBufferResources.emplace_back(
		res.aIndex,
		sStages,
		sAccess,
		VK_IMAGE_LAYOUT_GENERAL
	);

	return res;
}


RenderBufferResource& RenderGraphPass::AddVertexBufferInput( const std::string& srName )
{
	return AddBufferInput( srName, VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT );
}


RenderBufferResource& RenderGraphPass::AddIndexBufferInput( const std::string& srName )
{
	return AddBufferInput( srName, VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, VK_ACCESS_INDEX_READ_BIT, VK_BUFFER_USAGE_INDEX_BUFFER_BIT );
}


// ------------------------------------------------------------------------


HRenderPass RenderGraph::CreateRenderPass( const std::string& srName, RenderGraphQueueFlagBits sStage )
{
	auto it = aPassToIndex.find( srName );

	if ( it == aPassToIndex.end() )
	{
		// Create new pass
		u32 index = aPasses.size();

		HRenderPass pass = aPasses.Add( new RenderGraphPass( this, index, sStage ) );

		// RenderGraphPass& pass = aPasses.emplace_back( srName, index, sStage );
		aPassToIndex[ srName ] = index;
		return pass;
	}
	else
	{
		// return aPasses.Get( it->second );
		return it->second;
	}
}


RenderGraphPass* RenderGraph::GetRenderPass( HRenderPass handle )
{
	return aPasses.Get( handle );
}


RenderBufferResource& RenderGraph::GetBufferResource( const std::string& srName )
{
	auto it = aResourceToIndex.find( srName );
	
	if ( it == aResourceToIndex.end() )
	{
		// Create new resource
		u32 index = aResources.size();
		RenderBufferResource* res = new RenderBufferResource( index );
		aResources.push_back( res );
		res->aName = srName;
		aResourceToIndex[ srName ] = index;
		return *res;
	}
	else
	{
		RenderResource* res = aResources[ it->second ];
		Assert( res->aType == RenderResource::Type::Buffer );
		return (RenderBufferResource&)(*res);
	}
}


RenderTextureResource& RenderGraph::GetTextureResource( const std::string& srName )
{
	auto it = aResourceToIndex.find( srName );

	if ( it == aResourceToIndex.end() )
	{
		// Create new resource
		u32 index = aResources.size();
		RenderTextureResource* res = new RenderTextureResource( index );
		aResources.push_back( res );
		res->aName = srName;
		aResourceToIndex[ srName ] = index;
		return *res;
	}
	else
	{
		RenderResource* res = aResources[ it->second ];
		Assert( res->aType == RenderResource::Type::Texture );
		return (RenderTextureResource&)(*res);
	}
}


bool RenderGraph::Bake()
{
	if ( !ValidatePasses() )
		return false;
	
	// TODO: make sure we have a backbuffer

	// TODO: do sorting of dependencies by working our way back from the backbuffer

	// 
	// FlattenPasses();
	// 
	// ReorderPasses();

	// build physical passes to be used in the GPU execution stage

}


bool RenderGraph::ValidatePasses()
{
	for ( size_t i = 0; i < aPasses.size(); i++ )
	{
		RenderGraphPass* pass = aPasses.GetByIndex( i );
	
		// make sure the size of each type of input and output are equal
		if ( pass->aColorInputs.size() != pass->aColorOutputs.size() )
		{
			LogError( "Color input/output mismatch for pass \"%s\"\n", pass->aName.c_str() );
			return false;
		}

		if ( pass->aDepthStencilInput && pass->aDepthStencilOutput )
		{
			// check dimensions
		}
	}
}


// ------------------------------------------------------------------------


void CommandBufferHelper::RunCommand( VkCommandBuffer c, DeviceCommand& command )
{
	switch ( command.aType )
	{
		case CommandType::ExecuteCommands:
		{
			CmdExecuteCommands_t* executeCommands = command.apExecuteCommands;
			vkCmdExecuteCommands( c, executeCommands->aCommands.size(), executeCommands->aCommands.data() );
			break;
		}
		
		case CommandType::BeginRenderPass:
		{
			CmdBeginRenderPass_t* beginRenderPass = command.apRenderPass;
			vkCmdBeginRenderPass( c, &beginRenderPass->aInfo, beginRenderPass->aContents );
			break;
		}

		case CommandType::EndRenderPass:
		{
			vkCmdEndRenderPass( c );
			break;
		}
		
		case CommandType::BindPipeline:
		{
			CmdBindPipeline_t* bindPipeline = command.apBindPipeline;
			vkCmdBindPipeline( c, bindPipeline->aPipelineBindPoint, bindPipeline->aPipeline );
			break;
		}

		case CommandType::BindVertexBuffer:
		{
			CmdBindVertexBuffer_t* info = command.apVertexBuffer;
			vkCmdBindVertexBuffers( c, info->aFirstBinding, info->aBuffers.size(), info->aBuffers.data(), info->aOffsets.data() );
			break;
		}

		case CommandType::BindIndexBuffer:
		{
			CmdBindIndexBuffer_t* info = command.apIndexBuffer;
			vkCmdBindIndexBuffer( c, info->aBuffer, info->aOffset, info->aIndexType );
			break;
		}

		case CommandType::BindDescriptorSet:
		{
			CmdBindDescriptorSet_t* info = command.apDescSet;
			vkCmdBindDescriptorSets( c, info->aPipelineBindPoint, aCurLayout, info->aFirstSet, info->aDescriptorSets.size(), info->aDescriptorSets.data(), info->aDynamicOffsets.size(), info->aDynamicOffsets.data() );
			break;
		}

		case CommandType::PushConstants:
		{
			CmdPushConstants_t* info = command.apPushConstants;
			vkCmdPushConstants( c, aCurLayout, info->aStageFlags, info->aOffset, info->aSize, info->apData );
			break;
		}

		case CommandType::Draw:
		{
			CmdDraw_t* info = command.apDraw;
			vkCmdDraw( c, info->aVertexCount, info->aInstanceCount, info->aFirstVertex, info->aFirstInstance );
			break;
		}

		case CommandType::DrawIndexed:
		{
			CmdDrawIndexed_t* info = command.apDrawIndexed;
			vkCmdDrawIndexed( c, info->aIndexCount, info->aInstanceCount, info->aFirstIndex, info->aVertexOffset, info->aFirstInstance );
			break;
		}

		default:
			break;
	}
}


// is part of Device in hydra graphics
void CommandBufferHelper::ExecuteCommands( VkCommandBuffer c )
{
	for ( auto& command : aCommands )
	{
		RunCommand( c, command );
	}
}


void CommandBufferHelper::AddPushConstants( void* spData, u32 sOffset, u32 sSize, u32 sStageFlags )
{
	CmdPushConstants_t* info = new CmdPushConstants_t;
	info->apData = spData;
	info->aOffset = sOffset;
	info->aSize = sSize;
	info->aStageFlags = sStageFlags;
	
	DeviceCommand& cmd = aCommands.emplace_back( CommandType::PushConstants );
	cmd.apPushConstants = info;
}

