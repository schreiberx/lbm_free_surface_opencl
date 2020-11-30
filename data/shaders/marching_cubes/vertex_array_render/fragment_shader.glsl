// use opengl 3.2 core shaders

/*
in vec3 frag_coord;
in vec3 normal;
*/
out vec4 frag_data;
in vec3 frag_position3;
in vec3 frag_normal3;

vec4 blinnShading(vec4 material_color);

void main(void)
{
	frag_data = vec4(blinnShading(vec4(1.0, 1.0, 1.0, 1.0)));
}
