#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_EXT_nonuniform_qualifier : enable

layout(push_constant) uniform Push{
    mat4 aModelMatrix;
	int  aViewInfo;
    int  aDiffuse;
} push;

// viewport
layout(set = 1, binding = 0) buffer Storage_Viewport
{
	mat4 aProjView;
	mat4 aProjection;
	mat4 aView;
	vec3 aViewPos;
	float aNearZ;
	float aFarZ;
} gViewport[];

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 inTexCoord;

layout(location = 0) out vec2 fragTexCoord;

void main()
{
	gl_Position = gViewport[push.aViewInfo].aProjView * push.aModelMatrix * vec4(inPosition, 1.0);
	fragTexCoord = inTexCoord;
}

