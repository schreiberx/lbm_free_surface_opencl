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
 * CMatrices.hpp
 *
 *  Created on: Mar 24, 2010
 *      Author: martin
 */

#ifndef CMATRICES_HPP_
#define CMATRICES_HPP_

#include "libmath/CGlSlMath.hpp"

/**
 * matrices used for rendering
 */
class CMatrices
{
public:
	// those variables are initialized in each render step
	// and used by subsequent methods
	GLSL::mat4 ortho_matrix;
	GLSL::mat4 projection_matrix;
	GLSL::mat4 model_matrix;
	GLSL::mat4 view_matrix;
	GLSL::mat3 view_normal_matrix3;
	GLSL::mat4 view_model_matrix;
	GLSL::mat3 view_model_normal_matrix3;
	GLSL::mat4 pvm_matrix;

	// matrices which can be freely modified
	GLSL::mat3 p_view_normal_matrix3;
	GLSL::mat3 p_model_normal_matrix3;
	GLSL::mat3 p_view_model_normal_matrix3;

	GLSL::mat4 p_view_matrix;
	GLSL::mat4 p_view_normal_matrix;
	GLSL::mat4 p_model_matrix;
	GLSL::mat4 p_view_model_matrix;
	GLSL::mat4 p_normal_matrix;

	GLSL::mat4 p_projection_matrix;
	GLSL::mat4 p_pvm_matrix;

	GLSL::mat3 p_transposed_view_matrix3;
};

#endif /* CMATRICES_HPP_ */
