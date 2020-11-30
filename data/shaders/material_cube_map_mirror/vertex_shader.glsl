// use opengl 3.2 core version!


in vec4 vertex_position;
in vec3 vertex_normal3;

out vec3 frag_normal3;
out vec3 eye_direction3;

uniform mat4 pvm_matrix;
uniform mat3 normal_matrix3;
uniform mat4 view_model_matrix;
uniform mat4 view_matrix;

void main(void)
{
	gl_Position = pvm_matrix*vertex_position;
	frag_normal3 = normal_matrix3*vertex_normal3;

	// compute vector from origin to fragment position
	eye_direction3 = vec3(view_model_matrix*vertex_position);
}
