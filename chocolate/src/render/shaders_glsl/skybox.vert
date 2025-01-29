#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_EXT_nonuniform_qualifier : enable

#include "core.glsl"

layout(push_constant) uniform Push{
	mat4 matrix;
    int  sky;
	uint aRenderable;
	uint aViewport;
} push;

// layout(location = 0) in vec3 inPosition;
layout(location = 0) out vec3 fragTexCoord;

void main()
{
    Renderable_t renderable = gRenderables[ push.aRenderable ];
	uint         vertIndex  = gl_VertexIndex;

	// is this renderable using an index buffer?
	if ( renderable.aIndexBuffer != 4294967295 )
		vertIndex = gIndexBuffers[ renderable.aIndexBuffer ].aIndex[ gl_VertexIndex ];

	VertexData_t vert = gVertexBuffers[ renderable.aVertexBuffer ].aVert[ vertIndex ];

	// mat4 inMatrix = gModelMatrices[ push.aRenderable ];
	vec3 inPos    = vert.aPosNormX.xyz;

    fragTexCoord = inPos;
	gl_Position  = gViewports[ push.aViewport ].aProjection * push.matrix * vec4(inPos, 1.0);
}

