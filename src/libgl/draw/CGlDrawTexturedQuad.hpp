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


#ifndef C_GL_DRAW_TEXTURED_QUAD_HPP
#define C_GL_DRAW_TEXTURED_QUAD_HPP

#include "libgl/incgl3.h"
#include "libgl/core/CGlProgram.hpp"
#include "libgl/core/CGlTexture.hpp"
#include "libmath/CGlSlMath.hpp"
#include "lib/CError.hpp"

/**
 * \brief draw a quad texturized with the 2d texture from "data/textures/img_8598.jpg"
 */
class CGlDrawTexturedQuad
{
	CGlProgram program;

	CGlUniform pvm_matrix_uniform;

public:
	CGlTexture texture;	///< texture handler
	CGlVertexArrayObject vao;
	CError error;		///< error handler
	CGlBuffer buffer;

	CGlDrawTexturedQuad()
	{
		program.initVertFragShadersFromDirectory("draw/textured_quad");

		program.bindAttribLocation(0, "vertex_position");

		program.link();

		program.setupUniform(pvm_matrix_uniform, "pvm_matrix");


		static const float vertices[4][4] = {
					{-1.0, -1.0, -1.0, 1.0},
					{ 1.0, -1.0, -1.0, 1.0},
					{-1.0,  1.0, -1.0, 1.0},
					{ 1.0,  1.0, -1.0, 1.0},
				};

		static const float texcoords[4][2] = {
					{0.0, 1.0},
					{1.0, 1.0},
					{0.0, 0.0},
					{1.0, 0.0},
				};

		vao.bind();

			buffer.bind();
			buffer.resize(sizeof(vertices)+sizeof(texcoords), GL_DYNAMIC_DRAW);
			buffer.subData(0, sizeof(vertices), vertices);
			buffer.subData(sizeof(vertices), sizeof(texcoords), texcoords);

			glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, 0);
			glEnableVertexAttribArray(0);
			glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, (void*)sizeof(vertices));
			glEnableVertexAttribArray(1);

		vao.unbind();

		CGlErrorCheck();
	}


	/**
	 * render texturized quad
	 */
	void render(const GLSL::mat4 &pvm_matrix, CGlTexture &p_texture)
	{
		vao.bind();
		CGlErrorCheck();

		program.use();
		pvm_matrix_uniform.set(pvm_matrix);
		glDisable(GL_CULL_FACE);
		glDisable(GL_DEPTH_TEST);

		p_texture.bind();
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
		p_texture.unbind();
		CGlErrorCheck();

		glEnable(GL_CULL_FACE);
		glEnable(GL_DEPTH_TEST);

		vao.unbind();
		CGlErrorCheck();
	}

	/**
	 * render texturized quad
	 */
	void render(const GLSL::mat4 &pvm_matrix	///< pvm matrix to render quad
	)
	{
		render(pvm_matrix, texture);
	}


	/**
	 * render slice of 3d volume texture
	 */
	void renderVolume(	GLSL::mat4 &pvm_matrix,	///< pvm matrix to render quad
						CGlTexture &p_texture	///< 3d texture
	)
	{
		float vertices[4][4] = {
					{-1.0, -1.0, 0.0, 1.0},
					{ 1.0, -1.0, 0.0, 1.0},
					{-1.0,  1.0, 0.0, 1.0},
					{ 1.0,  1.0, 0.0, 1.0},
				};

		float texcoords[4][3] = {
					{0.0, 0.0, 0.0},
					{1.0, 0.0, 0.0},
					{0.0, 1.0, 1.0},
					{1.0, 1.0, 1.0},
				};

		p_texture.bind();

		// index, size, type, normalized, stride, pointer
		glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, vertices);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, texcoords);
		CGlErrorCheck();

		glEnableVertexAttribArray(0);
		glEnableVertexAttribArray(1);
		CGlErrorCheck();

		program.use();
		pvm_matrix_uniform.set(pvm_matrix);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

		glDisableVertexAttribArray(0);
		glDisableVertexAttribArray(1);

		p_texture.unbind();

		CGlErrorCheck();
	}
};

#endif	/* C_GL_DRAW_TEXTURE_QUAD_HPP */
