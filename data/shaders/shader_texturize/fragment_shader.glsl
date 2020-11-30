// use opengl 3.2 core shaders


in vec2 rast_texture_coord;

out vec4 frag_data;

uniform sampler2D sampler;

void main(void)
{
	frag_data = texture(sampler, rast_texture_coord);
	return;
}
