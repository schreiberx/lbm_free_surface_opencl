// use opengl 3.2 core shaders


out uint mc_index;
out uint vertex_count;

uniform sampler3D fluid_fraction_texture;
uniform isampler2D triangle_vertices_texture;


void main(void)
{
	/*
	 * compute 3d position
	 */
	ivec2 flat_pos = ivec2(gl_FragCoord.xy);

	ivec3 pos;
	pos.x = flat_pos.x % DOMAIN_CELLS_X;
	pos.y = flat_pos.y % DOMAIN_CELLS_Y;
	pos.z = flat_pos.x / DOMAIN_CELLS_X + FT_Z_MOD*(flat_pos.y / DOMAIN_CELLS_Y);

#if 1
	// texelFetch is also cached
	int index =	(int(texelFetch(fluid_fraction_texture, ivec3(pos.x, pos.y, pos.z), 0).r > ISO_VALUE))			|
				(int(texelFetch(fluid_fraction_texture, ivec3(pos.x+1, pos.y, pos.z), 0).r > ISO_VALUE) << 1)	|
				(int(texelFetch(fluid_fraction_texture, ivec3(pos.x+1, pos.y+1, pos.z), 0).r > ISO_VALUE) << 2)	|
				(int(texelFetch(fluid_fraction_texture, ivec3(pos.x, pos.y+1, pos.z), 0).r > ISO_VALUE) << 3)	|
				(int(texelFetch(fluid_fraction_texture, ivec3(pos.x, pos.y, pos.z+1), 0).r > ISO_VALUE) << 4)	|
				(int(texelFetch(fluid_fraction_texture, ivec3(pos.x+1, pos.y, pos.z+1), 0).r > ISO_VALUE) << 5)	|
				(int(texelFetch(fluid_fraction_texture, ivec3(pos.x+1, pos.y+1, pos.z+1), 0).r > ISO_VALUE) << 6)	|
				(int(texelFetch(fluid_fraction_texture, ivec3(pos.x, pos.y+1, pos.z+1), 0).r > ISO_VALUE) << 7);
#else
	vec3 i = vec3(INV_DOMAIN_CELLS_X, INV_DOMAIN_CELLS_X, INV_DOMAIN_CELLS_Z);
	vec3 h = vec3(0.5, 0.5, 0.5);
	int index =	(int(texture(fluid_fraction_texture, (vec3(pos.x, pos.y, pos.z)+h)*i).r > ISO_VALUE))			|
				(int(texture(fluid_fraction_texture, (vec3(pos.x+1, pos.y, pos.z)+h)*i).r > ISO_VALUE) << 1)	|
				(int(texture(fluid_fraction_texture, (vec3(pos.x+1, pos.y+1, pos.z)+h)*i).r > ISO_VALUE) << 2)	|
				(int(texture(fluid_fraction_texture, (vec3(pos.x, pos.y+1, pos.z)+h)*i).r > ISO_VALUE) << 3)	|
				(int(texture(fluid_fraction_texture, (vec3(pos.x, pos.y, pos.z+1)+h)*i).r > ISO_VALUE) << 4)	|
				(int(texture(fluid_fraction_texture, (vec3(pos.x+1, pos.y, pos.z+1)+h)*i).r > ISO_VALUE) << 5)	|
				(int(texture(fluid_fraction_texture, (vec3(pos.x+1, pos.y+1, pos.z+1)+h)*i).r > ISO_VALUE) << 6)	|
				(int(texture(fluid_fraction_texture, (vec3(pos.x, pos.y+1, pos.z+1)+h)*i).r > ISO_VALUE) << 7);
#endif
	mc_index = uint(index);
	vertex_count = uint(texelFetch(triangle_vertices_texture, ivec2(0, index), 0).r-1);
}
