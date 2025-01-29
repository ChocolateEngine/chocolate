#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_EXT_nonuniform_qualifier : enable

#define CH_FRAG_SHADER 1

#include "core.glsl"

layout(push_constant) uniform SelectParams {
	uint aRenderable;
	uint aViewport;
	uint aVertexCount;
	uint aDiffuse;
	vec4 aColor;
	vec2 aCursorPos;
}
push;

layout(location = 0) in vec2 inTexCoord;

layout(location = 0) out vec4 outColor;


void main()
{
	// if ( push.aDiffuse >= 0 && texture( texSamplers[ push.aDiffuse ], inTexCoord ).a < 0.5)
	// 	discard;
		
	outColor = push.aColor;
}

