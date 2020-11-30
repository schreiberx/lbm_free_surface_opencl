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


/*
 * CGlProjectionMatrix.hpp
 *
 *  Created on: Mar 25, 2010
 *      Author: martin
 */

#ifndef CGLPROJECTIONMATRIX_HPP_
#define CGLPROJECTIONMATRIX_HPP_

#include "libmath/CGlSlMath.hpp"

/**
 * features for projection matrix - e.g. unfrustum
 */
class CGlProjectionMatrix
{
public:

	// recomputed values of projection matrix
	float frustum_left, frustum_right, frustum_top, frustum_bottom;
	float frustum_near_plane, frustum_far_plane;

	GLSL::mat4 projection_matrix;

	void setup(	GLSL::mat4 &p_projection_matrix)
	{
		projection_matrix = p_projection_matrix;
	}

	/**
	 * unproject in_projection_matrix
	 */
	void unproject()
	{
		GLSL::mat4 inv_projection_matrix = GLSL::inverse(projection_matrix);

		// compute z component for near and far plane (usually negative)
		frustum_near_plane	= -(-inv_projection_matrix[2][2]+inv_projection_matrix[2][3])/(-inv_projection_matrix[3][2]+inv_projection_matrix[3][3]);
		frustum_far_plane	= -(inv_projection_matrix[2][2]+inv_projection_matrix[2][3])/(inv_projection_matrix[3][2]+inv_projection_matrix[3][3]);

		frustum_left = (-inv_projection_matrix[0][0]-inv_projection_matrix[0][2]+inv_projection_matrix[0][3])/(-inv_projection_matrix[3][0]-inv_projection_matrix[3][2]+inv_projection_matrix[3][3]);
		frustum_right = (inv_projection_matrix[0][0]-inv_projection_matrix[0][2]+inv_projection_matrix[0][3])/(inv_projection_matrix[3][0]-inv_projection_matrix[3][2]+inv_projection_matrix[3][3]);
		frustum_bottom = (-inv_projection_matrix[1][1]-inv_projection_matrix[1][2]+inv_projection_matrix[1][3])/(-inv_projection_matrix[3][1]-inv_projection_matrix[3][2]+inv_projection_matrix[3][3]);
		frustum_top = (inv_projection_matrix[1][1]-inv_projection_matrix[1][2]+inv_projection_matrix[1][3])/(inv_projection_matrix[3][1]-inv_projection_matrix[3][2]+inv_projection_matrix[3][3]);
	}
};

#endif /* CGLPROJECTIONMATRIX_HPP_ */
