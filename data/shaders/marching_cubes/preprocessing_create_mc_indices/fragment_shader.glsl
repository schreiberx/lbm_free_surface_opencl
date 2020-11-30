// use opengl 3.2 core shaders


out uint mc_index;
uniform sampler3D fluid_fraction_texture;


float getFluidFraction(int x, int y, int z)
{
	return texelFetch(fluid_fraction_texture, ivec3(x,y,z), 0).r;
}


void main(void)
{
	/*
	 * compute 3d position
	 */

	ivec2 flat_pos = ivec2(floor(gl_FragCoord.xy));

	ivec4 pos;
	pos.x = flat_pos.x % DOMAIN_CELLS_X;
	pos.y = flat_pos.y % DOMAIN_CELLS_Y;
	pos.z = flat_pos.x / DOMAIN_CELLS_X + FT_Z_MOD*(flat_pos.y / DOMAIN_CELLS_Y);

	float ff000 = getFluidFraction(pos.x, pos.y, pos.z);
	float ff001 = getFluidFraction(pos.x+1, pos.y, pos.z);
	float ff010 = getFluidFraction(pos.x, pos.y+1, pos.z);
	float ff011 = getFluidFraction(pos.x+1, pos.y+1, pos.z);

	float ff100 = getFluidFraction(pos.x, pos.y, pos.z+1);
	float ff101 = getFluidFraction(pos.x+1, pos.y, pos.z+1);
	float ff110 = getFluidFraction(pos.x, pos.y+1, pos.z+1);
	float ff111 = getFluidFraction(pos.x+1, pos.y+1, pos.z+1);

	mc_index =	uint(
				(int(ff000 > ISO_VALUE) << 0)	|
				(int(ff001 > ISO_VALUE) << 1)	|
				(int(ff011 > ISO_VALUE) << 2)	|
				(int(ff010 > ISO_VALUE) << 3)	|
				(int(ff100 > ISO_VALUE) << 4)	|
				(int(ff101 > ISO_VALUE) << 5)	|
				(int(ff111 > ISO_VALUE) << 6)	|
				(int(ff110 > ISO_VALUE) << 7)
				);
}
