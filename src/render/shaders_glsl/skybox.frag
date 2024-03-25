#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_EXT_nonuniform_qualifier : enable

#define CH_FRAG_SHADER 1

#include "core.glsl"

layout(push_constant) uniform Push{
	mat4 matrix;
    int  sky;
	uint aRenderable;
	uint aViewport;
} push;

layout(location = 0) in vec3 fragTexCoord;

layout(location = 0) out vec4 outColor;

void main()
{
    outColor = texture( texSamplersCube[push.sky], fragTexCoord );
}

