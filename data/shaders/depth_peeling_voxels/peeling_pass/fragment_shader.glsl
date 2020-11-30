// use opengl 3.2 core shaders


out vec4 frag_data;
out vec4 frag_normal;

in vec3 out_vertex_normal;
in vec3 out_vertex_position;

uniform int prev_depth_layer;

uniform sampler2DArray prev_depth_texture;

void main(void)
{
	float prev_depth = texelFetch(prev_depth_texture, ivec3(ivec2(gl_FragCoord.xy), prev_depth_layer), 0).r;

	if (gl_FragCoord.z <= prev_depth+0.00001)
	{
		discard;
		return;
	}

	frag_normal.xyz = normalize(out_vertex_normal);
	frag_data.xyz = out_vertex_position;
}
