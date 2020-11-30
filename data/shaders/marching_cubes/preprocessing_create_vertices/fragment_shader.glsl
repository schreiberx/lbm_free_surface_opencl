// use opengl 3.2 core shaders


out vec4 vertex_pos;
out vec4 vertex_normal;

uniform sampler3D fluid_fraction_texture;
uniform isampler2D mc_indices_texture;



float getFluidFractionInterpolated(float x, float y, float z)
{
	return texture(fluid_fraction_texture, vec3(x,y,z)).r;
}

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
	int axis = flat_pos.x % 3;
	flat_pos.x /= 3;

	ivec3 pos;
	pos.x = flat_pos.x % DOMAIN_CELLS_X;
	pos.y = flat_pos.y % DOMAIN_CELLS_Y;
	pos.z = flat_pos.x / DOMAIN_CELLS_X + FT_Z_MOD*(flat_pos.y / DOMAIN_CELLS_Y);

	int mc_index = 	texelFetch(mc_indices_texture, flat_pos, 0).r;

	if (mc_index == 0 || mc_index == 255)	discard;

	vec3 fpos = vec3(pos);

#define interp(start, end)	((ISO_VALUE - start) / (end - start))

	float ff000 = getFluidFraction(pos.x, pos.y, pos.z);

	switch(axis)
	{
		case 0:
			fpos.x += interp(ff000, getFluidFraction(pos.x+1, pos.y, pos.z));
			break;

		case 1:
			fpos.y += interp(ff000, getFluidFraction(pos.x, pos.y+1, pos.z));
			break;

		case 2:
			fpos.z += interp(ff000, getFluidFraction(pos.x, pos.y, pos.z+1));
			break;
	}

/**
 * 2 different implementations because it seems that GLSL uses a different
 * interpolation for 3D textures
 */
#if 1
	/**
	 * use GLSL interpolated values (they seem to be not really accurate, but who cares...)
	 */
	vertex_pos = vec4(fpos, 1.0);

	fpos =	fpos*vec3(INV_DOMAIN_CELLS_X, INV_DOMAIN_CELLS_Y, INV_DOMAIN_CELLS_Z) +
			vec3(INV_05_DOMAIN_CELLS_X, INV_05_DOMAIN_CELLS_Y, INV_05_DOMAIN_CELLS_Z);

	vec3 normal;
	normal.x =	getFluidFractionInterpolated(fpos.x-float(INV_DOMAIN_CELLS_X), fpos.y, fpos.z) -
				getFluidFractionInterpolated(fpos.x+float(INV_DOMAIN_CELLS_X), fpos.y, fpos.z);

	normal.y =	getFluidFractionInterpolated(fpos.x, fpos.y-float(INV_DOMAIN_CELLS_Y), fpos.z) -
				getFluidFractionInterpolated(fpos.x, fpos.y+float(INV_DOMAIN_CELLS_Y), fpos.z);

	normal.z =	getFluidFractionInterpolated(fpos.x, fpos.y, fpos.z-float(INV_DOMAIN_CELLS_Z)) -
				getFluidFractionInterpolated(fpos.x, fpos.y, fpos.z+float(INV_DOMAIN_CELLS_Z));

	vertex_normal = vec4(normalize(normal.xyz), 0);

#else
	/**
	 * interpolation using texels
	 */
	float ff001 = getFluidFraction(pos.x+1, pos.y, pos.z);
	float ff010 = getFluidFraction(pos.x, pos.y+1, pos.z);
	float ff100 = getFluidFraction(pos.x, pos.y, pos.z+1);
	float ff011 = getFluidFraction(pos.x+1, pos.y+1, pos.z);
	float ff101 = getFluidFraction(pos.x+1, pos.y, pos.z+1);
	float ff110 = getFluidFraction(pos.x, pos.y+1, pos.z+1);

	ivec3 neg_pos;
	neg_pos.x = max(pos.x - 1, 0);
	neg_pos.y = max(pos.y - 1, 0);
	neg_pos.z = max(pos.z - 1, 0);

	ivec3 add2_pos;
	add2_pos.x = min(pos.x + 2, DOMAIN_CELLS_X-1);
	add2_pos.y = min(pos.y + 2, DOMAIN_CELLS_Y-1);
	add2_pos.z = min(pos.z + 2, DOMAIN_CELLS_Z-1);

	float ff002 = getFluidFraction(add2_pos.x, pos.y, pos.z);
	float ff020 = getFluidFraction(pos.x, add2_pos.y, pos.z);
	float ff200 = getFluidFraction(pos.x, pos.y, add2_pos.z);

	float ff0_0 = getFluidFraction(pos.x, neg_pos.y, pos.z);
	float ff0_1 = getFluidFraction(pos.x+1, neg_pos.y, pos.z);

	float ff_00 = getFluidFraction(pos.x, pos.y, neg_pos.z);
	float ff_01 = getFluidFraction(pos.x+1, pos.y, neg_pos.z);

	float ff00_ = getFluidFraction(neg_pos.x, pos.y, pos.z);
	float ff01_ = getFluidFraction(neg_pos.x, pos.y+1, pos.z);

	float ff10_ = getFluidFraction(neg_pos.x, pos.y, pos.z+1);
	float ff1_0 = getFluidFraction(pos.x, neg_pos.y, pos.z+1);
	float ff_10 = getFluidFraction(pos.x, pos.y+1, neg_pos.z);


	vec3 normal;

	float ivd, vd;
	switch(axis)
	{
		case 0:
			/**
			 * X interpolated MC vertex
			 */
			vd = interp(ff000, ff001);
			ivd = 1.0 - vd;

			normal.x = (ff000-ff002)*vd + (ff00_-ff001)*ivd;

			normal.y = 	(vd*ff0_1 + ivd*ff0_0) -
						(vd*ff011 + ivd*ff010);

			normal.z = 	(vd*ff_01 + ivd*ff_00) -
						(vd*ff101 + ivd*ff100);
			break;

		case 1:
			/**
			 * Y interpolated MC vertex
			 */
			vd = interp(ff000, ff010);
			ivd = 1.0 - vd;

			normal.x = 	(vd*ff01_ + ivd*ff00_) -
						(vd*ff011 + ivd*ff001);

			normal.y = (ff000-ff020)*vd + (ff0_0-ff010)*ivd;
			normal.z = 	(vd*ff_10 + ivd*ff_00) -
						(vd*ff110 + ivd*ff100);
			break;

		case 2:
			/**
			 * Z interpolated MC vertex
			 */
			vd = interp(ff000, ff100);
			ivd = 1.0 - vd;

			normal.x = 	(vd*ff10_ + ivd*ff00_) -
						(vd*ff101 + ivd*ff001);

			normal.y = 	(vd*ff1_0 + ivd*ff0_0) -
						(vd*ff110 + ivd*ff010);

			normal.z = (ff000-ff200)*vd + (ff_00-ff100)*ivd;
			break;
	}

	vertex_pos = vec4(fpos, 1.0);
	vertex_normal = vec4(normalize(normal.xyz), 0);
#endif
}
