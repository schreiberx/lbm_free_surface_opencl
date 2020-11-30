// use opengl 3.2 core version!


in vec4 vertex_position;
in vec3 vertex_normal;

uniform mat4 pvm_matrix;
uniform mat3 pvm_normal_matrix3;
uniform mat4 view_model_matrix;
uniform mat3 view_model_normal_matrix3;

out vec3 frag_position3;
out vec3 frag_normal3;

void main(void)
{
	gl_Position = pvm_matrix*vertex_position;

	frag_position3 = vec3(view_model_matrix*vertex_position);
	frag_normal3 = view_model_normal_matrix3*vertex_normal;
}

