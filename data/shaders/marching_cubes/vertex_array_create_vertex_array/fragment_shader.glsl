// use opengl 3.2 core shaders


flat in int index;
out ivec2 vertex_texture_coord;

uniform int layers;
uniform usampler2D histoPyramid;
uniform isampler2D mc_indices_texture;
uniform isampler2D triangle_vertices_texture;

void main(void)
{
	ivec2 flat_pos = ivec2(gl_FragCoord.xy);
	uint id = uint(flat_pos.y * VERTEX_ARRAY_TEXTURE_WIDTH + flat_pos.x);

	/*
	 * after the following loop, flat_pos contains the position in the flat texture of the volume
	 */
	flat_pos = ivec2(0,0);

	// we start with the 2nd layer from above (2x2 mipmap texture)
	uint value;

#if TEXTURE_WIDTH == TEXTURE_HEIGHT
	/*
	 * this version works only for equal width and height of textures
	 */
	for (int i = layers-2; i >= 0; i--)
	{
		// go down one layer in the pyramid
		flat_pos <<= 1;

		flat_pos.x += 1;
		flat_pos.y += 1;
		value = texelFetch(histoPyramid, flat_pos, i).r;	// if texelFetch is out of texture, NULL is returned
		if (id >= value)	continue;

		flat_pos.x -= 1;
		value = texelFetch(histoPyramid, flat_pos, i).r;
		if (id >= value)	continue;

		flat_pos.x += 1;
		flat_pos.y -= 1;
		value = texelFetch(histoPyramid, flat_pos, i).r;
		if (id >= value)	continue;

		flat_pos.x -= 1;
		value = texelFetch(histoPyramid, flat_pos, i).r;
	}

#else
	for (int i = layers-2; i >= 0; i--)
	{
		ivec2 tex_size = textureSize(histoPyramid, i);

		// go down one layer in the pyramid
		flat_pos <<= 1;

		value = texelFetch(histoPyramid, ivec2(flat_pos.x+1, flat_pos.y), i).r;
		if (id < value)
		{
			value = texelFetch(histoPyramid, flat_pos, i).r;
			continue;
		}

		value = texelFetch(histoPyramid, ivec2(flat_pos.x, flat_pos.y+1), i).r;
		if (id < value)
		{
			flat_pos.x += int(tex_size.r > 1);
			value = texelFetch(histoPyramid, flat_pos, i).r;
			continue;
		}


		value = texelFetch(histoPyramid, ivec2(flat_pos.x+1, flat_pos.y+1), i).r;
		if (id < value)
		{
			flat_pos.y++;
			value = texelFetch(histoPyramid, flat_pos, i).r;
			continue;
		}

		/**!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
		 *
		 * in the case that the texture width and height are not equal, a NULL would be read until the
		 * current mipmap texture width or height exceeds 1. to fix this, the following code is used.
		 *
		 * if this would not be fixed, e.g. the next realative read position would be (+1,+1) instead of (+1,+0)
		 *
		 **!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!*/
		flat_pos.x += int(tex_size.r > 1);
		flat_pos.y += int(tex_size.g > 1);

		value = texelFetch(histoPyramid, flat_pos, i).r;
	}
#endif


	// compute 3d position
	ivec3 pos;
	pos.x = flat_pos.x % DOMAIN_CELLS_X;
	pos.y = flat_pos.y % DOMAIN_CELLS_Y;
	pos.z = flat_pos.x / DOMAIN_CELLS_X + FT_Z_MOD*(flat_pos.y / DOMAIN_CELLS_Y);


//	vertex_texture_coord.x = id;
//	vertex_texture_coord.y = 0;
//	vertex_texture_coord = ivec2(pos.x, pos.y);
//	return;

	int mc_index = texelFetch(mc_indices_texture, flat_pos, 0).r;

	// relative vertex id for current cube triangle vertices:
	int vertex_id = texelFetch(triangle_vertices_texture, ivec2(int(id-value)+1, mc_index), 0).r;

	if (vertex_id >= 4 && vertex_id < 8)
	{
		flat_pos.x += DOMAIN_CELLS_X;

		int tmp = int(flat_pos.x >= TEXTURE_WIDTH);
		flat_pos.y += DOMAIN_CELLS_Y*tmp;
		flat_pos.x -= TEXTURE_WIDTH*tmp;
	}

#if 0
	int displacements_x[] = int[](0, 3+1, 0, 1, 0, 3+1, 0, 1, 2, 3+2, 3+2, 2);
	int displacements_y[] = int[](0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 1);
	vertex_texture_coord.x = flat_pos.x*3+displacements_x[vertex_id];
	vertex_texture_coord.y = flat_pos.y+displacements_y[vertex_id];
#else
	vertex_texture_coord.x = flat_pos.x*3+((0xBE4C4C>>(vertex_id*2))&3) + ((0x280404>>(vertex_id*2))&3);
	vertex_texture_coord.y = flat_pos.y+((0xc44>>vertex_id)&1);
#endif
}
