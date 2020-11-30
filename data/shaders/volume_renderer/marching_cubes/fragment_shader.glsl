// use opengl 3.2 core shaders


out vec4 frag_data;

uniform isampler2D mc_indices_texture;
uniform sampler2D vertices_texture;
uniform sampler2D normals_texture;
uniform isampler2D triangle_vertices_texture;

#define ISO_VALUE	0.5f

uniform mat4 pvm_matrix;
uniform mat4 view_matrix;
uniform mat4 model_matrix;
uniform mat3 model_normal_matrix3;
uniform mat3 view_model_normal_matrix3;

uniform vec3 texture_size;
uniform vec3 inv_texture_size;
uniform vec3 texture_size_add_one;
uniform vec3 inv_texture_size_add_one;


uniform ivec3 domain_cells;
uniform int domain_cell_elements;

uniform int texture_width;
uniform int texture_height;

uniform int ft_z_mod;


vec4 blinnShading(	vec4 material_color,	///< color of material
					vec3 view_normal3,			///< normal of fragment in viewspace
					vec3 view_position3		///< position of fragment in viewspace
);

#define EPSILON	0.000001


vec3 _pos;		// position of current ray
vec3 _dir;		// direction of current ray

vec3 _normal;
bool _exit;

ivec2 texture_positions[12];

bool computeCollisionPoint(
			ivec2 texture_pos,
			int mc_index
)
{
	// compute collision point
	if (mc_index == 0 || mc_index == 255)
		return false;

	texture_pos.x *= 3;

	/** prepare texture reading position indexed with marching cube vertices **/
	texture_positions[0] = texture_pos;
	texture_positions[3] = ivec2(texture_pos.x+1, texture_pos.y);
	texture_positions[8] = ivec2(texture_pos.x+2, texture_pos.y);
	texture_positions[8] = ivec2(texture_pos.x+2, texture_pos.y);
	texture_positions[1] = ivec2(texture_pos.x+3+1, texture_pos.y);
	texture_positions[9] = ivec2(texture_pos.x+3+2, texture_pos.y);

	texture_positions[2] = ivec2(texture_pos.x, texture_pos.y+1);
	texture_positions[11] = ivec2(texture_pos.x+2, texture_pos.y+1);

	texture_positions[10] = ivec2(texture_pos.x+3+2, texture_pos.y+1);

	texture_pos.x += domain_cells.x*3;
#if 0
	if (texture_pos.x >= texture_width*3)
	{
		texture_pos.x -= texture_width*3;
		texture_pos.y += domain_cells.y;
	}
#else
	// glsl doesn't offer sync commands, thus we have to avoid divergence of thread paths
	int foo = int(texture_pos.x >= texture_width*3);
	texture_pos.x -= foo*texture_width*3;
	texture_pos.y += foo*domain_cells.y;
#endif
	texture_positions[4] = ivec2(texture_pos.x, texture_pos.y);
	texture_positions[7] = ivec2(texture_pos.x+1, texture_pos.y);
	texture_positions[5] = ivec2(texture_pos.x+3+1, texture_pos.y);
	texture_positions[6] = ivec2(texture_pos.x, texture_pos.y+1);

	vec3 v0, v1, v2;
	int vertex_id0, vertex_id1, vertex_id2;
	int vertex_count_plus_one = texelFetch(triangle_vertices_texture, ivec2(0, mc_index), 0).r;

	for (int vertex_index = 1; vertex_index < vertex_count_plus_one; vertex_index += 3)
	{
		// load vertex positions
		vertex_id0 = int(texelFetch(triangle_vertices_texture, ivec2(vertex_index, mc_index), 0).r);
		v0 = texelFetch(vertices_texture, texture_positions[vertex_id0], 0).xyz;

		vertex_id1 = int(texelFetch(triangle_vertices_texture, ivec2(vertex_index+1, mc_index), 0).r);
		v1 = texelFetch(vertices_texture, texture_positions[vertex_id1], 0).xyz;

		vertex_id2 = int(texelFetch(triangle_vertices_texture, ivec2(vertex_index+2, mc_index), 0).r);
		v2 = texelFetch(vertices_texture, texture_positions[vertex_id2], 0).xyz;

		vec3 e1 = v1 - v0;
		vec3 e2 = v2 - v0;

		vec3 pvec = cross(_dir, e2);
		float det = dot(e1, pvec);

		if (det < EPSILON)		continue;

		vec3 tvec = _pos - v0;
		vec3 q = cross(tvec, e1);

		float u = dot(tvec, pvec);

		if (u < -EPSILON || u > det)	continue;

		vec3 qvec = cross(tvec, e1);

		float v = dot(_dir, qvec);

		if (v < -EPSILON || u + v > det)	continue;

		float t = dot(e2, qvec);

		float inv_det = 1.0/det;
		t *= inv_det;
		u *= inv_det;
		v *= inv_det;

		vec3 n0 = texelFetch(normals_texture, texture_positions[vertex_id0], 0).xyz;
		vec3 n1 = texelFetch(normals_texture, texture_positions[vertex_id1], 0).xyz;
		vec3 n2 = texelFetch(normals_texture, texture_positions[vertex_id2], 0).xyz;

		_pos += t*_dir;
		_normal = normalize(n0 + (n1-n0)*u + (n2-n0)*v);
		return true;
	}
	return false;
}

void rayCast()
{
	vec3 collide_surface = vec3(greaterThanEqual(_dir, vec3(0,0,0)))*texture_size;
	vec3 dist = (collide_surface - _pos)/_dir;

	float max_dist = min(min(min(dist[0], dist[1]), dist[2]), length(texture_size));

	vec3 inv_dir = vec3(1.0f)/_dir;

	// increment values (-1, 0 or +1) for axes
	vec3 axes_increments = sign(_dir);

	// current cell coordinate
	ivec3 current_cell = ivec3(floor(_pos));

	// compute axes for next ray crossing to get positive distances
	vec3 next_axes =	floor(_pos) + vec3(greaterThanEqual(axes_increments, vec3(0,0,0)));

	for (float current_step_fac = 0.5; current_step_fac < max_dist;)
	{
		// compute distances to next axes
		vec3 dist = (next_axes - _pos)*inv_dir;

		// this seems to be much faster because it avoids a lot of branching
		float dist0_leq_dist1 = float(dist[0] <= dist[1]);
		float dist1_leq_dist2 = float(dist[1] <= dist[2]);
		float dist0_leq_dist2 = float(dist[0] <= dist[2]);

		// maximum distance
		float dist0 = dist0_leq_dist1*dist0_leq_dist2;
		float dist1 = dist1_leq_dist2*(1.0f-dist0_leq_dist1);
		float dist2 = (1.0f-dist0_leq_dist2)*(1.0f-dist1_leq_dist2);

		// get next axes for collision
		vec3 tmp;
		tmp.x = axes_increments[0]*dist0;
		tmp.y = axes_increments[1]*dist1;
		tmp.z = axes_increments[2]*dist2;

		next_axes += tmp;

		// set step_fac to next distance value
		current_step_fac = dist[0]*dist0 + dist[1]*dist1 + dist[2]*dist2;

		/**
		 * compute texture position for cell and load the value at the axis
		 * because this value is interpolated from the neighbor cells, we can
		 * assume an interface in the neighborhood, if the value is > 0
		 */
		vec3 current_pos = _pos + _dir*current_step_fac;
		ivec2 texture_pos = ivec2(	current_cell.x + domain_cells.x*(current_cell.z%ft_z_mod),
									current_cell.y + domain_cells.y*(current_cell.z/ft_z_mod)
								);
		int mc_index = texelFetch(mc_indices_texture, texture_pos, 0).r;

		if (computeCollisionPoint(texture_pos, mc_index))
			return;

		current_cell += ivec3(tmp);
	}

	_exit = true;
}

in vec3 pos_in;
in vec3 dir_in;

void main(void)
{
	_pos = pos_in*texture_size-vec3(0.5);
	_dir = normalize(dir_in);

	_exit = false;
	rayCast();

	if (_exit)	discard;
//	frag_data = vec4(0);	return;

	/**
	 * scene_volume_position is the coordinate in model space when drawing.
	 * by translating and scaling this way, the corners of the volume match exactly the volume box
	 */
	vec4 scene_volume_position = vec4(_pos*inv_texture_size*vec3(2.f,2.f,2.f)-vec3(1.f,1.f,1.f), 1);
	
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
