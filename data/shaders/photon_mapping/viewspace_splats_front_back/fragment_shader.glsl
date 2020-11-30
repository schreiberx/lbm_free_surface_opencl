// use opengl 3.2 core shaders


out vec4 out_photon_pos;	///< final position of photon
out vec4 out_surface_normal_and_attenuation;	///< surface normal and angle(surface, direction))

uniform sampler2D front_voxel_texture;
uniform sampler2D front_normal_texture;

uniform sampler2D back_voxel_texture;
uniform sampler2D back_normal_texture;

uniform sampler2D diffuse_voxel_texture;
uniform sampler2D diffuse_normal_texture;

uniform vec2 peeling_texture_size;
uniform vec2 inv_peeling_texture_size;
uniform vec2 inv_photon_texture_size;
uniform vec2 scale_photon_to_peeling_texture_coord;

uniform mat4 projection_matrix;		// projection matrix

uniform float refraction_index;

uniform float step_size = 1.0;


vec3 _pos;		// current 3d position
vec3 _dir;		// direction (normalized!)
vec2 _tex_pos;	// current position in texture
vec4 _tmp;		// global temporary variable
float distance;


void getDiffuseIntersection()
{
	_dir = normalize(_dir);

	/*
	 * project raycasting direction to screenspace
	 */
	vec4 p1 = projection_matrix*vec4(_pos, 1);
	p1.xy /= p1.w;

	vec4 p2 = projection_matrix*vec4(_pos+normalize(_dir), 1);
	p2.xy /= p2.w;

	vec2 tex_iter_dir = normalize((p2.xy - p1.xy)*peeling_texture_size)*inv_peeling_texture_size;

	// compute distance to texture borders in texture steps
	_tmp.xy = (vec2(greaterThanEqual(tex_iter_dir, vec2(0)))-_tex_pos)/tex_iter_dir;

	// max iterations
	// add 2 for numerical reasons (avoiding gaps at texture borders)
	float max_iters = min(min(_tmp.x, _tmp.y), min(peeling_texture_size.x, peeling_texture_size.y))+2.0;

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
		vec3 voxel = texture(diffuse_voxel_texture, _tex_pos + tex_iter_dir*i).xyz;

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
			_pos.x = 1.0/0.0;
			return;
		}
#else
		// use current texel value, if there's no voxel at the previous texel
		if (isinf(prev_voxel.z))
		{
			distance += length(_pos-voxel.xyz);
			_pos = voxel.xyz;

			// update texture position
			_tex_pos += tex_iter_dir*i;
			return;
		}

		// use current texel value, if there's no voxel at the previous texel
		if (isinf(voxel.z))
		{
			distance += length(_pos-prev_voxel.xyz);
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

		p1.xyz = _pos;
		_pos = (	prev_voxel*(r.y) +
					voxel*(-t.y)
				)	* inv_delta;

		distance += length(_pos-p1.xyz);

		// update texture position
		_tex_pos += tex_iter_dir*(i-step_size*r.y*inv_delta);
		return;
#endif
	}
	_pos.x = (1.0/0.0);	// set x component to infinity
}

void getNextIntersectionPeeling()
{
	/*
	 * project raycasting direction to screenspace
	 */

	vec4 p1 = projection_matrix*vec4(_pos, 1);
	p1.xy /= p1.w;

	vec4 p2 = projection_matrix*vec4(_pos+normalize(_dir), 1);
	p2.xy /= p2.w;

	vec2 tex_iter_dir = normalize((p2.xy - p1.xy)*peeling_texture_size)*inv_peeling_texture_size;

	// compute distance to texture borders in texture steps
	_tmp.xy = (vec2(greaterThanEqual(tex_iter_dir, vec2(0)))-_tex_pos)/tex_iter_dir;

	// max iterations
	// add 2 for numerical reasons (avoiding gaps at texture borders)
	float max_iters = min(_tmp.x, _tmp.y)+2.0;

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
			distance += length(_pos-voxel.xyz);
			_pos = voxel.xyz;

			// update texture position
			_tex_pos += tex_iter_dir*i;
			return;
		}

		// use current texel value, if there's no voxel at the previous texel
		if (isinf(voxel.z))
		{
			distance += length(_pos-prev_voxel.xyz);
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

		p1.xyz = _pos;
		_pos = (	prev_voxel*(r.y) +
					voxel*(-t.y)
				)	* inv_delta;

		distance += length(_pos-p1.xyz);

		// update texture position
		_tex_pos += tex_iter_dir*(i-step_size*r.y*inv_delta);
		return;
#endif
	}
	_pos.x = (1.0/0.0);	// set x component to infinity
}




void main(void)
{
	_tmp = texelFetch(front_voxel_texture, ivec2(gl_FragCoord.xy*scale_photon_to_peeling_texture_coord), 0);

	if (isinf(_tmp.z))
	{
		out_photon_pos.x = 1.0/0.0;
		return;
	}

	gl_FragDepth = _tmp.a;		// the depth component was stored to the alpha channel in the first depth peeling pass

	_pos = _tmp.xyz;			// we start at the first voxel position (because we created the texture in perspective mode)
	_dir = normalize(_pos);		// because we are in viewspace, the position can be used as direction
	_tex_pos = gl_FragCoord.xy*inv_photon_texture_size;

	distance = length(_pos);

	float inv_refraction_index = 1.0f/refraction_index;

	float state = 0.0;		// 0 == incoming ray	1 == exiting ray
	float state_pm = 1.0;

	// load normal
	vec3 normal = texelFetch(front_normal_texture, ivec2(gl_FragCoord.xy*scale_photon_to_peeling_texture_coord), 0).xyz;

	// compute new direction
	vec3 refract_dir = refract(_dir, normal, inv_refraction_index);

	if (refract_dir == vec3(0,0,0))
	{
		// total reflection

		// not interesting for this version of photon mapping => discard
		out_photon_pos.x = 1.0/0.0;
		return;

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

	if (isinf(_pos.x))
	{
		out_photon_pos.x = 1.0/0.0;
		return;
	}

	// load normal for intersection point
	normal = normalize(texture(back_normal_texture, _tex_pos).xyz);

	// compute new direction
	refract_dir = refract(_dir, -normal, refraction_index);

	if (refract_dir == vec3(0,0,0))
	{
		// total reflection

		// not interesting for this version of photon mapping => discard
		out_photon_pos.x = 1.0/0.0;
		return;

		// => reflect incient direction _dir on normal _normal;
		_dir = reflect(_dir, normal);
	}
	else
	{
		_dir = refract_dir;
	}

	getDiffuseIntersection();
//	_pos -= _dir*0.02;

	// output photon position and setup alpha channel with the angle between the surface and photon direction
	out_photon_pos = vec4(_pos, distance);

	normal = -normalize(texture(diffuse_normal_texture, _tex_pos).xyz);
	out_surface_normal_and_attenuation = vec4(normal, dot(normalize(_dir), normal));
}
