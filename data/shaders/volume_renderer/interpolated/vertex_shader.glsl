// use opengl 3.2 core version!


in vec4 vertex_position;
in vec3 vertex_texture_coord3;

out vec3 pos_in;
out vec3 dir_in;

uniform mat4 pvm_matrix;
uniform mat4 view_model_matrix;
uniform mat3 view_model_normal_matrix3;

uniform vec3 texture_size;
uniform vec3 inv_texture_size;

void main(void)
{
	gl_Position = pvm_matrix*vertex_position;
	/*
	 * if the volume is scaled, the vertex position is displaced.
	 * therefore we have to scale it by the texture_size!
	 */
	dir_in = (vertex_position.xyz + vec3(view_model_matrix[3])*view_model_normal_matrix3)*texture_size;

	// avoid clipping!
//	float positive = float(gl_Position.z < 0.0);
//	gl_Position.z = gl_Position.w*positive + (1.0f-positive)*gl_Position.z;

	pos_in = vertex_texture_coord3;
}
