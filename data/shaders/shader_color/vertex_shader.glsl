// use opengl 3.2 core version!


in vec4 vertex_position;
uniform mat4 pvm_matrix;



void main(void)
{
	gl_Position = pvm_matrix*vertex_position;
}
