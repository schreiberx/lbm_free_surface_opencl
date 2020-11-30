// use opengl 3.2 core shaders
//

out vec4 frag_data;

uniform vec4 background_color;

void main(void)
{
	frag_data = background_color;
}
