// use opengl 3.2 core shaders


in vec3 rast_texture_coord;
out vec4 frag_data;

uniform sampler3D sampler;

void main(void)
{
	float value = texture(sampler, rast_texture_coord).r;
	frag_data = vec4(value);
}
