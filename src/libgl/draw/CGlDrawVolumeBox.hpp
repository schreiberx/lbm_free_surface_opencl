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


#ifndef CGL_BOX_HPP
#define CGL_BOX_HPP

#include "libgl/core/CGlProgram.hpp"
#include "libgl/core/CGlBuffer.hpp"
#include "libgl/core/CGlVertexArrayObject.hpp"
#include "lib/CError.hpp"
#include "libmath/CGlSlMath.hpp"

/**
 * \brief	draw a box sufficient for volume rendering
 */
class CGlDrawVolumeBox
{
	CGlProgram program;
	CGlUniform pvm_matrix_uniform;

	CGlBuffer vertex_buffer;
	CGlBuffer texture_coord_buffer;
	CGlBuffer index_buffer;

	CGlVertexArrayObject vao;


public:
	CError error;		///< error handler

	CGlDrawVolumeBox()	:
		vertex_buffer(GL_ARRAY_BUFFER),
		texture_coord_buffer(GL_ARRAY_BUFFER),
		index_buffer(GL_ELEMENT_ARRAY_BUFFER)
	{
		program.initVertFragShadersFromDirectory("draw/volume_box");
		program.bindAttribLocation(0, "vertex_position");
		program.bindAttribLocation(1, "vertex_texture_coord");

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
				{N,N,P},
				{N,N,N},
				{N,P,P},
				{N,P,N},
				{P,N,P},
				{P,N,N},
				{P,P,P},
				{P,P,N},
			};
	#undef P
	#undef N

	#define P	+1.0f
	#define N	0.0f
		static const float texture_coords[8][3] = {
				{N,N,P},
				{N,N,N},
				{N,P,P},
				{N,P,N},
				{P,N,P},
				{P,N,N},
				{P,P,P},
				{P,P,N},
			};
/*
		static const float texture_coords[8][3] = {
				{N,N,N},
				{N,N,P},
				{N,P,N},
				{N,P,P},
				{P,N,N},
				{P,N,P},
				{P,P,N},
				{P,P,P},
			};
*/
	#undef P
	#undef N

		static const GLubyte indices[20] = {
				4,6,0,2,	// front
				1,3,		// left
				5,7,		// back
				4,6,		// right
				6,			// > ZERO triangle
				7,2,3,		// top
				3,1,		// > ZERO triangle
				1,5,0,4		// bottom
		};

		vao.bind();

			vertex_buffer.bind();
			vertex_buffer.data(sizeof(vertices), vertices);
			glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, NULL);
			glEnableVertexAttribArray(0);

			texture_coord_buffer.bind();
			texture_coord_buffer.data(sizeof(texture_coords), texture_coords);
			glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, NULL);
			glEnableVertexAttribArray(1);

			index_buffer.bind();
			index_buffer.data(sizeof(indices), indices);

		vao.unbind();
	}

	/**
	 * render the volume box without using a program
	 */
	void renderWithoutProgram()
	{
		vao.bind();

		glDrawElements(GL_TRIANGLE_STRIP, 20, GL_UNSIGNED_BYTE, 0);

		vertex_buffer.unbind();
		texture_coord_buffer.unbind();

		vao.unbind();

		CGlErrorCheck();
	}

	/**
	 * render the volume box
	 *
	 * the invoking procedure has to activate the face culling with glCullFace(GL_BACK); glEnable(GL_CULL_FACE);
	 * GL_CCW (default) has to be set for front faces
	 * GL_CW has to be set to render back faces
	 *
	 * depth test is not necessary because the faces are culled
	 */
	void render(const GLSL::mat4 &pmv)
	{
		CGlProgramUse program_use(program);
		pvm_matrix_uniform.set(pmv);
		renderWithoutProgram();
	}

};

#endif
