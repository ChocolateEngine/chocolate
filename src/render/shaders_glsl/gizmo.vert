#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_EXT_nonuniform_qualifier : enable

#include "core.glsl"


#define GIZMO_COLOR_MODE_NONE     0
#define GIZMO_COLOR_MODE_HOVERED  ( 1 << 0 )
#define GIZMO_COLOR_MODE_SELECTED ( 1 << 1 )

#define GIZMO_COLOR_MULT_HOVERED  1.4
#define GIZMO_COLOR_MULT_SELECTED 1.2


layout(push_constant) uniform Push
{
    mat4 aModelMatrix;
    vec4 aColor;
	uint aRenderable;
	uint aViewport;
	uint aColorMode;
} push;

// layout(location = 0) in vec3 inPosition;

layout(location = 0) out vec4 fragColor;

vec4 Desaturate( vec3 color, float factor )
{
    vec3 lum = vec3(0.299, 0.587, 0.114);
	vec3 gray = vec3(dot(lum, color));
	return vec4(mix(color, gray, factor), 1.0);
}

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
	// vec3 inNorm   = vec3(vert.aPosNormX.w, vert.aNormYZ_UV.xy);
	// vec2 inUV     = vert.aNormYZ_UV.zw;
	// vec4 inColor  = vert.aColor;

	// outPosition = inPos;
	// outPositionWorld = (inMatrix * vec4(outPosition, 1.0)).rgb;

	gl_Position = gViewports[ push.aViewport ].aProjView * push.aModelMatrix * vec4(inPos, 1.0);
    fragColor   = push.aColor;

	vec4 baseMultColor = vec4( 0.1, 0.1, 0.1, 1.0 );

	// uint hovered = push.aColorMode & GIZMO_COLOR_MODE_HOVERED;
	// 
	// if ( hovered > 0 )
	// {
	// 	fragColor *= baseMultColor;
	// }

	uint selected = push.aColorMode & GIZMO_COLOR_MODE_SELECTED;
	
	if ( selected > 0 )
	{
		fragColor *= baseMultColor;
	}
}
