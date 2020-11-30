// use opengl 3.2 core shaders


out vec4 frag_data;

uniform sampler2D front_voxel_texture;
uniform sampler2D front_normal_texture;

uniform sampler2D back_depth_texture;
uniform sampler2D back_normal_texture;

uniform samplerCube cube_map_texture;

uniform vec2 viewport_size;
uniform vec2 inv_viewport_size;

uniform mat3 inv_view_matrix3;		// inverse of view matrix
uniform mat4 projection_matrix;		// projection matrix
uniform mat4 inv_projection_matrix;	// inverse projection matrix

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


vec3 depthToView(float depth, vec2 tex_coord)
{
	_tmp.x = tex_coord.x*2.0-1.0;
	_tmp.y = tex_coord.y*2.0-1.0;
	_tmp.z = depth*2.0-1.0;
	_tmp.w = 1;

	// multiplication from the left side!!! (the vector is transposed)
	_tmp = inv_projection_matrix*_tmp;
	_tmp.xyz /= _tmp.w;

	return _tmp.xyz;
}


void getNextIntersectionPeeling()
{
	/*
	 * project raycasting direction to screenspace
	 */
#if 1
	vec4 p1 = projection_matrix*vec4(_pos, 1);
	p1.xy /= p1.w;

	vec4 p2 = projection_matrix*vec4(_pos+normalize(_dir), 1);
	p2.xy /= p2.w;

	vec2 tex_iter_dir = normalize((p2.xy - p1.xy)*viewport_size)*inv_viewport_size;
#else
	vec4 d = projection_matrix*vec4(_dir, 0);
//	d.xy /= d.w;
	vec2 tex_iter_dir = normalize(d.xy*textureSize(front_voxel_texture, 0).xy)/textureSize(front_voxel_texture, 0).xy;
#endif

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
#if 1
		_tmp.xy = _tex_pos + tex_iter_dir*i;
		vec3 voxel = depthToView(texture(back_depth_texture, _tmp.xy).r, _tmp.xy);
#else
		vec3 voxel;
		// use voxel variable as texture coordinates
		voxel.xy = _tex_pos + tex_iter_dir*i;

		_tmp.x = voxel.x*2.0-1.0;
		_tmp.y = voxel.y*2.0-1.0;
		_tmp.z = texture(back_depth_texture, voxel.xy).r*2.0-1.0;
		_tmp.w = 1;

		// multiplication from the left side!!! (the vector is transposed)
		_tmp = inv_projection_matrix*_tmp;
		voxel = _tmp.xyz/_tmp.w;
#endif
		if (isinf(voxel.z))		break;	// no intersection found - we only care about internal refractions

		vec2 r = ray_base*(voxel-_pos);

		if (r.x < 0 || r.y < 0)
		{
			prev_voxel = voxel;
			continue;
		}
/*
		vec2 s = ray_base*(prev_voxel-_pos);

		if (s.y > 0)
		{
			prev_voxel = voxel;
			continue;
		}
*/
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


	frag_data = blinnShading(	texture(cube_map_texture, inv_view_matrix3*_dir),
								start_normal,
								start_pos
							);
}
