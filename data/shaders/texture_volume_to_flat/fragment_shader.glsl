// use opengl 3.2 core shaders


out vec4 frag_data;
uniform sampler3D fluid_fraction_volume_texture;


float getVolumeValue(int x, int y, int z)
{
	return texelFetch(fluid_fraction_volume_texture, ivec3(x,y,z), 0).r;
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

	frag_data = vec4(	getVolumeValue(pos.x, pos.y, pos.z),
						getVolumeValue(pos.x+1, pos.y, pos.z),
						getVolumeValue(pos.x, pos.y+1, pos.z),
						getVolumeValue(pos.x+1, pos.y+1, pos.z)
			);
}
