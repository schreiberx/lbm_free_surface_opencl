// use opengl 3.2 core version!


in vec3 vertex_position;
out vec3 rast_texture_coord;

uniform mat4 pvm_matrix;

void main(void)
{
	gl_Position = pvm_matrix*vec4(vertex_position, 0);
	gl_Position.z = gl_Position.w*0.9999;
	rast_texture_coord = vertex_position;
}
