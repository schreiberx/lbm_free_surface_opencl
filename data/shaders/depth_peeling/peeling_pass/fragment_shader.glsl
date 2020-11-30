// use opengl 3.2 core shaders


out vec4 frag_data;
out vec4 frag_normal;

in vec3 out_vertex_normal;

uniform int prev_depth_layer;

uniform sampler2DArray prev_depth_texture;

void main(void)
{
	float prev_depth = texelFetch(prev_depth_texture, ivec3(ivec2(gl_FragCoord.xy), prev_depth_layer), 0).r;
	if (gl_FragCoord.z <= prev_depth+0.000001)
	{
		discard;
		return;
	}

	vec3 normal = normalize(out_vertex_normal);
	frag_data = vec4(abs(dot(normal, normalize(vec3(1,1,3)))));
	frag_normal.xyz = normal;
}
