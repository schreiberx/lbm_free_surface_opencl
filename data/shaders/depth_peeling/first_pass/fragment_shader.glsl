// use opengl 3.2 core shaders


out vec4 frag_data;
out vec4 frag_normal;

in vec3 out_vertex_normal;

void main(void)
{
	vec3 normal = normalize(out_vertex_normal);
	frag_data = vec4(abs(dot(normal, normalize(vec3(1,1,3)))));
	frag_normal.xyz = normal;
}
