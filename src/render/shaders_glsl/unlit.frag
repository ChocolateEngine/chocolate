#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_EXT_nonuniform_qualifier : enable

layout(push_constant) uniform Push{
    mat4 aModelMatrix;
	int  aViewInfo;
    int  aDiffuse;
} push;

layout(set = 0, binding = 0) uniform sampler2D[] texSampler;

layout(location = 0) in vec2 fragTexCoord;

layout(location = 0) out vec4 outColor;

void main() {
    outColor = texture(texSampler[push.aDiffuse], fragTexCoord);
}
