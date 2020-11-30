// use opengl 3.2 core version!

#extension GL_ARB_gpu_shader5 : enable

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
	dir_in = (vertex_position.xyz + transpose(view_model_normal_matrix3)*vec3(view_model_matrix[3]))*texture_size;

	pos_in = vertex_texture_coord3;
}
