// use opengl 3.2 core shaders


out vec4 frag_data;

uniform sampler3D volume_texture;
uniform sampler2DRect back_texture;
uniform samplerCube cube_map;

uniform float step_length;
uniform float refraction_index;

uniform mat4 pvm_matrix;
uniform mat4 view_matrix;
uniform mat4 model_matrix;
uniform mat3 model_normal_matrix3;
uniform mat3 view_model_normal_matrix3;

uniform vec3 gradient_delta;

#define ISO_VALUE	0.5f
#define MAX_REFLECTIONS		(4)

uniform vec3 texture_size;
uniform vec3 inv_texture_size;
uniform vec3 texture_size_add_one;
uniform vec3 inv_texture_size_add_one;

uniform float water_reflectance_at_normal_incidence;

vec3 first_reflect_dir;

//////////////////////////////////////////////////////////////////////////////////////////////////////////

// LIGHT0
uniform bool light0_enabled = true;
uniform vec3 light0_view_pos3 = vec3(10,10,10);	// position in view space

uniform vec3 light0_ambient_color3 = vec3(1.f,1.f,1.f);
uniform vec3 light0_diffuse_color3 = vec3(1.f,1.f,1.f);
uniform vec3 light0_specular_color3 = vec3(1.f,1.f,1.f);

uniform vec3 material_ambient_color3 = vec3(1.f,1.f,1.f);
uniform vec3 material_diffuse_color3 = vec3(1.f,1.f,1.f);
uniform vec3 material_specular_color3 = vec3(1.f,1.f,1.f);
uniform float material_specular_exponent = 20.0f;

vec4 blinnShadingVolume(	vec4 material_color,	///< color of material
					vec3 view_normal3,		///< normal of fragment in viewspace
					vec3 view_position3		///< position of fragment in viewspace
)
{
	if (!light0_enabled)
		return material_color;

	view_normal3 = normalize(view_normal3);

	// compute vector from surface point to light
	vec3 light_vec = normalize(light0_view_pos3 - view_position3);

	vec3 viewer_vec = -normalize(view_position3);
	vec3 halfway_vec = normalize(light_vec+viewer_vec);

	// compute FRESNEL FACTOR for refracted ray using SCHLICKs approximation
	float kasten_bier = 1.0f - dot(viewer_vec, halfway_vec);
	float zwei_kaesten_bier = kasten_bier*kasten_bier;
	kasten_bier = zwei_kaesten_bier*zwei_kaesten_bier*kasten_bier;
	kasten_bier = water_reflectance_at_normal_incidence + (1.0f-water_reflectance_at_normal_incidence)*kasten_bier;

	return material_color * vec4(
					(
						light0_ambient_color3*material_ambient_color3	+
						max(0.0f, dot(light_vec, view_normal3))*light0_diffuse_color3*material_diffuse_color3
					), 1.0f)
					+
					kasten_bier *
					vec4(	(pow(max(0.0f, dot(halfway_vec, view_normal3)), material_specular_exponent)
							*light0_specular_color3*material_specular_color3), 1.0f);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////

/*
vec4 blinnShading(	vec4 material_color,	///< color of material
					vec3 view_normal3,			///< normal of fragment in viewspace
					vec3 view_position3		///< position of fragment in viewspace
);
*/

/**
 * return the color for the worldspace ray leaving the volume area
 */
void setExitRayValue(	vec3 exit_dir3,		// exit direction
						vec3 pos,			// position of entering the volume
						vec3 iso_normal3,	// normal at incident ray
						vec3 enter_dir		// enter direction (incoming volume position to first intersection point)
						)
{
	vec4 scene_volume_position = vec4(pos*vec3(2.0f,2.0f,2.0f)-vec3(1.0f,1.0f,1.0f), 1);
	frag_data = blinnShadingVolume(	texture(cube_map, model_normal_matrix3*exit_dir3),
								normalize(view_model_normal_matrix3*normalize(iso_normal3)),
								vec3(view_matrix*model_matrix*scene_volume_position)
						);

	if (water_reflectance_at_normal_incidence != 0.0f)
	{
		// compute FRESNEL FACTOR usind SCHLICKs approximation
		float kasten_bier = 1.0f - max(dot(-enter_dir, iso_normal3), 0);
		float zwei_kaesten_bier = kasten_bier*kasten_bier;
		kasten_bier = zwei_kaesten_bier*zwei_kaesten_bier*kasten_bier;

		frag_data = mix(	frag_data,
							texture(cube_map, model_normal_matrix3*first_reflect_dir),
							min(water_reflectance_at_normal_incidence + (1.0f-water_reflectance_at_normal_incidence)*kasten_bier, 1.0) );
	}

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

float state_pm;	// state variable to distinguish between entering and leaving ray

void rayCast()
{
	_pos *= texture_size;
	_dir = normalize(_dir*texture_size);

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

	for (current_step_fac = 1.0; current_step_fac < max_dist;)
	{
		// compute distances to next axes
		vec3 dist = (next_axes - pos)*inv_dir;

		// this seems to be much faster because it avoids a lot of branching
		float dist0_leq_dist1 = float(dist[0] <= dist[1]);
		float dist1_leq_dist2 = float(dist[1] <= dist[2]);
		float dist0_leq_dist2 = float(dist[0] <= dist[2]);

		// compute maximum distance
		float dist0 = dist0_leq_dist1*dist0_leq_dist2;
		float dist1 = dist1_leq_dist2*(1.0f-dist0_leq_dist1);
		float dist2 = (1.0f-dist0_leq_dist2)*(1.0f-dist1_leq_dist2);

		// get next axes for collision
		next_axes.x += axes_increments.x*dist0;
		next_axes.y += axes_increments.y*dist1;
		next_axes.z += axes_increments.z*dist2;

		// set step_fac to next distance value
		current_step_fac = dist.x*dist0 + dist.y*dist1 + dist.z*dist2;

		current_pos = _pos + _dir*current_step_fac;

		current_interpolated_texture_pos = current_pos*inv_texture_size;
		float current_value = texture(volume_texture, current_interpolated_texture_pos).r;

		if (current_value*state_pm >= ISO_VALUE*state_pm)
		{
			// compute interpolated position
			_pos = (	prev_pos*(current_value - ISO_VALUE) +
						current_pos*(ISO_VALUE - prev_value)
					)	/	(current_value - prev_value);

			current_interpolated_texture_pos = _pos*inv_texture_size;

#if 0
			current_value = texture(volume_texture, _pos).r;

			_normal = normalize(
				vec3(	current_value - texture(volume_texture, current_interpolated_texture_pos+vec3(+gradient_delta.x*2.0,0,0)).r,
						current_value - texture(volume_texture, current_interpolated_texture_pos+vec3(0,+gradient_delta.y*2.0,0)).r,
						current_value - texture(volume_texture, current_interpolated_texture_pos+vec3(0,0,+gradient_delta.z*2.0)).r
					));
#else
			_normal = normalize(
				vec3(
						texture(volume_texture, current_interpolated_texture_pos+vec3(-gradient_delta.x,0,0)).r - texture(volume_texture, current_interpolated_texture_pos+vec3(+gradient_delta.x,0,0)).r,
						texture(volume_texture, current_interpolated_texture_pos+vec3(0,-gradient_delta.y,0)).r - texture(volume_texture, current_interpolated_texture_pos+vec3(0,+gradient_delta.y,0)).r,
						texture(volume_texture, current_interpolated_texture_pos+vec3(0,0,-gradient_delta.z)).r - texture(volume_texture, current_interpolated_texture_pos+vec3(0,0,+gradient_delta.z)).r
					));
#endif
			_pos *= inv_texture_size;
			_dir = normalize(_dir*inv_texture_size);
			return;
		}

		prev_pos = current_pos;
		prev_value = current_value;
	}

	_pos *= inv_texture_size;
	_dir = normalize(_dir*inv_texture_size);
	_exit = true;
}



in vec3 pos_in;
in vec3 dir_in;

void main(void)
{
	_pos = pos_in;
	_dir = normalize(dir_in*inv_texture_size);	// world space to volume space
	vec3 enter_dir = _dir;

	float inv_refraction_index = 1.0f/refraction_index;

	float state = 0.0;	// 0 == incoming ray	1 == exiting ray
	state_pm = 1.0;
	vec3 refract_dir;

	_exit = false;

	/*
	 * FIRST INTERSECTION
	 */
	rayCast();

	/*
	 * if the ray did not hit any surface in the first ray processing,
	 * discard frame (let the ray pass through the volume without any change)
	 */
	if (_exit)	discard;

	_dir = normalize(_dir*texture_size);		// dir to world space
	_normal = normalize(_normal*texture_size);	// normal to world space

	// save fluid entering position for final lighting computations
	vec3 enter_pos = _pos;
	vec3 enter_normal = _normal;

	// refraction direction
	refract_dir = refract(_dir, _normal, state*refraction_index + (1.0-state)*inv_refraction_index);
	first_reflect_dir = reflect(_dir, _normal);

	if (refract_dir == vec3(0,0,0))
	{
		// total reflection
		// => reflect incient direction _dir on normal _normal;
		refract_dir = first_reflect_dir;
	}
	else
	{
		state = 1.0;
		state_pm = -1.0;
	}

	refract_dir = normalize(refract_dir*inv_texture_size);	// refract_dir to volume space
	_dir = normalize(refract_dir);

	/*
	 * 2nd to MAX_REFLECTIONS INTERSECTIONS
	 */
	for (int i = 1; i < MAX_REFLECTIONS; i++)
	{
		_pos += _dir*step_length;

		rayCast();

		if (_exit)	break;

		_dir = normalize(_dir*texture_size);		// dir to world space
		_normal = normalize(_normal*texture_size);	// normal to world space

		/*
		 * reflect light ray with direction _dir on normal _normal;
		 */
		vec3 normal_fix = state_pm*_normal;

		float _refraction_index = state*refraction_index + (1.0-state)*inv_refraction_index;
		refract_dir = refract(_dir, normal_fix, _refraction_index);

		if (refract_dir == vec3(0,0,0))
		{
			// total reflection
			// => reflect incient direction _dir on normal _normal;
			refract_dir = reflect(_dir, _normal);
		}
		else
		{
			state = 1.0-state;
			state_pm = -state_pm;
		}
		refract_dir = normalize(refract_dir*inv_texture_size);	// refract_dir to volume space

		_dir = normalize(refract_dir);
	}

	setExitRayValue(normalize(_dir*texture_size*texture_size), enter_pos, enter_normal, enter_dir);
}
