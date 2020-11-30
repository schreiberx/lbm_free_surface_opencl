// use opengl 3.2 core version!


/**
 * this shader is used to convert raw data from opencl arrays which was copied to a RGBA texture to a flat texture
 */
in vec4 vertex_position;

void main(void)
{
	gl_Position = vertex_position;
}
