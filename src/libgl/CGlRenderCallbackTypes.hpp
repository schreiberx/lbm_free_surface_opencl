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
 * CGlRenderCallbackTypes.hpp
 *
 *  Created on: Mar 20, 2010
 *      Author: martin
 */

#ifndef CGL_RENDER_CALLBACK_TYPES_HPP_
#define CGL_RENDER_CALLBACK_TYPES_HPP_

/**
 * class with a bunch of typedefs for standardization of render callbacks necessary e.g. to create depth peelings
 */
class CGlRenderCallbackTypes
{
public:
	/**
	 * callback type for rendering pass to setup view and model matrix
	 */
	typedef 	void (setup_view_model_matrix)(
											void *base_class,								///< pointer to this class
											const GLSL::mat4 &pvm_matrix,					///< projection,view,model matrix
											const GLSL::mat4 &view_model_matrix,				///< view-model matrix
											const GLSL::mat3 &view_model_normal_matrix3		///< normal matrix
											);

	/**
	 * callback type for calling the rendering pass.
	 *
	 * this interface has to be implemented by the rendering pass.
	 */
	typedef 	void (call_render_pass)(
									const GLSL::mat4 &projection_matrix,
									const GLSL::mat4 &view_matrix,
									CGlRenderCallbackTypes::setup_view_model_matrix *callback,	// callback function
									void *callback_parameter,							// parameter for callback function
									void *user_data
								);
};

#endif /* CGLRENDERCALLBACKS_HPP_ */
