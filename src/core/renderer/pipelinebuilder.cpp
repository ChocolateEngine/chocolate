/*
pipelinebuilder.cpp ( Authored by p0lyh3dron )

Defines the pipelinebuilder declared in pipelinebuilder.h.  
*/
#include "../../../inc/core/renderer/pipelinebuilder.h"
#include "../../../inc/core/renderer/allocator.h"

#include <future>
/* Queues a pipeline to be built.  */
void PipelineBuilder::Queue( VkPipeline *spPipeline, VkPipelineLayout *spLayout, Layouts *spLayouts, const String &srVertShader, const String &srFragShader, int sFlags )
{
	aQueue.push_back( { spLayouts, srVertShader, srFragShader, sFlags, spPipeline, spLayout } );
}
/* Asynchronously builds all descriptor set layouts in aQueue.  */
void PipelineBuilder::BuildPipelines( VkPipeline *spSinglePipeline )
{
	/* Make a single-time pipeline ( a new model is loaded but other pipelines do not require a rebuild )  */
	if ( spSinglePipeline )
		for ( auto&& pipeline : aQueue )
			if ( pipeline.apPipeline == spSinglePipeline )
			{
				*pipeline.apLayout = InitPipelineLayouts( pipeline.apSetLayouts->GetBuffer(  ), pipeline.apSetLayouts->GetSize(  ) );
				*spSinglePipeline = InitGraphicsPipeline< vertex_3d_t >( *pipeline.apLayout, pipeline.aVShader, pipeline.aFShader, pipeline.aFlags );
				return;
			}
	
	auto buildFunction = [ & ]( QueuedPipeline sPipeline )
		{
			*sPipeline.apLayout   = InitPipelineLayouts( sPipeline.apSetLayouts->GetBuffer(  ), sPipeline.apSetLayouts->GetSize(  ) );
			*sPipeline.apPipeline = InitGraphicsPipeline< vertex_3d_t >( *sPipeline.apLayout, sPipeline.aVShader, sPipeline.aFShader, sPipeline.aFlags );
		};

	for ( auto&& pipeline : aQueue )
	        buildFunction( pipeline );//std::async( std::launch::async, buildFunction, pipeline );
}
