// use opengl 3.2 core version!


in int in_index;
out int index;

void main(void)
{
	// nothing to do here
	// we get the vertex (point) id by using gl_PrimitiveIDIn

	index = in_index;
}

