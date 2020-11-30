// use opengl 3.2 core shaders


out vec4 frag_data;
out vec4 frag_normal;

in vec3 out_vertex_normal;
in vec3 out_vertex_position;

void main(void)
{
	// store the depth component to the alpha channel!!!
	frag_data = vec4(out_vertex_position, gl_FragCoord.z);
	frag_normal.xyz = normalize(out_vertex_normal);
}
