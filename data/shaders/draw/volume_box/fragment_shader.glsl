// use opengl 3.2 core shaders


smooth in vec4 rast_texture_coord;
out vec4 frag_data;

void main(void)
{
	frag_data = vec4(rast_texture_coord);
}
