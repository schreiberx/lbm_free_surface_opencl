// use opengl 3.2 core version!


in vec4 vertex_position;
in vec3 vertex_normal;

uniform mat4 pvm_matrix;
uniform mat3 view_model_normal_matrix3;


out vec3 out_vertex_normal;


void main(void)
{
	gl_Position = pvm_matrix*vertex_position;
	out_vertex_normal = view_model_normal_matrix3*vertex_normal;
}
