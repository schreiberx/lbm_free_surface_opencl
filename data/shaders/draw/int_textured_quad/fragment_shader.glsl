// use opengl 3.2 core shaders


in vec2 rast_texture_coord;

out vec4 frag_data;

uniform usampler2D sampler;

void main(void)
{
	frag_data = vec4(texture(sampler, rast_texture_coord));
#if 0
	int sx = 32;
	int sy = 64;
	int sz = 16;
	int div = sx*sy*sz;
	frag_data = vec4(texture(sampler, rast_texture_coord)/vec4(div));
	frag_data = vec4(texture(sampler, rast_texture_coord)/vec4(sx,sy,sz,1));
	//frag_data = vec4(texture(sampler, rast_texture_coord)/vec4(16, 16, 16*16*16, 16*16*16));
#endif
}
