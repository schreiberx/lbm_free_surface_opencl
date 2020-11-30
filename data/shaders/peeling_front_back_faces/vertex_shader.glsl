// use opengl 3.2 core version!


in vec4 vertex_position;
in vec3 vertex_normal;

uniform mat4 pvm_matrix;
uniform mat4 view_model_matrix;
uniform mat3 view_model_normal_matrix3;


out vec3 out_vertex_normal;
out vec3 out_vertex_position;


void main(void)
{
	gl_Position = pvm_matrix*vertex_position;

	out_vertex_normal = view_model_normal_matrix3*vertex_normal;

	vec4 v = view_model_matrix*vertex_position;
	out_vertex_position = v.xyz / v.w;	// usually dividing by .w is not necessary for view_model matrices (not homogenous component used)
}
