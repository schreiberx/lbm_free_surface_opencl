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
uniform mat4 projection_matrix;		// inverse transpose of pvm matrix

uniform float refraction_index;

uniform float step_size = 1.0;

#define MAX_REFLECTIONS		(4)


vec4 blinnShading(	vec4 material_color,	///< color of material
					vec3 view_normal3,			///< normal of fragment in viewspace
					vec3 view_position3		///< position of fragment in viewspace
);

vec3 _pos;		// current 3d position
vec3 _dir;		// direction (normalized!)
vec2 _tex_pos;	// current position in texture
vec3 _normal;	// normal for current 3d position
vec4 _tmp;		// global temporary variable

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

	/*
	 * slightly displace to avoid intersection with the previous voxel
	 */
	_pos += _dir*0.0001;
	_tex_pos += tex_iter_dir*0.0001;

	/*
	 * because the volume is usually sampled at a higher resolution compared to the resolution of the volume
	 * (screen resolution > volume resolution), we can allow larger step sized
	 */
	for (float i = 0; i < max_iters; i += step_size)
	{
		// get value at current sampling point
		// _tmp.xyz now holds the voxel position
		vec3 voxel_front = texture(front_voxel_texture, _tex_pos + tex_iter_dir*i).xyz;
		vec3 voxel_back = texture(back_voxel_texture, _tex_pos + tex_iter_dir*i).xyz;

		if (isinf(voxel_front.z + voxel_back.z))		break;

		vec2 r_front = ray_base*(voxel_front - _pos);
		vec2 r_back = ray_base*(voxel_back - _pos);

		bool intersection_front = r_front.x >= 0 && r_front.y <= 0;
		bool intersection_back = r_back.x >= 0 && r_back.y >= 0;

		// go to next intersection test if no intersection was found
		if (!(intersection_front || intersection_back))
			continue;

		vec3 prev_voxel;
		vec2 r;
		vec3 voxel;
		if (intersection_front)
		{
			// intersection with front texture
			voxel = voxel_front;
			prev_voxel = texture(front_voxel_texture, _tex_pos + tex_iter_dir*(i-1)).xyz;
			r = r_front;
		}
		else
		{
			// intersection with back texture
			voxel = voxel_back;
			prev_voxel = texture(back_voxel_texture, _tex_pos + tex_iter_dir*(i-1)).xyz;
			r = r_back;
		}
#if 0
		// no interpolation
		_tex_pos += tex_iter_dir*(i);
		_pos = voxel;
#else
		// linear interpolation
		vec2 t = ray_base*(prev_voxel-_pos);

		// compute interpolated position
		float inv_delta = 1.0/(r.y - t.y);
		_pos = (	prev_voxel*(r.y) +
					voxel*(-t.y)
				)	* inv_delta;

		// update texture position
		_tex_pos += tex_iter_dir*(i-step_size*r.y*inv_delta);
#endif

		if (intersection_front)		_normal = texture(front_normal_texture, _tex_pos).xyz;
		else						_normal = texture(back_normal_texture, _tex_pos).xyz;
		return;
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

	vec3 start_pos = _pos;

	// load normal
	vec3 start_normal = texelFetch(front_normal_texture, ivec2(gl_FragCoord.xy), 0).xyz;

	// compute new direction
	vec3 refract_dir = refract(_dir, start_normal, inv_refraction_index);

	if (refract_dir == vec3(0,0,0))
	{
		// total reflection
		// => reflect incient direction _dir on normal _normal;
		_dir = reflect(_dir, start_normal);
	}
	else
	{
		_dir = refract_dir;

		for (int i = 1; i < MAX_REFLECTIONS; i++)
		{
			// compute next intersection point
			getNextIntersectionPeeling();

			if (isinf(_pos.x))	// no intersection point was found -> final ray
				break;

			// compute new direction
			refract_dir = refract(_dir, -_normal, refraction_index);

			if (refract_dir == vec3(0,0,0))
			{
				// total reflection
				// => reflect incient direction _dir on normal _normal;
				_dir = reflect(_dir, _normal);
			}
			else
			{
				_dir = refract_dir;
				break;	// STOP here, because we exit the volume
			}
		}
	}

	frag_data = blinnShading(	texture(cube_map_texture, inv_view_matrix3*_dir),
								start_normal,
								start_pos
							);
}
