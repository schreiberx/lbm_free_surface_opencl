// use opengl 3.2 core shaders



/*
 * use sampler2DRect if texture has internal float format
 * use isampler2DRect if texture has internal integer format!!!
 */
uniform isampler2D mc_indices_texture;
uniform sampler2D vertices_texture;
uniform sampler2D normals_texture;
uniform isampler2D triangle_vertices_texture;

uniform mat4 pvm_matrix;
uniform mat3 pvm_normal_matrix3;
uniform mat4 view_model_matrix;
uniform mat3 view_model_normal_matrix3;

layout(points) in;
layout(triangle_strip, max_vertices = 15) out;

out vec3 frag_position3;
out vec3 frag_normal3;

in int index[];

// a passthrough geometry shader for color and position
void main()
{
	int cell_index = index[0];

	ivec2 texture_pos;
	texture_pos.x = cell_index % TEXTURE_WIDTH;
	texture_pos.y = cell_index / TEXTURE_WIDTH;

	int mc_index = texelFetch(mc_indices_texture, texture_pos, 0).r;

	if (mc_index == 0 || mc_index == 255)
		return;

	ivec3 volume_pos;
	volume_pos.x = texture_pos.x % DOMAIN_CELLS_X;
	volume_pos.y = texture_pos.y % DOMAIN_CELLS_Y;
	volume_pos.z = FT_Z_MOD*(cell_index/(TEXTURE_WIDTH*DOMAIN_CELLS_Y)) +
					(cell_index%TEXTURE_WIDTH)/DOMAIN_CELLS_X;

	if (volume_pos.z >= DOMAIN_CELLS_Z)
		return;

	texture_pos.x *= 3;

	ivec2 texture_positions[12];
	texture_positions[0] = texture_pos;
	texture_positions[3] = ivec2(texture_pos.x+1, texture_pos.y);
	texture_positions[8] = ivec2(texture_pos.x+2, texture_pos.y);
	texture_positions[8] = ivec2(texture_pos.x+2, texture_pos.y);
	texture_positions[1] = ivec2(texture_pos.x+3+1, texture_pos.y);
	texture_positions[9] = ivec2(texture_pos.x+3+2, texture_pos.y);

	texture_positions[2] = ivec2(texture_pos.x, texture_pos.y+1);
	texture_positions[11] = ivec2(texture_pos.x+2, texture_pos.y+1);

	texture_positions[10] = ivec2(texture_pos.x+3+2, texture_pos.y+1);

	texture_pos.x += DOMAIN_CELLS_X*3;
	if (texture_pos.x >= TEXTURE_WIDTH*3)
	{
		texture_pos.x -= TEXTURE_WIDTH*3;
		texture_pos.y += DOMAIN_CELLS_Y;
	}

	texture_positions[4] = ivec2(texture_pos.x, texture_pos.y);
	texture_positions[7] = ivec2(texture_pos.x+1, texture_pos.y);
	texture_positions[5] = ivec2(texture_pos.x+3+1, texture_pos.y);
	texture_positions[6] = ivec2(texture_pos.x, texture_pos.y+1);

	vec3 vertex_position3;
	int vertex_id;
	int vertex_count_plus_one = texelFetch(triangle_vertices_texture, ivec2(0, mc_index), 0).r;

	for (int vertex_index = 1; vertex_index < vertex_count_plus_one;)
	{
		vertex_id = texelFetch(triangle_vertices_texture, ivec2(vertex_index, mc_index), 0).r;
		vertex_position3 = texelFetch(vertices_texture, texture_positions[vertex_id], 0).xyz;
		gl_Position = pvm_matrix*vec4(vertex_position3, 1.0);
		frag_position3 = vec3(view_model_matrix*vec4(vertex_position3, 1.0));
		frag_normal3 = view_model_normal_matrix3*texelFetch(normals_texture, texture_positions[vertex_id], 0).xyz;
		EmitVertex();
		vertex_index += 1;

		vertex_id = texelFetch(triangle_vertices_texture, ivec2(vertex_index, mc_index), 0).r;
		vertex_position3 = texelFetch(vertices_texture, texture_positions[vertex_id], 0).xyz;
		gl_Position = pvm_matrix*vec4(vertex_position3, 1.0);
		frag_position3 = vec3(view_model_matrix*vec4(vertex_position3, 1.0));
		frag_normal3 = view_model_normal_matrix3*texelFetch(normals_texture, texture_positions[vertex_id], 0).xyz;
		EmitVertex();
		vertex_index += 1;

		vertex_id = texelFetch(triangle_vertices_texture, ivec2(vertex_index, mc_index), 0).r;
		vertex_position3 = texelFetch(vertices_texture, texture_positions[vertex_id], 0).xyz;
		gl_Position = pvm_matrix*vec4(vertex_position3, 1.0);
		frag_position3 = vec3(view_model_matrix*vec4(vertex_position3, 1.0));
		frag_normal3 = view_model_normal_matrix3*texelFetch(normals_texture, texture_positions[vertex_id], 0).xyz;
		EmitVertex();
		vertex_index += 1;

		EndPrimitive();
	}
}
