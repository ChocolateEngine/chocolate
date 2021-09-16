/*
shadercache.cpp ( Authored by p0lyh3dron )

Defines the shadercache declared in shadercache.h.  
*/
#include "../../../inc/core/renderer/shadercache.h"
/* Checks to see if a pipeline has already been compiled with the shader code.  */
bool ShaderCache::Exists( const std::string &srVertShader, const std::string &srFragShader, VkPipelineLayout sLayout )
{
        for ( uint32_t i = 0; i < aPipelines.size(  ); ++i )
		if ( srVertShader == aPipelines[ i ].aVertShader &&
		     srFragShader == aPipelines[ i ].aFragShader )
		        return aPipelineIndex = i;
	return false;
}
/* Get the pipeline indexed by a call to Exists(  ).  */
VkPipeline ShaderCache::GetPipeline(  )
{
	return aPipelines[ aPipelineIndex ].aPipeline;
}
/* Adds an allocated pipeline to the list of pipelines.  */
void ShaderCache::AddPipeline( const String &srVertShader, const String &srFragShader, VkPipelineLayout sLayout, VkPipeline sPipeline )
{
        aPipelines.push_back( { srVertShader, srFragShader, sLayout, sPipeline } );
}
/* Remove previous pipeline.  */
void ShaderCache::RemovePipeline(  )
{
	aPipelines.erase( aPipelines.begin(  ) + aPipelineIndex );
}
/* Clear the cache once the swapchain is outdated so new pipelines can be created.  */
void ShaderCache::ClearCache(  )
{
	aPipelines.clear(  );
}
