/*
shadercache.cpp ( Authored by p0lyh3dron )

Defines the shadercache declared in shadercache.h.  
*/
#include "../../../inc/core/renderer/shadercache.h"
/* Checks to see if a pipeline has already been compiled with the shader code.  */
bool ShaderCache::Exists( const std::string &srVertShader, const std::string &srFragShader, VkPipelineLayout sLayout )
{
        for ( const auto& pipeline : aPipelines )
		if ( srVertShader == pipeline.aVertShader &&
		     srFragShader == pipeline.aFragShader )
		        return aPipelineReturn = pipeline.aPipeline;
	return false;
}
/* Get the pipeline indexed by a call to Exists(  ).  */
VkPipeline ShaderCache::GetPipeline(  )
{
	return aPipelineReturn;
}
/* Adds an allocated pipeline to the list of pipelines.  */
void ShaderCache::AddPipeline( const String &srVertShader, const String &srFragShader, VkPipelineLayout sLayout, VkPipeline sPipeline )
{
        aPipelines.push_back( { srVertShader, srFragShader, sLayout, sPipeline } );
}
