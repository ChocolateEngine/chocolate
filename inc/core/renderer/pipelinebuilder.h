/* 
pipelinebuilder.h ( Authored by p0lyh3dron )

Declares the pipelinebuilder class which will
asynchronously build pipelines.
*/
#pragma once

#include <string>
#include <vector>
#include <vulkan/vulkan.hpp>
#include "../../types/databuffer.hh"

typedef std::string	String;
typedef DataBuffer< VkDescriptorSetLayout > Layouts;

class QueuedPipeline
{
public:
	Layouts			*apSetLayouts;
	String			aVShader;
	String			aFShader;
	int			aFlags;
	VkPipeline		*apPipeline;
	VkPipelineLayout	*apLayout;
};

class PipelineBuilder
{
	typedef std::vector< QueuedPipeline >	PipelineQueue;
protected:
	PipelineQueue	aQueue;
public:
	/* Queues a pipeline to be built.  */
	void	Queue( VkPipeline *spPipeline, VkPipelineLayout *spLayout, Layouts *spLayouts, const String &srVertShader, const String &srFragShader, int sFlags );
	/* Asynchronously builds all descriptor set layouts in aQueue.  */
	void	BuildPipelines( VkPipeline *spSinglePipeline = NULL );
	/* Remove a pipeline from the queue. */
	void	RemovePipeline( VkPipeline *spSinglePipeline );
};
