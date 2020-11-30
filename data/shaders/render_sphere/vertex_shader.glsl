// use opengl 3.2 core version!


in vec4 vertex_position;

uniform mat4 pvm_matrix;
uniform mat3 normal_matrix;

out vec3 frag_normal;

void main(void)
{
	gl_Position = pvm_matrix*vertex_position;
	frag_normal = normal_matrix*vec3(vertex_position);	// vertex position == vertex normal for sphere
}
