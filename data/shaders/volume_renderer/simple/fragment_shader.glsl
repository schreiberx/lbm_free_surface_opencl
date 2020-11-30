// use opengl 3.2 core shaders


out vec4 frag_data;

uniform sampler3D volume_texture;

uniform mat4 pvm_matrix;
uniform mat4 view_matrix;
uniform mat4 model_matrix;
uniform mat3 model_normal_matrix3;
uniform mat3 view_model_normal_matrix3;

uniform float step_length;
uniform vec3 gradient_delta;
#define ISO_VALUE	0.5f

uniform vec3 texture_size;
uniform vec3 inv_texture_size;
uniform vec3 texture_size_add_one;
uniform vec3 inv_texture_size_add_one;



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
	/*
	 * because we are only interested in intersection points in the direction
	 * of dir, we first compute the distances to the collision surfaces
	 */
	vec3 collide_surface = vec3(greaterThanEqual(_dir, vec3(0,0,0)));
	vec3 dist = (collide_surface - _pos)/_dir;

	float max_dist = min(min(min(dist[0], dist[1]), dist[2]), sqrt(3.0));

	for (float step_fac = step_length; step_fac < max_dist; step_fac += step_length)
	{
		vec3 pos = _pos + _dir*step_fac;

		float value = texture(volume_texture, pos).r;
		if (value >= ISO_VALUE)
		{
			_normal = normalize(
#if 0
			vec3(
					value - texture(volume_texture, pos+vec3(+gradient_delta.x*2.0,0,0)).r,
					value - texture(volume_texture, pos+vec3(0,+gradient_delta.y*2.0,0)).r,
					value - texture(volume_texture, pos+vec3(0,0,+gradient_delta.z*2.0)).r
				)
#elif 1
			vec3(
					texture(volume_texture, pos+vec3(-gradient_delta.x,0,0)).r - texture(volume_texture, pos+vec3(+gradient_delta.x,0,0)).r,
					texture(volume_texture, pos+vec3(0,-gradient_delta.y,0)).r - texture(volume_texture, pos+vec3(0,+gradient_delta.y,0)).r,
					texture(volume_texture, pos+vec3(0,0,-gradient_delta.z)).r - texture(volume_texture, pos+vec3(0,0,+gradient_delta.z)).r
				)
#else
			vec3(
					texture(volume_texture, pos+vec3(-gradient_delta.x,0,0)).r - texture(volume_texture, pos+vec3(+gradient_delta.x,0,0)).r,
					texture(volume_texture, pos+vec3(0,-gradient_delta.x,0)).r - texture(volume_texture, pos+vec3(0,+gradient_delta.x,0)).r,
					texture(volume_texture, pos+vec3(0,0,-gradient_delta.x)).r - texture(volume_texture, pos+vec3(0,0,+gradient_delta.x)).r
				)
#endif
			);
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

	_exit = false;
	rayCast();

	if (_exit)	discard;

	/**
	 * scene_volume_position is the coordinate in model space when drawing.
	 * by translating and scaling this way, the corners of the volume match exactly the volume box
	 */
	vec4 scene_volume_position = vec4(_pos*vec3(2.f,2.f,2.f)-vec3(1.f,1.f,1.f), 1);
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
	return;
}
