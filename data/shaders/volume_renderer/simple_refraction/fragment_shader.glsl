// use opengl 3.2 core shaders


out vec4 frag_data;

uniform sampler3D volume_texture;
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
 * follow a ray until an isosurface is entered
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
	/**
	 * compute distance to intersection with box [0,0,0]x[1,1,1]
	 */
	/*
	 * because we are only interested in intersection points in the direction
	 * of dir, we first compute the distances to the collision surfaces
	 */
	vec3 collide_surface = vec3(greaterThanEqual(_dir, vec3(0,0,0)));
	vec3 dist = (collide_surface - _pos)/_dir;

	float max_dist = min(min(min(dist[0], dist[1]), dist[2]), sqrt(3.0));

	for (float step_fac = step_length*0.5; step_fac < max_dist; step_fac += step_length)
	{
		vec3 pos = _pos + _dir*step_fac;

		float value = texture(volume_texture, pos).r;
		if (value*state_pm >= ISO_VALUE*state_pm)
		{
#if 0
			_normal = normalize(vec3(
					value - texture(volume_texture, pos+vec3(+gradient_delta.x*2.0,0,0)).r,
					value - texture(volume_texture, pos+vec3(0,+gradient_delta.y*2.0,0)).r,
					value - texture(volume_texture, pos+vec3(0,0,+gradient_delta.z*2.0)).r
				));
#elif 1
			_normal = normalize(vec3(
					texture(volume_texture, pos+vec3(-gradient_delta.x,0,0)).r - texture(volume_texture, pos+vec3(+gradient_delta.x,0,0)).r,
					texture(volume_texture, pos+vec3(0,-gradient_delta.y,0)).r - texture(volume_texture, pos+vec3(0,+gradient_delta.y,0)).r,
					texture(volume_texture, pos+vec3(0,0,-gradient_delta.z)).r - texture(volume_texture, pos+vec3(0,0,+gradient_delta.z)).r
				));
#endif
			
			_normal = normalize(_normal);
			_pos = pos;
			return;
		}
	}

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
			// => reflect incient direction _dir on normal;
			refract_dir = reflect(_dir, normal_fix);
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
