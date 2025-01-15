#version 450

//shader input
layout (location = 0) in vec3 in_color;
layout (location = 1) in vec2 in_uv;

//output write
layout (location = 0) out vec4 out_frag_color;

void main() 
{
	out_frag_color = vec4( in_color, 1.0f );
}
