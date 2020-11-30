// use opengl 3.2 core shaders


out vec4 front_frag_data;
out vec4 front_frag_normal;
out float front_frag_depth;

out vec4 back_frag_data;
out vec4 back_frag_normal;
out float back_frag_depth;

uniform sampler3D volume_texture;

uniform float step_length;

uniform mat4 pvm_matrix;
uniform mat4 view_matrix;
uniform mat4 model_matrix;
uniform mat3 model_normal_matrix3;
uniform mat3 view_model_normal_matrix3;

uniform vec3 texture_size = vec3(8,8,8);
uniform vec3 inv_texture_size = vec3(1,1,1)/vec3(8,8,8);
uniform vec3 texture_size_add_one = vec3(8+1,8+1,8+1);
uniform vec3 inv_texture_size_add_one = vec3(1,1,1)/vec3(8+1,8+1,8+1);

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

/**
 * compute distance to intersection with box [0,0,0]x[1,1,1]
 */
vec3 collide_surface;
vec3 dist;
float max_dist;
vec3 inters_point;

void rayCast()
{
	for (float step_fac = 0; step_fac < max_dist; step_fac += step_length)
	{
		vec3 current_pos = _pos + _dir*step_fac;

		float current_value = texture(volume_texture, current_pos).r;

		if (current_value >= ISO_VALUE)
		{
//#define prev_pos	_normal
//#define prev_value	step_fac
			// abuse _normal as previous position
			vec3 prev_pos = _pos + _dir*(step_fac-step_length);
			// abuse step_fac for
			float prev_value = texture(volume_texture, prev_pos).r;

			// compute interpolated position
			_pos = (	prev_pos*(current_value - ISO_VALUE) +
						current_pos*(ISO_VALUE - prev_value)
					)	/	(current_value - prev_value);

			// interpolate between prev and current value
			_pos = (	prev_pos*(current_value - ISO_VALUE) +
						current_pos*(ISO_VALUE - prev_value)
					)	/ (current_value - prev_value);
//			_pos = prev_pos;
//#undef prev_pos
//#undef prev_value
#if 0
			current_value = texture(volume_texture, _pos).r;

			_normal = normalize(
				vec3(	current_value - texture(volume_texture, _pos+vec3(+gradient_delta.x*2.0,0,0)).r,
						current_value - texture(volume_texture, _pos+vec3(0,+gradient_delta.y*2.0,0)).r,
						current_value - texture(volume_texture, _pos+vec3(0,0,+gradient_delta.z*2.0)).r
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
	}

	_exit = true;
}


in vec3 pos_in;
in vec3 dir_in;

void main(void)
{
	vec4 frag_pos;
	vec4 scene_volume_position;

	_pos = pos_in;
	_dir = normalize(dir_in*inv_texture_size);	// world space to volume space

	_exit = false;


	/**
	 * compute distance to intersection with box [0,0,0]x[1,1,1]
	 */
	collide_surface = vec3(greaterThanEqual(_dir, vec3(0,0,0)));
	dist = (collide_surface - _pos)/_dir;

	max_dist = min(min(min(dist[0], dist[1]), dist[2]), sqrt(3.0));

	// intersection point on cube face
	inters_point = _pos + _dir*max_dist;

	rayCast();

	if (_exit)	discard;

	scene_volume_position = vec4(_pos*vec3(2.f,2.f,2.f)-vec3(1.f,1.f,1.f), 1);
	front_frag_data = vec4(vec3(view_matrix*model_matrix*scene_volume_position), 1);
	front_frag_normal = vec4(normalize(view_model_normal_matrix3*_normal), 1);

	frag_pos = pvm_matrix*scene_volume_position;
	front_frag_depth = (gl_DepthRange.diff*(frag_pos.z / frag_pos.w)+(gl_DepthRange.near+gl_DepthRange.far))*0.5;

	/**
	 * and the same procedure backwards...
	 */
	_pos = inters_point;
	_dir = -_dir;

	rayCast();

	scene_volume_position = vec4(_pos*vec3(2.f,2.f,2.f)-vec3(1.f,1.f,1.f), 1);
	back_frag_data = vec4(vec3(view_matrix*model_matrix*scene_volume_position), 1);
	back_frag_normal = vec4(normalize(view_model_normal_matrix3*_normal), 1);

	frag_pos = pvm_matrix*scene_volume_position;
	back_frag_depth = (gl_DepthRange.diff*(frag_pos.z / frag_pos.w)+(gl_DepthRange.near+gl_DepthRange.far))*0.5;

	return;
}
