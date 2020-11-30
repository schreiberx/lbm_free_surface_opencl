// use opengl 3.2 core shaders


out vec4 frag_data;
in float point_alpha;

//in vec3 asdf;

void main(void)
{
#if 1
	// this version is 0.005 times faster than the 3rd one :)
	float a = length(gl_PointCoord*2.0-1.0);
	if (a > 1.0)	discard;
	frag_data = vec4(1.0, 1.0, 1.0, (1.0-a)*point_alpha);

#elif 1
	vec2 a = gl_PointCoord*2.0-1.0;
	a.x = a.x*a.x+a.y*a.y;
	if (a.x > 1.0)	discard;
	frag_data = vec4(1.0, 1.0, 1.0, (1.0-sqrt(a.x))*point_alpha);

#else
	// use max(alpha, 0) - otherwise we'll produce negative alpha values
	frag_data = vec4(1.0, 1.0, 1.0, max((1.0-length(gl_PointCoord*2.0-1.0))*point_alpha, 0));
#endif
}
