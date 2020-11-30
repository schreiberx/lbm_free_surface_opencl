// use opengl 3.2 core shaders


// MC bit shift displacements (+-z, +-y, +-x)
#define MC000	(0u)
#define MC001	(1u)
#define MC011	(2u)
#define MC010	(3u)
#define MC100	(4u)
#define MC101	(5u)
#define MC111	(6u)
#define MC110	(7u)

out uvec4 mc_index;
out uvec4 vertex_count;

uniform sampler2D fluid_fraction_texture;
uniform isampler2D triangle_vertices_texture;


void main(void)
{
	uvec4 idata;

	// compute 3d position
	ivec2 flat_pos = ivec2(gl_FragCoord.xy);

	ivec3 pos;
	pos.x = flat_pos.x % DOMAIN_CELLS_X;
	pos.y = flat_pos.y % DOMAIN_CELLS_Y;
	pos.z = flat_pos.x / DOMAIN_CELLS_X + FT_Z_MOD*(flat_pos.y / DOMAIN_CELLS_Y);

	// get fluid fractions for this cells
	idata = uvec4(greaterThan(texelFetch(fluid_fraction_texture, ivec2(flat_pos), 0), vec4(ISO_VALUE)));

	/*
	 * MC INDEX:
	 * |--------
	 * | B | A |
	 * |--------
	 * | R | G |
	 * |--------
	 */
	// ( X, Y)
	// (+0,+0)
	mc_index.r =	idata.r				|
					(idata.g << MC001)	|
					(idata.b << MC010)	|
					(idata.a << MC011);

	// (+1,+0)
	mc_index.g =	idata.g				|
					idata.a << MC010;

	// (+0,+1)
	mc_index.b =	idata.b				|
					idata.a << MC001;

	// (+1,+1)
	mc_index.a =	idata.a;

	// get fluid fractions neighbors for the 2 right cells
	idata.rg = uvec4(greaterThan(texelFetch(fluid_fraction_texture, ivec2(flat_pos.x+1, flat_pos.y), 0), vec4(ISO_VALUE))).rb;
	mc_index.g |=	idata.r << MC001	|
					idata.g << MC011;

	mc_index.a |=	idata.g << MC001;

	// get fluid fractions neighbors for the 2 top cells
	idata.rg = uvec4(greaterThan(texelFetch(fluid_fraction_texture, ivec2(flat_pos.x, flat_pos.y+1), 0), vec4(ISO_VALUE))).rg;
	mc_index.b |=	idata.r << MC010	|
					idata.g << MC011;

	mc_index.a |=	idata.g << MC010;


	// get fluid fractions neighbors for the 1 right-top cells
	idata.r = uvec4(greaterThan(texelFetch(fluid_fraction_texture, ivec2(flat_pos.x+1, flat_pos.y+1), 0), vec4(ISO_VALUE))).r;
	mc_index.a |=	idata.r << MC011;

	/*
	 * Z+1 LAYER
	 */
	flat_pos.x += (DOMAIN_CELLS_X/2);
	flat_pos.y += (DOMAIN_CELLS_Y/2)*int(flat_pos.x >= TEXTURE_WIDTH);
	flat_pos.x -= (TEXTURE_WIDTH)*int(flat_pos.x >= TEXTURE_WIDTH);

	idata = uvec4(greaterThan(texelFetch(fluid_fraction_texture, ivec2(flat_pos), 0), vec4(ISO_VALUE)));

	// (+0,+0)
	mc_index.r |= 	idata.r << MC100	|
					idata.g << MC101	|
					idata.b << MC110	|
					idata.a << MC111;

	// (+1,+0)
	mc_index.g |=	idata.g << MC100	|
					idata.a << MC110;

	// (+0,+1)
	mc_index.b |=	idata.b << MC100	|
					idata.a << MC101;

	// (+1,+1)
	mc_index.a |=	idata.a << MC100;

	// get fluid fractions neighbors for the 2 right cells
	idata.rg = uvec4(greaterThan(texelFetch(fluid_fraction_texture, ivec2(flat_pos.x+1, flat_pos.y), 0), vec4(ISO_VALUE))).rb;
	mc_index.g |=	idata.r << MC101	|
					idata.g << MC111;

	mc_index.a |=	idata.g << MC101;

	// get fluid fractions neighbors for the 2 top cells
	idata.rg = uvec4(greaterThan(texelFetch(fluid_fraction_texture, ivec2(flat_pos.x, flat_pos.y+1), 0), vec4(ISO_VALUE))).rg;
	mc_index.b |=	idata.r << MC110	|
					idata.g << MC111;

	mc_index.a |=	idata.g << MC110;


	// get fluid fractions neighbors for the 1 right-top cells
	idata.r = uvec4(greaterThan(texelFetch(fluid_fraction_texture, ivec2(flat_pos.x+1, flat_pos.y+1), 0), vec4(ISO_VALUE))).r;
	mc_index.a |=	idata.r << MC111;

	vertex_count.r = uint(texelFetch(triangle_vertices_texture, ivec2(0, mc_index.r), 0).r-1);
	vertex_count.g = uint(texelFetch(triangle_vertices_texture, ivec2(0, mc_index.g), 0).r-1);
	vertex_count.b = uint(texelFetch(triangle_vertices_texture, ivec2(0, mc_index.b), 0).r-1);
	vertex_count.a = uint(texelFetch(triangle_vertices_texture, ivec2(0, mc_index.a), 0).r-1);
}
