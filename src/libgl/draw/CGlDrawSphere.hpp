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


#ifndef CGL_DRAW_SPHERE_HPP
#define CGL_DRAW_SPHERE_HPP

#include "libgl/core/CGlProgram.hpp"
#include "libgl/core/CGlBuffer.hpp"
#include "lib/CError.hpp"
#include "libmath/CGlSlMath.hpp"

/**
 * \brief	create and render a sphere with polygons
 */
class CGlDrawSphere
{
	CGlBuffer vertex_buffer;
	CGlBuffer texture_coord_buffer;
	CGlBuffer index_buffer;
	CGlVertexArrayObject vao;

	int triangle_strip_indices_count;

public:
	CError error;		///< error handler

	/**
	 * initialize OpenGL buffers to render a sphere with the given parameters
	 */
	void initBuffers(	GLuint hsegments = 30,	///< number of horizontal segments
						GLuint vsegments = 15	///< number of vertical segments
	)
	{
		triangle_strip_indices_count = (hsegments+1)*2*vsegments;
		GLuint vertices_count = hsegments*(vsegments+1);

		GLfloat *vertices = new GLfloat[vertices_count*3];
		GLfloat *texture_coords = new GLfloat[vertices_count*2];
		GLuint *triangle_strip_indices = new GLuint[triangle_strip_indices_count];


		// create triangle indices
		GLuint *t = triangle_strip_indices;
		int count = 0;
		for (GLuint dbeta = 0; dbeta < vsegments; dbeta++)
		{
			for (GLuint dalpha = 0; dalpha < hsegments; dalpha++)
			{
				*t = dalpha+dbeta*hsegments;
				t++;
				*t = dalpha+(dbeta+1)*hsegments;
				t++;

				count += 2;
			}

			*t = dbeta*hsegments;
			t++;
			*t = (dbeta+1)*hsegments;
			t++;
			count += 2;
		}

		// create vertex data
		GLfloat *v = vertices;
		GLfloat *tc = texture_coords;
		int vcount = 0;
		for (GLuint dbeta = 0; dbeta <= vsegments; dbeta++)
		{
			float y = ((float)dbeta/(float)vsegments) * M_PI;

			for (GLuint dalpha = 0; dalpha < hsegments; dalpha++)
			{
				float r = ((float)dalpha/(float)hsegments) * 2.0 * M_PI;

				float scale = CMath::sin<float>(y);
				v[0] = CMath::sin<float>(r)*scale;
				v[1] = -CMath::cos<float>(y);
				v[2] = -CMath::cos<float>(r)*scale;
				v += 3;

				tc[0] = (float)dalpha/(float)hsegments;
				tc[1] = (float)dbeta/(float)vsegments;
				tc += 2;

				vcount ++;
			}
		}

		GLuint *pt = triangle_strip_indices;
		for (int i = 0; i < triangle_strip_indices_count; i++)
		{
			if (*pt >= vertices_count)
				std::cerr << "ERROR" << std::endl;
			pt++;
		}

		vao.bind();

			vertex_buffer.bind();
			vertex_buffer.data(vertices_count*3*sizeof(GLfloat), vertices);
			glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, NULL);
			glEnableVertexAttribArray(0);

			glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, NULL);
			glEnableVertexAttribArray(1);

			texture_coord_buffer.bind();
			texture_coord_buffer.data(vertices_count*2*sizeof(GLfloat), texture_coords);
			glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 0, NULL);
			glEnableVertexAttribArray(2);

			index_buffer.bind();
			index_buffer.data((triangle_strip_indices_count)*sizeof(GLuint), triangle_strip_indices);

		vao.unbind();

		delete[] vertices;
		delete[] texture_coords;
		delete[] triangle_strip_indices;
	}


	/**
	 * initialize the buffers to render a sphere
	 */
	CGlDrawSphere(	GLuint hsegments,	///< number of horizontal segments
						GLuint vsegments	///< number of vertical segments
	)	:
		vertex_buffer(GL_ARRAY_BUFFER),
		texture_coord_buffer(GL_ARRAY_BUFFER),
		index_buffer(GL_ELEMENT_ARRAY_BUFFER)
	{
		initBuffers(hsegments, vsegments);
	}

	CGlDrawSphere()	:
		vertex_buffer(GL_ARRAY_BUFFER),
		texture_coord_buffer(GL_ARRAY_BUFFER),
		index_buffer(GL_ELEMENT_ARRAY_BUFFER)
	{
	}

	/**
	 * initialize buffers to render sphere with given parameters
	 */
	void initSphere(	GLuint hsegments = 30,	///< number of horizontal segments
						GLuint vsegments = 15	///< number of vertical segments
	)
	{
		initBuffers(hsegments, vsegments);
	}

	/**
	 * render the sphere without using a GLSL program
	 */
	void renderWithoutProgram()
	{
		vao.bind();
		glDrawElements(GL_TRIANGLE_STRIP, triangle_strip_indices_count, GL_UNSIGNED_INT, 0);
		vao.unbind();

		CGlErrorCheck();
	}
};

#endif
