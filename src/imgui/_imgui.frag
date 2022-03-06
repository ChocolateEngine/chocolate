// Compile with: glslangValidator -V -x -o _imgui.frag.u32 _imgui.frag
#version 450 core

layout(location = 0) out vec4 fColor;
layout(set=0, binding=0) uniform sampler2D sTexture;
layout(location = 0) in struct { vec4 Color; vec2 UV; } In;

// fix color space issue
vec3 rec709_eotf(vec3 v_)
{
	mat2x3 v = mat2x3( (v_/4.5), pow((v_+0.0999)/1.099, vec3(2.22222)) );
	return vec3(
		v[v_[0] < 0.081 ? 0 : 1][0],
		v[v_[1] < 0.081 ? 0 : 1][1],
		v[v_[2] < 0.081 ? 0 : 1][2] );
}


// funny blur shader
// https://github.com/Jam3/glsl-fast-gaussian-blur/blob/master/9.glsl
vec4 blur9(sampler2D image, vec2 uv, vec2 resolution, vec2 direction)
{
  vec4 color = vec4(0.0);
  vec2 off1 = vec2(1.3846153846) * direction;
  vec2 off2 = vec2(3.2307692308) * direction;
  color += texture(image, uv) * 0.2270270270;
  color += texture(image, uv + (off1 / resolution)) * 0.3162162162;
  color += texture(image, uv - (off1 / resolution)) * 0.3162162162;
  color += texture(image, uv + (off2 / resolution)) * 0.0702702703;
  color += texture(image, uv - (off2 / resolution)) * 0.0702702703;
  return color;
}


void main()
{
    fColor = In.Color * texture(sTexture, In.UV.st);
	
	  //fColor *= blur9( sTexture, In.UV.st, vec2(30, 30), vec2(2, 2) );
	
    // even though this is supposedly wrong,
    // it still looks more accurate than the "correct" color conversion function above
	  fColor = vec4(pow(fColor.rgb, vec3(2.2)), fColor.a);
    // fColor = vec4(rec709_eotf(fColor.rgb), fColor.a);
}

