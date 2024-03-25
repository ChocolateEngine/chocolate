#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_EXT_nonuniform_qualifier : enable

#define CH_FRAG_SHADER 1

#include "core.glsl"
#include "common_lighting.glsl"

#define VIEWPORT 0
#define MATERIAL 1

layout(push_constant) uniform Push
{
    vec3 aColor;
	uint aRenderable;
	uint aViewport;
} push;

// Material Info
layout(set = CH_DESC_SET_PER_SHADER, binding = 0) buffer readonly Buffer_Material
{
    int aNormalMap;
} materials[];

//layout(location = 0) in vec2 fragTexCoord;
//layout(location = 1) in vec3 inPosition;

layout(location = 0) in vec3 inPositionWorld;
layout(location = 1) in vec3 inNormalWorld;

layout(location = 0) out vec4 outColor;

// #define mat materials[ push.aMaterial ]
// #define mat materials[0]


void main()
{
    // TODO: transparency
    // outColor = vec4(push.aColor, 1.0);
    // outColor = vec4(1.0, 0.0, 0.5, 1.0);

    outColor = AddLighting( vec4(push.aColor, 1.0), inPositionWorld, inNormalWorld );
}

// look at this later for cubemaps:
// https://forum.processing.org/two/discussion/27871/use-samplercube-and-sampler2d-in-a-single-glsl-shader.html

