// use opengl 3.2 core shaders


out uvec4 frag_data;

uniform int current_level;
uniform usampler2D src_texture;

void main(void)
{
	ivec2 read_pos = ivec2(gl_FragCoord) << 1;
	uvec4 data;

	data = texelFetch(src_texture, read_pos, current_level);
	frag_data.r = data.r + data.g + data.b + data.a;

	data = texelFetch(src_texture, ivec2(read_pos.x+1, read_pos.y), current_level);
	frag_data.g = data.r + data.g + data.b + data.a;

	data = texelFetch(src_texture, ivec2(read_pos.x, read_pos.y+1), current_level);
	frag_data.b = data.r + data.g + data.b + data.a;

	data = texelFetch(src_texture, ivec2(read_pos.x+1, read_pos.y+1), current_level);
	frag_data.a = data.r + data.g + data.b + data.a;
}
