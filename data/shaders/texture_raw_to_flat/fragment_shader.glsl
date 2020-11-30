// use opengl 3.2 core shaders


out vec4 frag_data;
uniform sampler2D fluid_fraction_raw_texture;

float getRawValue(int x, int y, int z)
{
	// compute linear access value
	int l = x + y*DOMAIN_CELLS_X + z*(DOMAIN_CELLS_X*DOMAIN_CELLS_Y);

	// 4 values are stored in one RGBA color
	int rgba_l = l >> 2;

	ivec2 tex_coord;
	tex_coord.x = rgba_l % TEXTURE_WIDTH;
	tex_coord.y = rgba_l / TEXTURE_WIDTH;

	return texelFetch(fluid_fraction_raw_texture, tex_coord, 0)[l&3];
}



void main(void)
{
	// 2d position in flat texture (x and y coordinates are expanded)
	ivec2 flat_pos = ivec2(floor(gl_FragCoord.xy));

	// expand, because we store 2x2x1 pieces in RGBA components
	flat_pos *= 2;

	// compute real 3d position
	ivec3 pos;
	pos.z = flat_pos.x / DOMAIN_CELLS_X + FT_Z_MOD*(flat_pos.y / DOMAIN_CELLS_Y);
	if (pos.z >= DOMAIN_CELLS_Z)
	{
		frag_data = vec4(0);
		return;
	}
	pos.x = flat_pos.x % DOMAIN_CELLS_X;
	pos.y = flat_pos.y % DOMAIN_CELLS_Y;

/*
 * the first one is the faster version
 */
#if 0
	int l;			// linear access
	int rgba_l;		// linear access of rgba compressed raw data
	vec4 raw_color;	// color from raw texture

	/**
	 * read the components (x+0,y+0) and (x+1,y+0)
	 */
	// compute linear access value
	l = pos.x + pos.y*DOMAIN_CELLS_X + pos.z*(DOMAIN_CELLS_X*DOMAIN_CELLS_Y);

	// 4 values are stored in one RGBA color
	rgba_l = l >> 2;

	ivec2 tex_coord;
	tex_coord.x = rgba_l % TEXTURE_WIDTH;
	tex_coord.y = rgba_l / TEXTURE_WIDTH;

	raw_color = texelFetch(fluid_fraction_raw_texture, ivec2(rgba_l % TEXTURE_WIDTH, rgba_l / TEXTURE_WIDTH), 0);

	frag_data.x = raw_color[l&3];
	frag_data.y = raw_color[(l+1)&3];

	/**
	 * read the components (x+0,y+1) and (x+1,y+1)
	 */
	// compute linear access value
	l += DOMAIN_CELLS_X;

	// 4 values are stored in one RGBA color
	rgba_l = l >> 2;

	raw_color = texelFetch(fluid_fraction_raw_texture, ivec2(rgba_l % TEXTURE_WIDTH, rgba_l / TEXTURE_WIDTH), 0);

	frag_data.z = raw_color[l&3];
	frag_data.w = raw_color[(l+1)&3];

#else
	frag_data = vec4(	getRawValue(pos.x, pos.y, pos.z),
						getRawValue(pos.x+1, pos.y, pos.z),
						getRawValue(pos.x, pos.y+1, pos.z),
						getRawValue(pos.x+1, pos.y+1, pos.z)
			);
#endif
}
