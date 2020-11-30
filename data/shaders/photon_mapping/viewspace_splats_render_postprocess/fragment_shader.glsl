// use opengl 3.2 core shaders


out vec4 frag_data;
uniform sampler2D photon_blend_texture;

void main(void)
{
	frag_data = vec4(1, 1, 1, texelFetch(photon_blend_texture, ivec2(gl_FragCoord), 0).r);
}
