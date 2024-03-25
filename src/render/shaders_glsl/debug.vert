#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_EXT_nonuniform_qualifier : enable

#include "core.glsl"

layout(push_constant) uniform Push
{
    mat4 aModelMatrix;
    vec4 aColor;
	uint aRenderable;
	uint aViewport;
} push;

// layout(location = 0) in vec3 inPosition;
// layout(location = 1) in vec3 inColor;

layout(location = 0) out vec4 fragColor;

void main()
{
	// gl_Position = gViewport[0].aProjView * vec4(inPosition, 1.0);

    Renderable_t renderable = gRenderables[ push.aRenderable ];
	uint         vertIndex  = gl_VertexIndex;

	// is this renderable using an index buffer?
	if ( renderable.aIndexBuffer != 4294967295 )
		vertIndex = gIndexBuffers[ renderable.aIndexBuffer ].aIndex[ gl_VertexIndex ];

	VertexData_t vert = gVertexBuffers[ renderable.aVertexBuffer ].aVert[ vertIndex ];

	// mat4 inMatrix = gModelMatrices[ push.aRenderable ];
	vec3 inPos    = vert.aPosNormX.xyz;
	// vec3 inNorm   = vec3(vert.aPosNormX.w, vert.aNormYZ_UV.xy);
	// vec2 inUV     = vert.aNormYZ_UV.zw;
	vec4 inColor  = vert.aColor;

	// outPosition = inPos;
	// outPositionWorld = (inMatrix * vec4(outPosition, 1.0)).rgb;

	// gl_Position = gViewports[ push.aViewport ].aProjView * inMatrix * vec4(inPos, 1.0);
	gl_Position = gViewports[ push.aViewport ].aProjView * vec4(inPos, 1.0);
    fragColor   = inColor;
}

