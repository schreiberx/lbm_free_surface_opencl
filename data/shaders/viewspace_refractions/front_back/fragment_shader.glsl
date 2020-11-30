// use opengl 3.2 core shaders


out vec4 frag_data;

uniform sampler2D front_voxel_texture;
uniform sampler2D front_normal_texture;

uniform sampler2D back_voxel_texture;
uniform sampler2D back_normal_texture;

uniform samplerCube cube_map_texture;

uniform vec2 viewport_size;
uniform vec2 inv_viewport_size;

uniform mat3 inv_view_matrix3;		// inverse of view matrix
uniform mat4 projection_matrix;		// projection matrix

uniform float refraction_index;

uniform float step_size = 1.0;

uniform float water_reflectance_at_normal_incidence;


vec3 _pos;		// current 3d position
vec3 _dir;		// direction (normalized!)
vec2 _tex_pos;	// current position in texture
vec4 _tmp;		// global temporary variable


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
					vec3 view_normal3,		///< normal of fragment in viewspace
					vec3 view_position3		///< position of fragment in viewspace
);
*/

void getNextIntersectionPeeling()
{
	/*
	 * project raycasting direction to screenspace
	 */
	vec4 p1 = projection_matrix*vec4(_pos, 1);
	p1.xy /= p1.w;

	vec4 p2 = projection_matrix*vec4(_pos+normalize(_dir), 1);
	p2.xy /= p2.w;

	vec2 tex_iter_dir = normalize((p2.xy - p1.xy)*viewport_size)*inv_viewport_size;

	// compute distance to texture borders in texture steps
	_tmp.xy = (vec2(greaterThanEqual(tex_iter_dir, vec2(0)))-_tex_pos)/tex_iter_dir;

	// max iterations
	// add 2 for numerical reasons (avoiding gaps at texture borders)
	float max_iters = min(min(_tmp.x, _tmp.y), min(viewport_size.x, viewport_size.y))+2.0;

	/*
	 * compute matrix to switch to ray-dir basis with it's origin at _pos
	 */
	vec3 U = normalize(cross(_dir, cross(-_pos, _dir)));

	mat3x2 ray_base = mat3x2(
						_dir[0], U[0],
						_dir[1], U[1],
						_dir[2], U[2]
				);

	vec3 prev_voxel = _pos;
	prev_voxel.z = (1.0/0.0);

	/*
	 * because the volume is usually sampled at a higher resolution compared to the resolution of the volume
	 * (screen resolution > volume resolution), we can allow larger step sized
	 */
	for (float i = 0; i < max_iters; i += step_size)
	{
		// get value at current sampling point
		// _tmp.xyz now holds the voxel position
		vec3 voxel = texture(back_voxel_texture, _tex_pos + tex_iter_dir*i).xyz;

		if (isinf(voxel.z))		break;	// no intersection found - we only care about internal refractions

		vec2 r = ray_base*(voxel-_pos);

		if (r.x < 0 || r.y < 0)
		{
			prev_voxel = voxel;
			continue;
		}

#if 0
		if (isinf(prev_voxel.z) || isinf(voxel.z))
		{
			_pos.x = (1.0/0.0);
			return;
		}
#else
		// use current texel value, if there's no voxel at the previous texel
		if (isinf(prev_voxel.z))
		{
			_pos = voxel.xyz;

			// update texture position
			_tex_pos += tex_iter_dir*i;
			return;
		}

		// use current texel value, if there's no voxel at the previous texel
		if (isinf(voxel.z))
		{
			_pos = prev_voxel.xyz;

			// update texture position
			_tex_pos += tex_iter_dir*(i-step_size);
			return;
		}
#endif

#if 0	// no interpolation

		// update texture position
		_tex_pos += tex_iter_dir*(i);
		_pos = voxel;
		return;
#else
		vec2 t = ray_base*(prev_voxel-_pos);

		// compute interpolated position
		float inv_delta = 1.0/(r.y - t.y);
		_pos = (	prev_voxel*(r.y) +
					voxel*(-t.y)
				)	* inv_delta;

		// update texture position
		_tex_pos += tex_iter_dir*(i-step_size*r.y*inv_delta);
		return;
#endif
	}
	_pos.x = (1.0/0.0);	// set x component to infinity
}




void main(void)
{
	_tmp = texelFetch(front_voxel_texture, ivec2(gl_FragCoord.xy), 0);

	if (isinf(_tmp.z))
	{
		discard;
		return;
	}

	gl_FragDepth = _tmp.a;		// the depth component was stored to the alpha channel in the first depth peeling pass

	_pos = _tmp.xyz;			// we start at the first voxel position (because we created the texture in perspective mode)
	_dir = normalize(_pos);		// because we are in viewspace, the position can be used as direction
	_tex_pos = gl_FragCoord.xy*inv_viewport_size;

	float inv_refraction_index = 1.0f/refraction_index;

	float state = 0.0;		// 0 == incoming ray	1 == exiting ray
	float state_pm = 1.0;

	// load normal
	vec3 normal = texelFetch(front_normal_texture, ivec2(gl_FragCoord.xy), 0).xyz;

	vec3 start_pos = _pos;
	vec3 start_normal = normal;

	// compute new direction
	vec3 refract_dir = refract(_dir, normal, inv_refraction_index);
	vec3 first_reflect_dir = reflect(_dir, normal);;

	if (refract_dir == vec3(0,0,0))
	{
		// total reflection
		// => reflect incient direction _dir on normal _normal;
		_dir = first_reflect_dir;
	}
	else
	{
		state = 1.0-state;
		state_pm = -state_pm;
		_dir = refract_dir;
	}

	// compute next intersection point
	getNextIntersectionPeeling();

	if (!isinf(_pos.x))	// no intersection point was found -> final ray
	{
		// load normal for intersection point
		normal = normalize(texture(back_normal_texture, _tex_pos).xyz);

		// compute new direction
		refract_dir = refract(_dir, -normal, refraction_index);

		if (refract_dir == vec3(0,0,0))
		{
			// total reflection
			// => reflect incient direction _dir on normal _normal;
			_dir = reflect(_dir, normal);
		}
		else
		{
			_dir = refract_dir;
		}
	}

	frag_data = blinnShadingVolume(
								texture(cube_map_texture, inv_view_matrix3*_dir),
								start_normal,
								start_pos
							);

	if (water_reflectance_at_normal_incidence != 0.0f)
	{
		// compute FRESNEL FACTOR for refracted ray using SCHLICKs approximation
		float kasten_bier = 1.0f - dot(first_reflect_dir, start_normal);
		float zwei_kaesten_bier = kasten_bier*kasten_bier;
		kasten_bier = zwei_kaesten_bier*zwei_kaesten_bier*kasten_bier;

		frag_data = mix(	frag_data,	// refracted data
							texture(cube_map_texture, inv_view_matrix3*first_reflect_dir),	// reflected environment
							water_reflectance_at_normal_incidence + (1.0f-water_reflectance_at_normal_incidence)*kasten_bier);

	}
}
