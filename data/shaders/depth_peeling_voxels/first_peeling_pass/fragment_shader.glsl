// use opengl 3.2 core shaders


out vec4 frag_data;
out vec4 frag_normal;

in vec3 out_vertex_normal;
in vec3 out_vertex_position;

uniform vec2 depth_texture_access_scale;

uniform sampler2D prev_depth_texture;

void main(void)
{
	float prev_depth = texture(prev_depth_texture, gl_FragCoord.xy*depth_texture_access_scale).r;
	if (gl_FragCoord.z <= prev_depth+0.00001)
	{
		discard;
		return;
	}

	frag_normal.xyz = normalize(out_vertex_normal);
	frag_data.xyz = out_vertex_position;
}
