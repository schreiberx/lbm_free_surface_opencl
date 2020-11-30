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


#ifndef CGLEXPANDABLEVIEWBOX_HPP
#define CGLEXPANDABLEVIEWBOX_HPP

#include "libmath/CVector.hpp"
#include "libmath/CMatrix.hpp"
#include "libmath/CGlSlMath.hpp"
#include "libgl/CGlProjectionMatrix.hpp"

/**
 * Expand a renderbox by points at its min and max limits
 *
 * The renderbox is initialized with the projection and view matrix to project the points
 * to the near plane. After projection, the field of view angle can be expanded by scaling
 * the x and y component to achieve full coverage of the desired points. points origin
 * e. g. from box corners.
 */
class CGlExpandableViewBox
{
private:
	CGlVertexArrayObject vao;
	CGlBuffer vertex_buffer;
	CGlBuffer index_buffer;

	bool verbose;

	GLSL::mat4 in_projection_matrix;		///< projection matrix
	GLSL::mat4 in_view_matrix;				///< view matrix
	GLSL::mat4 in_proj_view_matrix;			///< projection*view matrix

	GLSL::mat4 inv_projection_matrix;		///< inverse of in_projection_matrix
	GLSL::mat4 inv_view_matrix;				///< inverse of in_view_matrix

public:
	CGlProjectionMatrix cGlProjectionMatrix;	///< for unprojection of the input projection matrix

	float left_projected_plane;				///< left plane for frustum (-1,1,-1,1,1,..)
	float right_projected_plane;			///< right plane for frustum (-1,1,-1,1,1,..)
	float top_projected_plane;				///< top plane for frustum (-1,1,-1,1,1,..)
	float bottom_projected_plane;			///< bottom plane for frustum (-1,1,-1,1,1,..)
	float min_near_plane;					///< minimum z component for final frustum matrix
	float max_far_plane;					///< maximum z components for final frustum matrix

	float lsb_frustum_left;				///< left plane in lsb frustum
	float lsb_frustum_right;			///< right plane in lsb frustum
	float lsb_frustum_top;				///< top plane in lsb frustum
	float lsb_frustum_bottom;			///< bottom plane in lsb frustum
	float lsb_frustum_near_plane;		///< near plane in lsb frustum
	float lsb_frustum_far_plane;		///< far plane in lsb frustum

	GLSL::mat4 lsb_projection_matrix;		///< projection matrix to render from light space box
	GLSL::mat4 lsb_view_matrix;		///< view matrix to render from light space box


	/**
	 * initialize the expandable view box
	 */
	CGlExpandableViewBox(	bool p_verbose = false	///< turn on/off verbose mode
			)	:
			vertex_buffer(GL_ARRAY_BUFFER),
			index_buffer(GL_ELEMENT_ARRAY_BUFFER),
			verbose(p_verbose)
	{
		vao.bind();


			static const GLubyte indices[16] = {
					// faces for clockwise triangle strips
					0,1,3,2,0,	// left
					4,6,2,6,	// front
					7,5,4,		// right
					5,1,3,7		// back
			};

			index_buffer.bind();
			index_buffer.data(sizeof(indices), indices);

			vertex_buffer.bind();
			vertex_buffer.resize(sizeof(GLfloat)*(3*8));
			glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, NULL);
			glEnableVertexAttribArray(0);

		vao.bind();
	}

	/**
	 * prepare the new viewbox
	 */
	void setup(
			const GLSL::mat4	&p_projection_matrix,	///< projection matrix
			const GLSL::mat4	&p_view_matrix			///< view matrix
			)
	{
		in_projection_matrix = p_projection_matrix;
		in_view_matrix = p_view_matrix;
		in_proj_view_matrix = in_projection_matrix * in_view_matrix;

		inv_projection_matrix = GLSL::inverse(in_projection_matrix);
		inv_view_matrix = GLSL::inverse(in_view_matrix);

		bottom_projected_plane = CMath::numeric_inf<float>();
		top_projected_plane = -CMath::numeric_inf<float>();
		left_projected_plane = CMath::numeric_inf<float>();
		right_projected_plane = -CMath::numeric_inf<float>();

		min_near_plane = CMath::numeric_inf<float>();
		max_far_plane = 0;

		cGlProjectionMatrix.setup(in_projection_matrix);
		cGlProjectionMatrix.unproject();
	}


	/**
	 * expand the viewbox if rendering point to screenspace and if the point is out of the screen space
	 */
	void expandWithPoint(
			const GLSL::vec3	&point
		)
	{
		GLSL::vec4 view_point = in_view_matrix * GLSL::vec4(point.data[0], point.data[1], point.data[2], 1);

		min_near_plane = CMath::min<float>(min_near_plane, -view_point[2]);
		max_far_plane = CMath::max<float>(max_far_plane, -view_point[2]);

		view_point[0] /= view_point[2];
		view_point[1] /= view_point[2];

		left_projected_plane = CMath::min<float>(left_projected_plane, view_point[0]);
		right_projected_plane = CMath::max<float>(right_projected_plane, view_point[0]);

		bottom_projected_plane = CMath::min<float>(bottom_projected_plane, view_point[1]);
		top_projected_plane = CMath::max<float>(top_projected_plane, view_point[1]);
	}



private:
	/**
	 * divide first 3 components of v by 4th component and store vector to dest_vec
	 */
	inline void a(
			GLSL::vec4 v,
			GLfloat *dest_vec
		)
	{
		v.data[0] /= v.data[3];
		v.data[1] /= v.data[3];
		v.data[2] /= v.data[3];

		dest_vec[0] = v.data[0];
		dest_vec[1] = v.data[1];
		dest_vec[2] = v.data[2];
	}

public:
	/**
	 * draw the box of the covered volume by the scaled projection and view matrix
	 *
	 * this has been tested only for frustum projection matrices!
	 */
	void emitBoxVertices()
	{		// prepare vertex array
		GLfloat vertices[8][3];

		GLSL::mat4 m = GLSL::inverse(lsb_projection_matrix*lsb_view_matrix);

		a(	m * GLSL::vec3(-1, -1, 1),	vertices[0]	);
		a(	m * GLSL::vec3(-1, -1, -1),	vertices[1]	);
		a(	m * GLSL::vec3(-1, 1, 1),	vertices[2]	);
		a(	m * GLSL::vec3(-1, 1, -1),	vertices[3]	);
		a(	m * GLSL::vec3(1, -1, 1),	vertices[4]	);
		a(	m * GLSL::vec3(1, -1, -1),	vertices[5]	);
		a(	m * GLSL::vec3(1, 1, 1),	vertices[6]	);
		a(	m * GLSL::vec3(1, 1, -1),	vertices[7]	);

		vao.bind();
			vertex_buffer.bind();
			vertex_buffer.subData(0, sizeof(GLfloat)*(3*8), vertices);

			glDrawElements(GL_LINE_STRIP, 16, GL_UNSIGNED_BYTE, 0);
		vao.unbind();
	}


	/**
	 * compute projection and view matrix (lsb_projection_matrix, lsb_view_matrix) for rendering in light space box
	 */
	void computeMatrices()
	{
		lsb_view_matrix = in_view_matrix;

		float max_left_right_plane = CMath::max<float>(CMath::abs<float>(left_projected_plane), CMath::abs<float>(right_projected_plane))*min_near_plane;
		float max_bottom_top_plane = CMath::max<float>(CMath::abs<float>(bottom_projected_plane), CMath::abs<float>(top_projected_plane))*min_near_plane;

		// scale to same resolution in x and y direction
		if (max_left_right_plane < max_bottom_top_plane)
			max_left_right_plane = max_bottom_top_plane;
		else
			max_bottom_top_plane = max_left_right_plane;

		lsb_frustum_left = -max_left_right_plane;
		lsb_frustum_right = max_left_right_plane;
		lsb_frustum_bottom = -max_bottom_top_plane;
		lsb_frustum_top = max_bottom_top_plane;
		lsb_frustum_near_plane = min_near_plane;
		lsb_frustum_far_plane = max_far_plane;

		lsb_projection_matrix = GLSL::frustum(	lsb_frustum_left,
												lsb_frustum_right,
												lsb_frustum_bottom,
												lsb_frustum_top,
												lsb_frustum_near_plane,
												lsb_frustum_far_plane
											);
	}
};

#endif
