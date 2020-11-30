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


#ifndef C_GL_DRAW_RECTANGLE_HPP
#define C_GL_DRAW_RECTANGLE_HPP

#include "libgl/incgl3.h"
#include "libgl/core/CGlProgram.hpp"
#include "libgl/core/CGlTexture.hpp"
#include "libmath/CGlSlMath.hpp"
#include "lib/CError.hpp"

/**
 * \brief	render a quad texturized with a rectangle texture
 */
class CGlDrawRectangle
{
	CGlProgram draw_slice_program;
	CGlUniform pvm_matrix_uniform;
	CGlTexture texture;

public:
	CError error;	///< error handler

	/**
	 * default constructor
	 */
	CGlDrawRectangle()
	{
		draw_slice_program.initVertFragShadersFromDirectory("draw/rectangle");

		draw_slice_program.bindAttribLocation(0, "vertex_position");

		draw_slice_program.link();

		draw_slice_program.bindAttribLocation(0, "vertex_position");
		draw_slice_program.bindAttribLocation(1, "vertex_texture_coord");

		draw_slice_program.use();
		draw_slice_program.setUniform1i("slice_rectangle", 0);
		draw_slice_program.disable();

		// out_color is also bound automatically, but we do it here again ^^
		draw_slice_program.bindFragDataLocation(0, "out_color");

		draw_slice_program.setupUniform(pvm_matrix_uniform, "pvm_matrix");

		texture.loadFromFile("data/textures/img_8598.jpg");
	}

	/**
	 * render texturized quad
	 */
	void render(	GLSL::mat4 &pvm_matrix,			///< pvm matrix
					CGlTexture &p_texture			///< rectangle texture
	)
	{
		float vertices[4][4] = {
					{-1.0, -1.0, 0.0, 1.0},
					{ 1.0, -1.0, 0.0, 1.0},
					{-1.0,  1.0, 0.0, 1.0},
					{ 1.0,  1.0, 0.0, 1.0},
				};

		float texcoords[4][2] = {
					{0.0, 0.0},
					{(float)p_texture.width, 0.0},
					{0.0, (float)p_texture.height},
					{(float)p_texture.width, (float)p_texture.height},
				};

		p_texture.bind();

		// index, size, type, normalized, stride, pointer
		glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, vertices);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, texcoords);
		CGlErrorCheck();

		glEnableVertexAttribArray(0);
		glEnableVertexAttribArray(1);
		CGlErrorCheck();

		draw_slice_program.use();
		pvm_matrix_uniform.set(pvm_matrix);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
		draw_slice_program.disable();

		glDisableVertexAttribArray(1);
		glDisableVertexAttribArray(0);

		CGlErrorCheck();
	}
};

#endif
