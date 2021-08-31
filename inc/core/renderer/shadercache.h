/*
shadercache.h ( Authored by p0lyh3dron )

Declares the shader cache that will be
used to optimize and share graphics
pipelines between models.  
*/
#pragma once

#include <vulkan/vulkan.hpp>
#include <map>
#include <string>

class PipelineInfo
{
public:
	std::string 		aVertShader;
	std::string		aFragShader;
	VkPipelineLayout	aLayout;
	VkPipeline		aPipeline;
};

class ShaderCache
{
	typedef std::vector< PipelineInfo >  	AllocatedPipelines;
	typedef std::string		        String;
protected:
	AllocatedPipelines	aPipelines;
        VkPipeline		aPipelineReturn;
public:
	/* Checks to see if a pipeline has already been compiled with the shader code.  */
	bool		Exists( const std::string &srVertShader, const std::string &srFragShader, VkPipelineLayout sLayout );
	/* Get the pipeline indexed by a call to Exists(  ).  */
        VkPipeline	GetPipeline(  );
	/* Adds an allocated pipeline to the list of pipelines.  */
	void		AddPipeline( const String &srVertShader, const String &srFragShader, VkPipelineLayout sLayout, VkPipeline sPipeline );
};
