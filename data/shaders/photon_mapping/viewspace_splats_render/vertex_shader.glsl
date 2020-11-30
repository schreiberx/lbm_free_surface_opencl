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

	// fetch position in light view space
	// the alpha channel stores the way length of the photon
	pos.xyzw = texelFetch(photon_light_view_pos_texture, vertex_array_texture_pos, 0).rgba;

	point_alpha /= pos.a*pos.a;

#if 0
	// does not improve performance
	if (isinf(pos.x))
	{
		// discard not available => move point out of viewspace
		gl_Position = vec4(0.,0.,1.0/0.0,1);
		return;
	}
#endif

    pos = view_MUL_inv_light_view_matrix*vec4(pos.xyz, 1);
//	pos.xyz /= pos.w;


    gl_Position = projection_matrix*pos;
	gl_PointSize = -splat_size/pos.z;

	/*******************************************************************
	 * normal of surface and angle(surface_normal, photon_dir)
	 *******************************************************************/
	vec4 normal = texelFetch(photon_light_view_dir_texture, vertex_array_texture_pos, 0);
	point_alpha = max(point_alpha*normal.a, 0);

	normal.xyz = normalize(view_normal_MUL_transpose_light_view_matrix3*normalize(normal.xyz));

	point_alpha = max(point_alpha*max(dot(normal, normalize(pos)), 0), 0);
}
