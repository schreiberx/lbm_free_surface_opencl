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
		read_only image2d_t triangle_vertices_array,
		write_only image2d_t vertex_array,
		write_only image2d_t normal4_array
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


	/**
	 * we compute all vertex positions because we need those for normals
	 */
#define interp(start, end)	((ISO_VALUE - start) / (end - start))

	float ffd;
	float vd;	// vertex displacement
	float4 vertices[12];

	vertices[0] = (float4)(		fpos.x+interp(ff000, ff001),	fpos.y,							fpos.z, 1.0);
	vertices[1] = (float4)(		fpos.x+1.0,						fpos.y+interp(ff001, ff011),	fpos.z, 1.0);
	vertices[2] = (float4)(		fpos.x+interp(ff010, ff011),	fpos.y+1.0,						fpos.z, 1.0);
	vertices[3] = (float4)(		fpos.x,							fpos.y+interp(ff000, ff010),	fpos.z, 1.0);

	vertices[4] = (float4)(		fpos.x+interp(ff100, ff101),	fpos.y,							fpos.z+1.0, 1.0);
	vertices[5] = (float4)(		fpos.x+1.0,						fpos.y+interp(ff101, ff111),	fpos.z+1.0, 1.0);
	vertices[6] = (float4)(		fpos.x+interp(ff110, ff111),	fpos.y+1.0,						fpos.z+1.0, 1.0);
	vertices[7] = (float4)(		fpos.x,							fpos.y+interp(ff100, ff110),	fpos.z+1.0, 1.0);

	vertices[8] = (float4)(		fpos.x,						fpos.y,							fpos.z+interp(ff000, ff100), 1.0);
	vertices[9] = (float4)(		fpos.x+1.0,					fpos.y,							fpos.z+interp(ff001, ff101), 1.0);
	vertices[10] = (float4)(	fpos.x+1.0,					fpos.y+1.0,						fpos.z+interp(ff011, ff111), 1.0);
	vertices[11] = (float4)(	fpos.x,						fpos.y+1.0,						fpos.z+interp(ff010, ff110), 1.0);

	float4 normals[12];
	for (int i = 0; i < 12; i++)
		normals[i] = (float4)(0,0,0,0);

	int count = (int)(read_imagef(triangle_vertices_array, tv_sampler, (int2)(0, mc_index)).x);
	for (int i = 1; i < count;)
	{
		int v0 = (int)(read_imagef(triangle_vertices_array, tv_sampler, (int2)(i, mc_index)).x);	i++;
		int v1 = (int)(read_imagef(triangle_vertices_array, tv_sampler, (int2)(i, mc_index)).x);	i++;
		int v2 = (int)(read_imagef(triangle_vertices_array, tv_sampler, (int2)(i, mc_index)).x);	i++;

		float4 normal = -cross((vertices[v0] - vertices[v1]), (vertices[v2] - vertices[v1]));
		normal = normalize((float4)(normal.x, normal.y, normal.z,0));

		normals[v0] += normal;
		normals[v1] += normal;
		normals[v2] += normal;
	}
	// dont normalize normals cz. they are aggregated and normalized after aggregation

	texture_pos.x *= 3;
	int normal4_pos_y = texture_pos.y*4;

	write_imagef(vertex_array, texture_pos, vertices[0]);
	write_imagef(normal4_array, (int2)(texture_pos.x, normal4_pos_y+0), normals[0]);
	write_imagef(normal4_array, (int2)(texture_pos.x, normal4_pos_y+1), normals[2]);
	write_imagef(normal4_array, (int2)(texture_pos.x, normal4_pos_y+2), normals[4]);
	write_imagef(normal4_array, (int2)(texture_pos.x, normal4_pos_y+3), normals[6]);
	texture_pos.x += 1;

	write_imagef(vertex_array, texture_pos, vertices[3]);
	write_imagef(normal4_array, (int2)(texture_pos.x, normal4_pos_y+0), normals[3]);
	write_imagef(normal4_array, (int2)(texture_pos.x, normal4_pos_y+1), normals[1]);
	write_imagef(normal4_array, (int2)(texture_pos.x, normal4_pos_y+2), normals[7]);
	write_imagef(normal4_array, (int2)(texture_pos.x, normal4_pos_y+3), normals[5]);
	texture_pos.x += 1;

	write_imagef(vertex_array, texture_pos, vertices[8]);
	write_imagef(normal4_array, (int2)(texture_pos.x, normal4_pos_y+0), normals[8]);
	write_imagef(normal4_array, (int2)(texture_pos.x, normal4_pos_y+1), normals[9]);
	write_imagef(normal4_array, (int2)(texture_pos.x, normal4_pos_y+2), normals[11]);
	write_imagef(normal4_array, (int2)(texture_pos.x, normal4_pos_y+3), normals[10]);
}
