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


#ifndef CGL_SKY_BOX_HPP
#define CGL_SKY_BOX_HPP

#include "libgl/core/CGlState.hpp"

/**
 * \brief	render sky boxes with cube map textures
 */
class CGlDrawSkyBox
{
	CGlBuffer vertex_buffer;
	CGlBuffer index_buffer;
	CGlVertexArrayObject vao;

public:
	CError error;					///< error handler
	CGlTexture texture_cube_map;	///< cube map texture
	CGlUniform pvm_matrix_uniform;	///< uniform to pvm matrix

	CGlProgram program;				///< program to render sky box

	CGlDrawSkyBox()	:
			vertex_buffer(GL_ARRAY_BUFFER),
			index_buffer(GL_ELEMENT_ARRAY_BUFFER),
			texture_cube_map(GL_TEXTURE_CUBE_MAP)
	{
		program.initVertFragShadersFromDirectory("draw/skybox");

		program.link();

		program.setupUniform(pvm_matrix_uniform, "pvm_matrix");
		program.use();
		program.setUniform1i("sampler_cube", 0);
		program.disable();

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

#undef N
#undef P
#if 0
		static const GLubyte indices[20] = {
#if 0
				// faces for counter clockwise triangle strips
				4,6,0,2,	// front
				1,3,		// left
				5,7,		// back
				4,6,		// right
				6,			// > ZERO triangle
				7,2,3,		// top
				3,1,		// > ZERO triangle
				1,5,0,4		// bottom
#else
				// faces for clockwise triangle strips
				6,4,2,0,	// front
				3,1,		// left
				7,5,		// back
				6,4,		// right
				4,			// > ZERO triangle
				5,0,1,		// bottom
				1,3,		// > ZERO triangle
				3,7,2,6,		// top
#endif
		};
#else
		static const GLubyte indices[20] = {
#if 0
				// faces for counter clockwise triangle strips
				4,6,0,2,	// front
				1,3,		// left
				5,7,		// back
				4,6,		// right
				6,			// > ZERO triangle
				7,2,3,		// top
				3,1,		// > ZERO triangle
				1,5,0,4		// bottom
#else
				// faces for clockwise triangle strips
				6,4,2,0,	// front
				3,1,		// left
				7,5,		// back
				6,4,		// right
				4,			// > ZERO triangle
				5,0,1,		// bottom
				1,3,		// > ZERO triangle
				3,7,2,6,		// top
#endif
		};
#endif
		vao.bind();

			vertex_buffer.bind();
			vertex_buffer.data(sizeof(vertices), vertices);
			glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, NULL);
			glEnableVertexAttribArray(0);

			index_buffer.bind();
			index_buffer.data(sizeof(indices), indices);

		vao.unbind();
	}

	/**
	 * initialize skybox texture with files give as parameters
	 */
	void initWithTextures(	const char *file_pos_x,
							const char *file_neg_x,
							const char *file_pos_y,
							const char *file_neg_y,
							const char *file_pos_z,
							const char *file_neg_z
				)
	{
		texture_cube_map.bind();
		texture_cube_map.loadFromFile(file_pos_x, GL_TEXTURE_CUBE_MAP_POSITIVE_X);
		texture_cube_map.loadFromFile(file_neg_x, GL_TEXTURE_CUBE_MAP_NEGATIVE_X);
		texture_cube_map.loadFromFile(file_pos_y, GL_TEXTURE_CUBE_MAP_POSITIVE_Y);
		texture_cube_map.loadFromFile(file_neg_y, GL_TEXTURE_CUBE_MAP_NEGATIVE_Y);
		texture_cube_map.loadFromFile(file_pos_z, GL_TEXTURE_CUBE_MAP_POSITIVE_Z);
		texture_cube_map.loadFromFile(file_neg_z, GL_TEXTURE_CUBE_MAP_NEGATIVE_Z);
		texture_cube_map.unbind();

		CError_Append(texture_cube_map);
	}

	/**
	 * render skybox
	 */
	void renderWithProgram(GLSL::mat4 &p_pvm_matrix)
	{
//		CGlStateDisable depth_test(GL_DEPTH_TEST);
CGlErrorCheck();
		program.use();
		pvm_matrix_uniform.set(p_pvm_matrix);

		vao.bind();
		CGlErrorCheck();

		texture_cube_map.bind();
		glDrawElements(GL_TRIANGLE_STRIP, 20, GL_UNSIGNED_BYTE, 0);

		vao.unbind();

		program.disable();
		CGlErrorCheck();
	}
};

#endif
