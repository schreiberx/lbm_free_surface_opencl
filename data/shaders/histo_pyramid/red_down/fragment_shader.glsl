// use opengl 3.2 core shaders

/*
 * this histogram version expects the data only in the RED color channes of the texels and writes it down only to the red channel
 */

out int frag_data;

uniform int current_level;
uniform isampler2D down_texture;	// texture created in previous step
uniform isampler2D up_texture;	// texture created in bottom-up process


void main(void)
{
	ivec2 pos = ivec2(gl_FragCoord);
	int id = ((pos.y)&1)*2+((pos.x)&1);

	pos >>= 1;
	frag_data = texelFetch(down_texture, pos, current_level+1).r;
	if (id == 0)	return;

	pos <<= 1;	// eliminate 1st bit

	frag_data += texelFetch(up_texture, pos, current_level).r;
	if (id == 1)	return;

	frag_data += texelFetch(up_texture, ivec2(pos.x+1, pos.y), current_level).r;
	if (id == 2)	return;

	frag_data += texelFetch(up_texture, ivec2(pos.x, pos.y+1), current_level).r;
}
