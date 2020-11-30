// use opengl 3.2 core version!


in int in_index;

uniform sampler2D vertex_array_texture;
uniform sampler2D normal_array_texture;

uniform mat4 pvm_matrix;
uniform mat3 pvm_normal_matrix3;
uniform mat4 view_model_matrix;
uniform mat3 view_model_normal_matrix3;

out vec3 frag_position3;
out vec3 frag_normal3;

void main(void)
{
	ivec2 vertex_array_texture_pos = ivec2(	in_index % VERTEX_ARRAY_TEXTURE_WIDTH,
											in_index / VERTEX_ARRAY_TEXTURE_WIDTH);

	vec4 pos = vec4(texelFetch(vertex_array_texture, vertex_array_texture_pos, 0).rgb, 1);

	gl_Position = pvm_matrix*pos;

	frag_position3 = vec3(view_model_matrix*pos);
	frag_normal3 = view_model_normal_matrix3*texelFetch(normal_array_texture, vertex_array_texture_pos, 0).rgb;
}
