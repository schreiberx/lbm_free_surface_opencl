// use opengl 3.2 core version!


in vec4 vertex_pos;

void main(void)
{
	gl_Position = vertex_pos;	// ortho draw
}
