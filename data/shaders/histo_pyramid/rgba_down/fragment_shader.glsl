// use opengl 3.2 core shaders

/*
 * this histogram version expects the data in all color channels of the texels (RGBA)
 */
out uvec4 frag_data;

uniform int current_down_level;
uniform int current_up_level;
uniform usampler2D down_texture;	// texture created in previous step
uniform usampler2D up_texture;		// texture created in bottom-up process


void main(void)
{
	// compute position
	ivec2 pos = ivec2(gl_FragCoord);

	// compute the id \in [0;4] within 2x2 field to distinguish the different enumerations
	int id = ((pos.y)&1)*2+((pos.x)&1);

	// TODO: test, if array access can be optimized e.g. by using if/switch
	frag_data.r = texelFetch(down_texture, pos >> 1, current_down_level)[id];

	uvec4 base_displ = texelFetch(up_texture, pos, current_up_level);	// displacement for current cell


	frag_data.g = frag_data.r + base_displ.r;
	frag_data.b = frag_data.g + base_displ.g;
	frag_data.a = frag_data.b + base_displ.b;

	return;
}
