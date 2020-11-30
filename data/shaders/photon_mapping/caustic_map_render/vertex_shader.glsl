// use opengl 3.2 core version!


in int in_index;

uniform sampler2D photon_light_view_pos_texture;
uniform sampler2D photon_light_view_dir_texture;

uniform mat4 projection_matrix;
uniform mat4 view_MUL_inv_light_view_matrix;
uniform mat3 view_normal_MUL_transpose_light_view_matrix3;

uniform int texture_width;
uniform int texture_height;

out vec3 frag_position3;
out vec3 frag_normal3;

// splat size when rendered with current projection matrix at position (0,0,-1);
uniform float splat_size;

// attenuation of splat
uniform float splat_energy;
out float point_alpha;

//out vec3 asdf;

void main(void)
{
	// TODO: replace those computational intensive operations by bit operations
	ivec2 vertex_array_texture_pos = ivec2(	in_index % texture_width,
											in_index / texture_width);

	point_alpha = splat_energy;

	/*******************************************************************
	 * position of photon
	 *******************************************************************/
	vec4 pos;

	// x,y: position in caustic map texture
	// z: angle(surface_normal, photon_dir)
	// w: length of the way the photon had until the surface was hit
	pos.xyzw = texelFetch(photon_light_view_pos_texture, vertex_array_texture_pos, 0).rgba;

	// TODO: i dont know why to scale by 1/dist^4 - but this seems to work
	point_alpha /= pos.w*pos.w;//*pos.a*pos.a;

	gl_PointSize = splat_size;
	gl_Position = vec4(pos.xy*2.0-1.0, 0.5, 1);

#if 0
	// does not improve performance
	if (isinf(pos.x))
	{
		// discard not available => move point out of viewspace
		gl_Position = vec4(0,0,1.0/0.0,1);
		return;
	}
#endif

}
