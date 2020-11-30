const sampler_t tv_sampler = CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_NONE | CLK_FILTER_NEAREST;


int2 getFlatTextureCoord(int pos_x, int pos_y, int pos_z)
{
	return (int2)(	pos_x + (pos_z % FT_Z_MOD)*DOMAIN_CELLS_X,
					pos_y + (pos_z / FT_Z_MOD)*DOMAIN_CELLS_X);
}


__kernel void main(
		read_only image2d_t normal4_array,
		write_only image2d_t normal_array
		)
{
	const size_t gid = get_global_id(0);

	if (gid >= DOMAIN_CELLS)
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

	if (	pos.x >= DOMAIN_CELLS_X-1	||
			pos.y >= DOMAIN_CELLS_Y-1	||
			pos.z >= DOMAIN_CELLS_Z-1
			)
	{
			return;
	}

	texture_pos.x *= 3;

	int2 texture4_pos;
	texture4_pos.x = texture_pos.x;
	texture4_pos.y = texture_pos.y*4;

	int2 new_pos, new_pos2;

	// (0,0,0)
	float4 normalx = read_imagef(normal4_array, tv_sampler, texture4_pos);	// 0
	float4 normaly = read_imagef(normal4_array, tv_sampler, (int2)(texture4_pos.x+1, texture4_pos.y));	// 3
	float4 normalz = read_imagef(normal4_array, tv_sampler, (int2)(texture4_pos.x+2, texture4_pos.y));	// 8

	// (-1,0,0)
	new_pos.x = max(texture4_pos.x-3, 0);
	new_pos.y = texture4_pos.y;
	normaly += read_imagef(normal4_array, tv_sampler, (int2)(new_pos.x+1, new_pos.y+1));	// 1
	normalz += read_imagef(normal4_array, tv_sampler, (int2)(new_pos.x+2, new_pos.y+1));	// 9

	// (0,-1,0)
	new_pos.x = texture4_pos.x;
	new_pos.y = max(texture4_pos.y-4, 0);
	normalx += read_imagef(normal4_array, tv_sampler, (int2)(new_pos.x, new_pos.y+1));		// 2
	normalz += read_imagef(normal4_array, tv_sampler, (int2)(new_pos.x+2, new_pos.y+2));	// 11

	// (0,0,-1)
	new_pos.x = texture4_pos.x-DOMAIN_CELLS_X*3;
	new_pos.y = texture4_pos.y;
	if (new_pos.x < 0)
	{
		new_pos.x += TEXTURE_WIDTH*3;
		new_pos.y = max(new_pos.y-DOMAIN_CELLS_Y*4, 0);
	}
	normalx += read_imagef(normal4_array, tv_sampler, (int2)(new_pos.x, new_pos.y+2));		// 4
	normaly += read_imagef(normal4_array, tv_sampler, (int2)(new_pos.x+1, new_pos.y+2));	// 7

/*
	normalx = (float4)(0,0,0,0);
	normaly = (float4)(0,0,0,0);
	normalz = (float4)(0,0,0,0);
*/
	// (0,-1,-1)
	new_pos2.x = new_pos.x;
	new_pos2.y = max(new_pos.y-4, 0);
	normalx += read_imagef(normal4_array, tv_sampler, (int2)(new_pos2.x, new_pos2.y+3));	// 6

	// (-1,0,-1)
	new_pos2.x = max(new_pos.x-3, 0);
	new_pos2.y = new_pos.y;
	normaly += read_imagef(normal4_array, tv_sampler, (int2)(new_pos2.x+1, new_pos2.y+3));	// 5

	// (-1,-1,0)
	new_pos.x = max(texture4_pos.x-3, 0);
	new_pos.y = max(texture4_pos.y-4, 0);
	normalz += read_imagef(normal4_array, tv_sampler, (int2)(new_pos.x+2, new_pos.y+3));	// 10

	// store normals
	write_imagef(normal_array, texture_pos, normalize(normalx));
	texture_pos.x += 1;
	write_imagef(normal_array, texture_pos, normalize(normaly));
	texture_pos.x += 1;
	write_imagef(normal_array, texture_pos, normalize(normalz));
}
