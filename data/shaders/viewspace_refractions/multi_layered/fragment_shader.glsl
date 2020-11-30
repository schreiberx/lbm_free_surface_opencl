// use opengl 3.2 core shaders


out vec4 frag_data;

uniform sampler2D first_voxel_texture;
uniform sampler2D first_normal_texture;

uniform sampler2DArray voxel_texture;
uniform sampler2DArray normal_texture;

uniform samplerCube cube_map_texture;

uniform vec2 inv_viewport;

uniform mat3 inv_view_matrix3;		// inverse of view matrix
uniform mat4 projection_matrix;		// inverse transpose of pvm matrix

uniform float refraction_index;

uniform float step_size = 1.0;


vec4 blinnShading(	vec4 material_color,	///< color of material
					vec3 view_normal3,			///< normal of fragment in viewspace
					vec3 view_position3		///< position of fragment in viewspace
);


vec3 _pos;		// current 3d position
vec3 _dir;		// direction (normalized!)
vec2 _tex_pos;	// current position in texture
vec4 _tmp;		// global temporary variable

#if 0
void getNextIntersectionFirst()
{
#if 0
	vec2 tex_iter_dir = normalize(_dir.xy)/textureSize(first_voxel_texture, 0).xy;
#else
	if (_pos == vec3(0,0,0))	// displace pos.z slightly in case that pos is at origin
		_pos.z = -0.0001;

	vec4 p1 = projection_matrix*vec4(_pos, 1);
	p1.xy /= p1.w;

	vec4 p2 = projection_matrix*vec4(_pos+normalize(_dir), 1);
	p2.xy /= p2.w;

	vec2 tex_iter_dir = normalize((p2.xy - p1.xy)*textureSize(first_voxel_texture, 0).xy)/textureSize(first_voxel_texture, 0).xy;
#endif

	// compute distance to texture borders in texture steps
	_tmp.xy = (vec2(greaterThanEqual(tex_iter_dir.xy, vec2(0)))-_tex_pos)/tex_iter_dir.xy;

	// max iterations
	// add 2 for numerical reasons (avoiding gaps at texture borders)
	float max_iters = min(min(_tmp.x, _tmp.y)+2.0, 50000);

	vec3 prev_texel = _pos;

	for (float i = 0; i < max_iters; i += step_size)
	{
		// get value at current sampling point
		// _tmp.xyz not holds the voxel position
		_tmp.xyz = texture(first_voxel_texture, _tex_pos + tex_iter_dir*i).xyz;

		// compute the collision point of the ray with the plane at the voxel's z coordinate
		vec2 plane_intersection_point = _pos.xy + ((_pos.z - _tmp.z)/_dir.z)*_dir.xy;

		// compute 2d distance between plane intersection point and ray start position
		plane_intersection_point -= _pos.xy;
		float b = plane_intersection_point.x*plane_intersection_point.x + plane_intersection_point.y*plane_intersection_point.y;

		// reuse plane_intersection_point as temporary variable!!!
		// compute 2d distance between voxel and ray start position
		plane_intersection_point = _tmp.xy - _pos.xy;
		float a = plane_intersection_point.x*plane_intersection_point.x + plane_intersection_point.y*plane_intersection_point.y;

		if (a > b)
		{
			_tex_pos += tex_iter_dir*i;
			_pos = _tmp.xyz;
			return;
			// TODO: flatten values
#if 0
			if (isinf(prev_texel.z) || isinf(_tmp.z))
			{
				_pos.x = (1.0/0.0);
				return;
			}
#else
			// use current texel value, if there's no voxel at the previous texel
			if (isinf(prev_texel.z))
			{
				_pos = _tmp.xyz;

				// update texture position
				_tex_pos += tex_iter_dir*i;
				return;
			}

			// use current texel value, if there's no voxel at the previous texel
			if (isinf(_tmp.z))
			{
				_pos = prev_texel.xyz;

				// update texture position
				_tex_pos += tex_iter_dir*(i-1);
				return;
			}
#endif
			vec3 t = _tmp.xyz - prev_texel;
			vec3 v = prev_texel - _pos;

//			v.x = sqrt(v.x*v.x + v.y*v.y);
//			t.x = sqrt(t.x*t.x + t.y*t.y);

			_pos += (v.z*t.x - v.x*t.z)/(_dir.z*t.x - _dir.x*t.z) * _dir;

			// update texture position
			// TODO: texture position should be updated with interpolation
			_tex_pos += tex_iter_dir*i;
			return;
		}

		prev_texel = _tmp.xyz;
	}
	_pos.x = (1.0/0.0);	// set x component to infinity
}
#endif


void getNextIntersectionPeeling(float layer)
{
	/*
	 * project raycasting direction to screenspace
	 */
	vec4 p1 = projection_matrix*vec4(_pos, 1);
	p1.xy /= p1.w;

	vec4 p2 = projection_matrix*vec4(_pos+normalize(_dir), 1);
	p2.xy /= p2.w;

	vec2 tex_iter_dir = normalize((p2.xy - p1.xy)*textureSize(voxel_texture, 0).xy)/textureSize(voxel_texture, 0).xy;

	// compute distance to texture borders in texture steps
	_tmp.xy = (vec2(greaterThanEqual(tex_iter_dir, vec2(0)))-_tex_pos)/tex_iter_dir;

	// max iterations
	// add 2 for numerical reasons (avoiding gaps at texture borders)
	float max_iters = min(min(_tmp.x, _tmp.y)+2.0, 50000);

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
		vec3 voxel = texture(voxel_texture, vec3(_tex_pos + tex_iter_dir*i, layer)).xyz;

		if (isinf(voxel.z))		continue;

		vec2 r = ray_base*(voxel-_pos);

		if (r.x < 0 || r.y < 0)
		{
			prev_voxel = voxel;
			continue;
		}

		vec2 s = ray_base*(prev_voxel-_pos);

		if (s.y > 0)
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
	_tmp = texelFetch(first_voxel_texture, ivec2(gl_FragCoord.xy), 0);

	if (isinf(_tmp.z))
	{
		discard;
		return;
	}

	gl_FragDepth = _tmp.a;		// the depth component was stored to the alpha channel in the first depth peeling pass

	_pos = _tmp.xyz;			// we start at the first voxel position (because we created the texture in perspective mode)
	_dir = normalize(_pos);		// because we are in viewspace, the position can be used as direction
	_tex_pos = gl_FragCoord.xy/textureSize(first_voxel_texture, 0).xy;

	float inv_refraction_index = 1.0f/refraction_index;

	float state = 0.0;		// 0 == incoming ray	1 == exiting ray
	float state_pm = 1.0;

	// load normal
	vec3 normal = texelFetch(first_normal_texture, ivec2(gl_FragCoord.xy), 0).xyz;

	vec3 start_pos = _pos;
	vec3 start_normal = normal;

	vec3 refract_dir = refract(_dir, normal*state_pm, state*refraction_index + (1.0-state)*inv_refraction_index);

	if (refract_dir == vec3(0,0,0))
	{
		// total reflection
		// => reflect incient direction _dir on normal _normal;
		_dir = reflect(_dir, normal);
	}
	else
	{
		state = 1.0;
		state_pm = -1.0;
		_dir = refract_dir;
	}

	for (int i = 1; i < MAX_PEELINGS; i++)
	{
		// compute next intersection point
		getNextIntersectionPeeling(i);

		if (isinf(_pos.x))	// no intersection point was found -> final ray
			break;

		// load normal for intersection point
		normal = normalize(texture(normal_texture, vec3(_tex_pos, i)).xyz);
		vec3 refract_dir = refract(_dir, normal*state_pm, state*refraction_index + (1.0-state)*inv_refraction_index);

		if (refract_dir == vec3(0,0,0))
		{
			// total reflection
			// => reflect incient direction _dir on normal _normal;
			_dir = reflect(_dir, normal);
		}
		else
		{
			state = 1.0-state;
			state_pm = -state_pm;
			_dir = refract_dir;
		}
	}

	frag_data = blinnShading(	texture(cube_map_texture, inv_view_matrix3*_dir),
								start_normal,
								start_pos
							);
}
