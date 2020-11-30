// use opengl 3.2 core shaders


out vec4 frag_data;

uniform sampler3D volume_texture;

uniform mat4 pvm_matrix;
uniform mat4 view_matrix;
uniform mat4 model_matrix;
uniform mat3 model_normal_matrix3;
uniform mat3 view_model_normal_matrix3;
uniform mat4 view_model_matrix;

uniform vec3 texture_size;
uniform vec3 inv_texture_size;
uniform vec3 texture_size_add_one;
uniform vec3 inv_texture_size_add_one;

uniform vec3 gradient_delta;
#define ISO_VALUE	0.5f


vec4 blinnShading(	vec4 material_color,	///< color of material
					vec3 view_normal3,			///< normal of fragment in viewspace
					vec3 view_position3		///< position of fragment in viewspace
);

/**
 * follow a ray until an isosurface is hit
 *
 * input:
 * 	_pos:	start_position
 * 	_dir:	direction
 *
 * output:
 *  _normal:	normal at isosurface
 *  _exit:		true, if ray exists from surface
 */

vec3 _pos;		// position of current ray
vec3 _dir;		// direction of current ray
vec3 _normal;
bool _exit;



void rayCast()
{
	/**
	 * compute distance to intersection with box [0,0,0]x[1,1,1]
	 */

	// increment values (-1, 0 or +1) for axes
	vec3 axes_increments = sign(_dir);

	// setup position and align to the middle of the cell for accurate sampling
	vec3 pos = _pos + axes_increments*vec3(0.5,0.5,0.5);

	// compute collision surface with point
	vec3 collide_surface = vec3(greaterThanEqual(_dir, vec3(0,0,0)))*texture_size;

	// compute distance for every dimension
	vec3 dist = (collide_surface-pos)/_dir;

	// use minmum distance as sampling distance
	float max_dist = min(min(min(dist[0], dist[1]), dist[2]), length(texture_size));

	// inverse direction (frequently used in volume sampling)
	vec3 inv_dir = vec3(1.0f)/(_dir);

	// compute axes for next ray crossing to get positive distances
	vec3 next_axes =	floor(pos) +
						vec3(greaterThanEqual(axes_increments, vec3(0,0,0)));

	float current_step_fac;
	float current_value;
	vec3 current_pos;
	vec3 current_interpolated_texture_pos;

	float prev_value = 0.0;
	vec3 prev_pos = _pos;

	while (current_step_fac < max_dist)
	{
		// compute distances to next axes
		vec3 dist = (next_axes - pos)*inv_dir;

		// this seems to be much faster because it avoids a lot of branching
		float dist0_leq_dist1 = float(dist[0] <= dist[1]);
		float dist1_leq_dist2 = float(dist[1] <= dist[2]);
		float dist0_leq_dist2 = float(dist[0] <= dist[2]);

		// compute 0 and 1 values to select nearest distance
		vec3 mdist;
		mdist.x = dist0_leq_dist1*dist0_leq_dist2;
		mdist.y = dist1_leq_dist2*(1.0f-dist0_leq_dist1);
		mdist.z = (1.0f-dist0_leq_dist2)*(1.0f-dist1_leq_dist2);

		// compute next axes for collision
		next_axes += axes_increments*mdist;

		// set step_fac to next distance value
		current_step_fac = dot(dist, mdist);

		// compute new position of current sampling point
		current_pos = _pos + _dir*current_step_fac;

		// convert to texture coordinates
		current_interpolated_texture_pos = current_pos*inv_texture_size;
		float current_value = texture(volume_texture, current_interpolated_texture_pos).r;

		if (current_value >= ISO_VALUE)
		{
			// interpolate between prev and current value
			_pos = (	prev_pos*(current_value - ISO_VALUE) +
						current_pos*(ISO_VALUE - prev_value)
					)	/ (current_value - prev_value);

			_pos *= inv_texture_size;

#if 0
			current_value = texture(volume_texture, current_interpolated_texture_pos).r;

			_normal = normalize(
				vec3(	current_value - texture(volume_texture, _pos+vec3(+gradient_delta.x,0,0)).r,
						current_value - texture(volume_texture, _pos+vec3(0,+gradient_delta.y,0)).r,
						current_value - texture(volume_texture, _pos+vec3(0,0,+gradient_delta.z)).r
					));
#else
			_normal = normalize(
				vec3(
						texture(volume_texture, _pos+vec3(-gradient_delta.x,0,0)).r - texture(volume_texture, _pos+vec3(+gradient_delta.x,0,0)).r,
						texture(volume_texture, _pos+vec3(0,-gradient_delta.y,0)).r - texture(volume_texture, _pos+vec3(0,+gradient_delta.y,0)).r,
						texture(volume_texture, _pos+vec3(0,0,-gradient_delta.z)).r - texture(volume_texture, _pos+vec3(0,0,+gradient_delta.z)).r
					));
#endif
			return;
		}

		prev_pos = current_pos;
		prev_value = current_value;
	}

	_exit = true;
}


in vec3 pos_in;
in vec3 dir_in;

void main(void)
{
	_pos = pos_in*texture_size;
	_dir = normalize(dir_in);	// world space to volume space

	_exit = false;
	rayCast();

	if (_exit)	discard;

	/**
	 * scene_volume_position is the coordinate in 'cube' space when drawing ([0;1]).
	 * by translating and scaling this way, the corners of the volume match exactly the volume box
	 */
	vec4 scene_volume_position = vec4(_pos*vec3(2)-vec3(1), 1);
	frag_data = blinnShading(	vec4(1,1,1,1),
								normalize(view_model_normal_matrix3*_normal),
								vec3(view_matrix*model_matrix*scene_volume_position)
							);

	/**
	 * we have to update the depth value
	 *
	 * look at 2.13.1 Controlling the Viewport in the OpenGL 3.2 (Core Profile) specification
	 * for this operations (we have to remap the depth values)
	 */
	vec4 frag_pos = pvm_matrix*scene_volume_position;
	float frag_depth = frag_pos.z / frag_pos.w;
	gl_FragDepth = (gl_DepthRange.diff*frag_depth+(gl_DepthRange.near+gl_DepthRange.far))*0.5;
}
