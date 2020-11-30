// use opengl 3.2 core shaders


in vec3 frag_normal3;
in vec3 eye_direction3;

uniform samplerCube sampler;

uniform vec4 light_pos = vec4(1,1,1,0);
uniform vec3 light_color3 = vec3(1,1,1);

out vec4 frag_data;
uniform mat3 normal_matrix3;
uniform mat3 transp_view_normal_matrix3;
uniform mat4 view_matrix;

void main(void)
{
	vec3 normal3 = normalize(frag_normal3);
	vec3 reflected_vector3 = reflect(normalize(eye_direction3), normal3);

	float light = 0.2f + 0.8f*max(0.0f, dot(normal3, normalize(vec3(view_matrix*light_pos))));
	frag_data = texture(sampler, transp_view_normal_matrix3*reflected_vector3)*light;
}
