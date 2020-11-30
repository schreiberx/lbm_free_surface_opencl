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


#ifndef C_GL_DRAW_TEXTURED_ARRAY_QUAD_HPP
#define C_GL_DRAW_TEXTURED_ARRAY_QUAD_HPP

#include "libgl/incgl3.h"
#include "libgl/core/CGlProgram.hpp"
#include "libgl/core/CGlTexture.hpp"
#include "libmath/CGlSlMath.hpp"
#include "lib/CError.hpp"

/**
 * \brief draw a quad texturized with the 2d texture ARRAY
 */
class CGlDrawTexturedArrayQuad
{
	CGlProgram program;

	CGlUniform pvm_matrix_uniform;
	CGlUniform texture_array_layer_uniform;

public:
	CError error;		///< error handler

	CGlDrawTexturedArrayQuad()
	{
		program.initVertFragShadersFromDirectory("draw/textured_array_quad");

		program.bindAttribLocation(0, "vertex_position");

		program.link();

		program.setupUniform(pvm_matrix_uniform, "pvm_matrix");
		program.setupUniform(texture_array_layer_uniform, "texture_array_layer");
	}


	/**
	 * render texturized quad
	 */
	void render(	GLSL::mat4 &pvm_matrix,	///< pvm matrix
					CGlTexture &p_texture,	///< texture for quad texturing
					int layer				///< layer of texture array
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
					{1.0, 0.0},
					{0.0, 1.0},
					{1.0, 1.0},
				};

		program.use();
		pvm_matrix_uniform.set(pvm_matrix);
		texture_array_layer_uniform.set1i(layer);

		// index, size, type, normalized, stride, pointer
		glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, vertices);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, texcoords);
		CGlErrorCheck();

		glEnableVertexAttribArray(0);
		glEnableVertexAttribArray(1);
		CGlErrorCheck();

		p_texture.bind();
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
		p_texture.unbind();

		glDisableVertexAttribArray(1);
		glDisableVertexAttribArray(0);

		program.disable();

		CGlErrorCheck();

		glFlush();
	}
};

#endif	/* C_GL_DRAW_TEXTURE_QUAD_HPP */
