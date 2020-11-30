// use opengl 3.2 core version!


in int in_index;

uniform isampler2D vertex_array_texture;
uniform sampler2D vertices_texture;
uniform sampler2D normals_texture;

uniform mat4 pvm_matrix;
uniform mat3 pvm_normal_matrix3;
uniform mat4 view_model_matrix;
uniform mat3 view_model_normal_matrix3;

out vec3 frag_position3;
out vec3 frag_normal3;

void main(void)
{
	// texture coordinate for linear access
	ivec2 vertex_array_texture_pos = ivec2(	in_index % VERTEX_ARRAY_TEXTURE_WIDTH,
											in_index / VERTEX_ARRAY_TEXTURE_WIDTH);

	ivec2 idx = texelFetch(vertex_array_texture, vertex_array_texture_pos, 0).rg;
	vec4 pos = texelFetch(vertices_texture, idx, 0);

	gl_Position = pvm_matrix*pos;

	frag_position3 = vec3(view_model_matrix*pos);
	frag_normal3 = view_model_normal_matrix3*texelFetch(normals_texture, idx, 0).xyz;
}

