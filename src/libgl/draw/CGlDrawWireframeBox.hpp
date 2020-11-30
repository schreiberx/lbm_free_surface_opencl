/*
 * Copyright 2010 Martin Schreiber
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */


#ifndef CGL_WIREFRAME_BOX_HPP
#define CGL_WIREFRAME_BOX_HPP

#include "libgl/core/CGlState.hpp"

/**
 * \brief	render sky boxes with cube map textures
 */
class CGlWireframeBox
{
	CGlBuffer vertex_buffer;
	CGlBuffer index_buffer;

public:
	CError error;								///< error handler
	CGlUniform pvm_matrix_uniform;	///< uniform to pvm matrix

	CGlProgram program;				///< program to render sky box

	CGlWireframeBox()	:
			vertex_buffer(GL_ARRAY_BUFFER),
			index_buffer(GL_ELEMENT_ARRAY_BUFFER)
	{
		program.initVertFragShadersFromDirectory("draw/wireframe_box");
		program.link();

		program.setupUniform(pvm_matrix_uniform, "pvm_matrix");

		/**
		 * initialize buffers
		 */
		/*
		 * vertices for cube drawn counterclockwise
		 * use quads to draw surfaces
		 */
#define P	+1.0f
#define N	-1.0f
		static const GLfloat vertices[8][3] = {
				{N,N,P},	// 0
				{N,N,N},	// 1
				{N,P,P},	// 2
				{N,P,N},	// 3
				{P,N,P},	// 4
				{P,N,N},	// 5
				{P,P,P},	// 6
				{P,P,N},	// 7
			};

#undef N
#undef P

		static const GLubyte indices[16] = {
				// faces for clockwise triangle strips
				0,1,3,2,0,	// left
				4,6,2,6,	// front
				7,5,4,		// right
				5,1,3,7		// back
		};

		vertex_buffer.bind();
		vertex_buffer.data(sizeof(vertices), vertices);
		vertex_buffer.unbind();

		index_buffer.bind();
		index_buffer.data(sizeof(indices), indices);
		index_buffer.unbind();
	}

	/**
	 * render skybox
	 */
	void render(GLSL::mat4 &p_pvm_matrix)
	{
		program.use();
		pvm_matrix_uniform.set(p_pvm_matrix);

		vertex_buffer.bind();
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, NULL);
		glEnableVertexAttribArray(0);

		index_buffer.bind();
		glDrawElements(GL_LINE_STRIP, 16, GL_UNSIGNED_BYTE, 0);
		index_buffer.unbind();

		glDisableVertexAttribArray(0);
		vertex_buffer.unbind();
		program.disable();
	}
};

#endif
