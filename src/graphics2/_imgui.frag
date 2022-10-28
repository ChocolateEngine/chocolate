// Compile with: glslangValidator -V -x -o _imgui.frag.u32 _imgui.frag
#version 450 core

layout(location = 0) out vec4 fColor;
layout(set=0, binding=0) uniform sampler2D sTexture;
layout(location = 0) in struct { vec4 Color; vec2 UV; } In;

// https://gamedev.stackexchange.com/questions/92015/optimized-linear-to-srgb-glsl
// Converts a color from linear light gamma to sRGB gamma
vec4 fromLinear( vec4 linearRGB )
{
    bvec3 cutoff = lessThan(linearRGB.rgb, vec3(0.0031308));
    vec3 higher = vec3(1.055)*pow(linearRGB.rgb, vec3(1.0/2.4)) - vec3(0.055);
    vec3 lower = linearRGB.rgb * vec3(12.92);

    return vec4(mix(higher, lower, cutoff), linearRGB.a);
}

// Converts a color from sRGB gamma to linear light gamma
// REALLY close, but still not perfect
vec4 toLinear( vec4 sRGB )
{
    bvec3 cutoff = lessThan(sRGB.rgb, vec3(0.04045));
    vec3 higher = pow((sRGB.rgb + vec3(0.055))/vec3(1.055), vec3(2.4));
    vec3 lower = sRGB.rgb/vec3(12.92);

    return vec4(mix(higher, lower, cutoff), sRGB.a);
}

void main()
{
    fColor = In.Color * texture(sTexture, In.UV.st);
	
    // Convert Color Space
    fColor = toLinear( fColor );
}

