// use opengl 3.2 core version!


in uint linear_id;


out vec4 out_normal;

uniform sampler2D fluid_fraction_texture;
uniform usampler2D histo_pyramid_texture;
uniform usampler2D mc_indices_texture;
uniform isampler2D triangle_vertices_texture;


// matrices to transform the vertices and normals to a [-1;1]^3 cube
uniform mat4 vertex_fix_matrix4;
uniform mat3 normal_fix_matrix3;

float getFluidFraction(int x, int y, int z)
{
	ivec2 flat_pos;
	flat_pos.x = (z % FT_Z_MOD)*DOMAIN_CELLS_X + x;
	flat_pos.y = (z / FT_Z_MOD)*DOMAIN_CELLS_Y + y;

	return texelFetch(fluid_fraction_texture, flat_pos >> 1, 0)[(flat_pos.y&1)*2 + (flat_pos.x&1)];
}

void main(void)
{
	/*
	 * after the following loop, flat_pos contains the position in the flat texture of the volume
	 * together with 'value', the shader is able to compute the vertex
	 */
	ivec2 flat_pos = ivec2(0,0);

	// we start with the 2nd layer from above (2x2 mipmap texture)
	uint value;
	uvec4 data;

	for (int i = LAYERS-1; i >= 1; i--)
	{
		data = texelFetch(histo_pyramid_texture, flat_pos, i);

		/*
		 * go down one layer in the pyramid
		 * this does not modify the (0,0) texture coordinate for the topmost layer!!!
		 */
		flat_pos <<= 1;
#if 1
		flat_pos.y += int(linear_id >= data.b);
		flat_pos.x += int(linear_id >= data.a || (linear_id < data.b && linear_id >= data.g));
#else
		if (linear_id >= data.a)
		{
			flat_pos.x += 1;
			flat_pos.y += 1;
			continue;
		}

		if (linear_id >= data.b)
		{
			flat_pos.y += 1;
			continue;
		}

		if (linear_id >= data.g)
		{
			flat_pos.x += 1;
			continue;
		}
#endif
	}

	// read RGBA value from bottom layer
	data = texelFetch(histo_pyramid_texture, flat_pos, 0);
	uint mc_index;
	uvec4 mc_data = texelFetch(mc_indices_texture, flat_pos, 0);

	flat_pos <<= 1;

	/*
	 * final displacement for bottom layer
	 */
#if 1
	flat_pos.y += int(linear_id >= data.b);
	flat_pos.x += int(linear_id >= data.a || (linear_id < data.b && linear_id >= data.g));

#if 0
	bvec4 flags = bvec4(greaterThanEqual(uvec4(linear_id), data));
	flags.r = !flags.g;
	flags.g = flags.g && !flags.b;
	flags.b = flags.b && !flags.a;
	flags.a = flags.a;

	value =		data.r * uint(flags.r)	+
				data.g * uint(flags.g)	+
				data.b * uint(flags.b)	+
				data.a * uint(flags.a);

	mc_index =	mc_data.r * uint(flags.r)	+
				mc_data.g * uint(flags.g)	+
				mc_data.b * uint(flags.b)	+
				mc_data.a * uint(flags.a);

#else
	value	 =	data.a * uint(linear_id >= data.a)					+
				data.b * uint(linear_id >= data.b && linear_id < data.a)	+
				data.g * uint(linear_id >= data.g && linear_id < data.b)	+
				data.r * uint(linear_id < data.g);

	mc_index =	mc_data.a * uint(linear_id >= data.a)					+
				mc_data.b * uint(linear_id >= data.b && linear_id < data.a)	+
				mc_data.g * uint(linear_id >= data.g && linear_id < data.b)	+
				mc_data.r * uint(linear_id < data.g);
#endif

#else
	if (linear_id >= data.a)
	{
		value = data.a;
		mc_index = mc_data.a;
		flat_pos.x += 1;
		flat_pos.y += 1;
	}
	else if (linear_id >= data.b)
	{
		value = data.b;
		mc_index = mc_data.b;
		flat_pos.y += 1;
	}
	else if (linear_id >= data.g)
	{
		value = data.g;
		mc_index = mc_data.g;
		flat_pos.x += 1;
	}
	else
	{
		value = data.r;
		mc_index = mc_data.r;
	}
#endif
	// compute 3d position
	ivec3 pos;
	pos.x = flat_pos.x % DOMAIN_CELLS_X;
	pos.y = flat_pos.y % DOMAIN_CELLS_Y;
	pos.z = flat_pos.x / DOMAIN_CELLS_X + FT_Z_MOD*(flat_pos.y / DOMAIN_CELLS_Y);

	// relative vertex id for current cube triangle vertices
	int vertex_id = texelFetch(triangle_vertices_texture, ivec2(linear_id+1u-value, mc_index), 0).r;
	int interpolation_axis;	// 0 for x, 1 for y, 2 for z

#if 0
	int id = (pos.y & 1)*2 + (pos.x & 1);

	// midpoint
	//										  0    1    2    3    4    5    6    7    8    9   10   11
	const float displacements_x[] = float[](0.5,   1, 0.5,   0, 0.5,   1, 0.5,   0,   0,   1,   1,   0);
	const float displacements_y[] = float[](  0, 0.5,   1, 0.5,   0, 0.5,   1, 0.5,   0,   0, 1.0, 1.0);
	const float displacements_z[] = float[](  0,   0,   0,   0,   1,   1,   1,   1, 0.5, 0.5, 0.5, 0.5);

	gl_Position.x = float(pos.x) + displacements_x[vertex_id];
	gl_Position.y = float(pos.y) + displacements_y[vertex_id];
	gl_Position.z = float(pos.z) + displacements_z[vertex_id];
	gl_Position.w = 1;

#else

	// interpolation
	#define interp(start, end)	((ISO_VALUE - (start)) / ((end) - (start)))
	#define interpv(vertex_pos)	((ISO_VALUE - (vertex_pos.x)) / ((vertex_pos.y) - (vertex_pos.x)))

#if 1
//	const int displ_x[12] = int[](0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 1, 0);
//	const int displ_y[12] = int[](0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 1);
//	const int displ_z[12] = int[](0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0);

	pos.x += (0x622 >> vertex_id) & 1;
	pos.y += (0xc44 >> vertex_id) & 1;
	pos.z += (0xf0 >> vertex_id) & 1;

//	int interpolation_axis_a[12] = int[](0, 1, 0, 1, 0, 1, 0, 1, 2, 2, 2, 2);
//	interpolation_axis = interpolation_axis_a[vertex_id];

	interpolation_axis = (0xAA4444 >> (vertex_id<<1)) & 3;

//	int displ_interpol_x[3] = int[](1, 0, 0);
//	int displ_interpol_y[3] = int[](0, 1, 0);
//	int displ_interpol_z[3] = int[](0, 0, 1);

//	int dx = displ_interpol_x[interpolation_axis];
//	int dy = displ_interpol_y[interpolation_axis];
//	int dz = displ_interpol_z[interpolation_axis];

	ivec3 displ;
	displ.x = int(interpolation_axis == 0);
	displ.y = int(interpolation_axis == 1);
	displ.z = int(interpolation_axis == 2);

	gl_Position.xyz = vec3(pos);
	gl_Position.w = 1.0;

	float displacement = interp(	getFluidFraction(pos.x, pos.y, pos.z),
									getFluidFraction(pos.x+displ.x, pos.y+displ.y, pos.z+displ.z)
								);

	gl_Position.x += float(displ.x)*displacement;
	gl_Position.y += float(displ.y)*displacement;
	gl_Position.z += float(displ.z)*displacement;

	gl_Position = vertex_fix_matrix4*gl_Position;

#elif 1
	int displ_x[12] = int[](0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 1, 0);
	int displ_y[12] = int[](0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 1);
	int displ_z[12] = int[](0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0);

	pos.x += displ_x[vertex_id];
	pos.y += displ_y[vertex_id];
	pos.z += displ_z[vertex_id];

	int interpolation_axis_a[12] = int[](0, 1, 0, 1, 0, 1, 0, 1, 2, 2, 2, 2);
	interpolation_axis = interpolation_axis_a[vertex_id];

	int displ_interpol_x[3] = int[](1, 0, 0);
	int displ_interpol_y[3] = int[](0, 1, 0);
	int displ_interpol_z[3] = int[](0, 0, 1);

	int dx = displ_interpol_x[interpolation_axis];
	int dy = displ_interpol_y[interpolation_axis];
	int dz = displ_interpol_z[interpolation_axis];

	vertex_pos.xyz = vec3(pos);
	vertex_pos.w = 1.0;

	float displacement = interp(	getFluidFraction(pos.x, pos.y, pos.z),
									getFluidFraction(pos.x+dx, pos.y+dy, pos.z+dz)
								);

	vertex_pos.x += float(dx)*displacement;
	vertex_pos.y += float(dy)*displacement;
	vertex_pos.z += float(dz)*displacement;

#else
	vertex_pos = vec4(pos, 1);


	/*
	 * if the index is within [4;8], displace the axis along the z axis
	 * use interpolation_axis as temporary variable
	 */

	interpolation_axis = int((vertex_id >= 4) && (vertex_id <= 7));
	pos.z += interpolation_axis;
	vertex_pos.z += float(interpolation_axis);
	vertex_id -= 4*interpolation_axis;


	switch(vertex_id)
	{
		case 0:			// (0,0,0)-(1,0,0)
			vertex_pos.x += interp(getFluidFraction(pos.x, pos.y, pos.z), getFluidFraction(pos.x+1, pos.y, pos.z));
			interpolation_axis = 0;
			break;

		case 1:			// (1,0,0)-(1,1,0)
			vertex_pos.x++;
			pos.x++;
			vertex_pos.y += interp(getFluidFraction(pos.x, pos.y, pos.z), getFluidFraction(pos.x, pos.y+1, pos.z));
			interpolation_axis = 1;
			break;

		case 2:			// (0,1,0)-(1,1,0)
			pos.y++;
			vertex_pos.x += interp(getFluidFraction(pos.x, pos.y, pos.z), getFluidFraction(pos.x+1, pos.y, pos.z));
			vertex_pos.y++;
			interpolation_axis = 0;
			break;

		case 3:			// (0,0,0)-(0,1,0)
			vertex_pos.y += interp(getFluidFraction(pos.x, pos.y, pos.z), getFluidFraction(pos.x, pos.y+1, pos.z));
			interpolation_axis = 1;
			break;

	//////////////////////////////////////////

		case 8:			// (0,0,0)-(0,0,1)
			vertex_pos.z += interp(getFluidFraction(pos.x, pos.y, pos.z), getFluidFraction(pos.x, pos.y, pos.z+1));
			interpolation_axis = 2;
			break;

		case 9:			// (1,0,0)-(1,0,1)
			vertex_pos.x++;
			pos.x++;
			vertex_pos.z += interp(getFluidFraction(pos.x, pos.y, pos.z), getFluidFraction(pos.x, pos.y, pos.z+1));
			interpolation_axis = 2;
			break;

		case 10:			// (1,1,0)-(1,1,1)
			vertex_pos.x++;
			vertex_pos.y++;
			pos.x++;
			pos.y++;
			vertex_pos.z += interp(getFluidFraction(pos.x, pos.y, pos.z), getFluidFraction(pos.x, pos.y, pos.z+1));
			interpolation_axis = 2;
			break;

		case 11:			// (0,1,0)-(0,1,1)
			vertex_pos.y++;
			pos.y++;
			vertex_pos.z += interp(getFluidFraction(pos.x, pos.y, pos.z), getFluidFraction(pos.x, pos.y, pos.z+1));
			interpolation_axis = 2;
			break;
	}
#endif
	/*
	 * compute normal
	 */
	/**
	 * interpolation using texels
	 */
	float ff000 = getFluidFraction(pos.x, pos.y, pos.z);
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
	switch(interpolation_axis)
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

	out_normal = vec4(normalize(normal_fix_matrix3*normal), 0);
#endif
}
