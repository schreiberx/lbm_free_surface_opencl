#ifndef DOMAIN_CELLS_X
	#define DOMAIN_CELLS_X	(64)
	#define DOMAIN_CELLS_Y	(64)
	#define DOMAIN_CELLS_Z	(64)
	#define DOMAIN_CELLS	(DOMAIN_CELLS_X*DOMAIN_CELLS_Y*DOMAIN_CELLS_Z)

	#define FT_Z_MOD	(16)

	#define ISO_VALUE	(0.5)
#endif

inline float getFluidFraction(
			int x, int y, int z,
			__global float *fluid_fraction_array
)
{
	return fluid_fraction_array[x + y*DOMAIN_CELLS_X + z*(DOMAIN_CELLS_X*DOMAIN_CELLS_Y)];
}


const sampler_t tv_sampler = CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_NONE | CLK_FILTER_NEAREST;

__kernel void main(
		__global float fluid_fraction_array[DOMAIN_CELLS],
		write_only image2d_t mc_index_array,
		write_only image2d_t vertex_array,
		write_only image2d_t normal_array
		)
{
	const size_t gid = get_global_id(0);

	if (gid >= TEXTURE_PIXELS)
		return;

	/*
	 * compute 3d position
	 */
	int4 pos;
	pos.x = gid % DOMAIN_CELLS_X;
	pos.y = (gid / DOMAIN_CELLS_X) % DOMAIN_CELLS_Y;
	pos.z = gid / (DOMAIN_CELLS_X*DOMAIN_CELLS_Y);

	/*
	 * compute flat texture position
	 */
	int2 texture_pos;
	texture_pos.x = pos.x + (pos.z % FT_Z_MOD)*DOMAIN_CELLS_X;
	texture_pos.y = pos.y + (pos.z / FT_Z_MOD)*DOMAIN_CELLS_Y;

	// relative position for vertices
	float4 fpos;
	fpos.x = (float)pos.x;
	fpos.y = (float)pos.y;
	fpos.z = (float)pos.z;

	if (	pos.x >= DOMAIN_CELLS_X-1	||
			pos.y >= DOMAIN_CELLS_Y-1	||
			pos.z >= DOMAIN_CELLS_Z-1
		)
	{
			// we have to write a zero value to avoid rendering vertices
			write_imagef(mc_index_array, texture_pos, 0.0);
			return;
	}


	/**
	 * first interpolate to x and y axes
	 */
	float ff000 = getFluidFraction(pos.x, pos.y, pos.z, fluid_fraction_array);
	float ff001 = getFluidFraction(pos.x+1, pos.y, pos.z, fluid_fraction_array);
	float ff010 = getFluidFraction(pos.x, pos.y+1, pos.z, fluid_fraction_array);
	float ff011 = getFluidFraction(pos.x+1, pos.y+1, pos.z, fluid_fraction_array);

	float ff100 = getFluidFraction(pos.x, pos.y, pos.z+1, fluid_fraction_array);
	float ff101 = getFluidFraction(pos.x+1, pos.y, pos.z+1, fluid_fraction_array);
	float ff110 = getFluidFraction(pos.x, pos.y+1, pos.z+1, fluid_fraction_array);
	float ff111 = getFluidFraction(pos.x+1, pos.y+1, pos.z+1, fluid_fraction_array);

	int c_bit = (int)(ff000 > ISO_VALUE);
	int x_bit = (int)(ff001 > ISO_VALUE);
	int y_bit = (int)(ff010 > ISO_VALUE);
	int z_bit = (int)(ff100 > ISO_VALUE);

	int mc_index =	((int)(ff000 > ISO_VALUE) << 0)	|
					((int)(ff001 > ISO_VALUE) << 1)	|
					((int)(ff011 > ISO_VALUE) << 2)	|
					((int)(ff010 > ISO_VALUE) << 3)	|
					((int)(ff100 > ISO_VALUE) << 4)	|
					((int)(ff101 > ISO_VALUE) << 5)	|
					((int)(ff111 > ISO_VALUE) << 6)	|
					((int)(ff110 > ISO_VALUE) << 7);

//	mc_index -= (int)(mc_index == 255)*255;
	write_imagef(mc_index_array, texture_pos, (float4)((float)mc_index, 0, 0, 0));

	if (mc_index == 0 || mc_index == 255)
		return;


	texture_pos.x *= 3;
	int normal4_pos_y = texture_pos.y*4;


	/**
	 * we compute all vertex positions because we need those for normals
	 */
#define interp(start, end)	((ISO_VALUE - start) / (end - start))

	float ffd;
	float vd, ivd;	// vertex displacement
	float4 vertex, normal;
	normal.w = 0;

	int4 neg_pos;
	neg_pos.x = max(pos.x - 1, 0);
	neg_pos.y = max(pos.y - 1, 0);
	neg_pos.z = max(pos.z - 1, 0);

#define ADVANCED_INTERPOLATION	1

#if ADVANCED_INTERPOLATION
	int4 add2_pos;
	add2_pos.x = min(pos.x + 2, DOMAIN_CELLS_X-1);
	add2_pos.y = min(pos.y + 2, DOMAIN_CELLS_Y-1);
	add2_pos.z = min(pos.z + 2, DOMAIN_CELLS_Z-1);

	float ff002 = getFluidFraction(add2_pos.x, pos.y, pos.z, fluid_fraction_array);
	float ff020 = getFluidFraction(pos.x, add2_pos.y, pos.z, fluid_fraction_array);
	float ff200 = getFluidFraction(pos.x, pos.y, add2_pos.z, fluid_fraction_array);
#endif

	float ff0_0 = getFluidFraction(pos.x, neg_pos.y, pos.z, fluid_fraction_array);
	float ff0_1 = getFluidFraction(pos.x+1, neg_pos.y, pos.z, fluid_fraction_array);

	float ff_00 = getFluidFraction(pos.x, pos.y, neg_pos.z, fluid_fraction_array);
	float ff_01 = getFluidFraction(pos.x+1, pos.y, neg_pos.z, fluid_fraction_array);

	float ff00_ = getFluidFraction(neg_pos.x, pos.y, pos.z, fluid_fraction_array);
	float ff01_ = getFluidFraction(neg_pos.x, pos.y+1, pos.z, fluid_fraction_array);

	float ff10_ = getFluidFraction(neg_pos.x, pos.y, pos.z+1, fluid_fraction_array);
	float ff1_0 = getFluidFraction(pos.x, neg_pos.y, pos.z+1, fluid_fraction_array);
	float ff_10 = getFluidFraction(pos.x, pos.y+1, neg_pos.z, fluid_fraction_array);

	/**
	 * X interpolated MC vertex
	 */
	vd = interp(ff000, ff001);
	vertex = (float4)(		fpos.x+vd,	fpos.y,		fpos.z, 1.0);
	ivd = 1.0 - vd;

#if ADVANCED_INTERPOLATION
	normal.x = (ff000-ff002)*vd + (ff00_-ff001)*ivd;
#else
	normal.x = 2.0*(ff000-ff001);
#endif

	normal.y = 	(vd*ff0_1 + ivd*ff0_0) -
				(vd*ff011 + ivd*ff010);

	normal.z = 	(vd*ff_01 + ivd*ff_00) -
				(vd*ff101 + ivd*ff100);

	write_imagef(vertex_array, texture_pos, vertex);
	write_imagef(normal_array, texture_pos, normalize(normal));
	texture_pos.x += 1;

	/**
	 * Y interpolated MC vertex
	 */
	vd = interp(ff000, ff010);
	vertex = (float4)(		fpos.x,		fpos.y+vd,	fpos.z, 1.0);
	ivd = 1.0 - vd;

	normal.x = 	(vd*ff01_ + ivd*ff00_) -
				(vd*ff011 + ivd*ff001);

#if ADVANCED_INTERPOLATION
	normal.y = (ff000-ff020)*vd + (ff0_0-ff010)*ivd;
#else
	normal.y = 2.0*(ff000 - ff010);
#endif

	normal.z = 	(vd*ff_10 + ivd*ff_00) -
				(vd*ff110 + ivd*ff100);

	write_imagef(vertex_array, texture_pos, vertex);
	write_imagef(normal_array, texture_pos, normalize(normal));
	texture_pos.x += 1;

	/**
	 * Z interpolated MC vertex
	 */

	vd = interp(ff000, ff100);
	vertex = (float4)(		fpos.x,		fpos.y,		fpos.z+vd, 1.0);
	ivd = 1.0 - vd;

	normal.x = 	(vd*ff10_ + ivd*ff00_) -
				(vd*ff101 + ivd*ff001);

	normal.y = 	(vd*ff1_0 + ivd*ff0_0) -
				(vd*ff110 + ivd*ff010);

#if ADVANCED_INTERPOLATION
	normal.z = (ff000-ff200)*vd + (ff_00-ff100)*ivd;
#else
	normal.z = 2.0*(ff000 - ff100);
#endif

	write_imagef(vertex_array, texture_pos, vertex);
	write_imagef(normal_array, texture_pos, normalize(normal));
}
