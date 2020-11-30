// use opengl 3.2 core shaders


out int frag_data;

uniform int current_level;
uniform isampler2D src_texture;

void main(void)
{
	ivec2 read_pos = ivec2(gl_FragCoord)*2;

	frag_data 	=	texelFetch(src_texture, read_pos, current_level).r							+
					texelFetch(src_texture, ivec2(read_pos.x+1, read_pos.y), current_level).r	+
					texelFetch(src_texture, ivec2(read_pos.x, read_pos.y+1), current_level).r	+
					texelFetch(src_texture, ivec2(read_pos.x+1, read_pos.y+1), current_level).r;
}
