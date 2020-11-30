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
 * CGlDepthPeelingFrontFaces.hpp
 *
 *  Created on: Mar 25, 2010
 *      Author: martin
 */

#ifndef CGL_PEELING_FRONT_FACES_HPP_
#define CGL_PEELING_FRONT_FACES_HPP_

#include "libgl/core/CGlTexture.hpp"
#include "libgl/core/CGlFbo.hpp"
#include "libgl/core/CGlError.hpp"
#include "libgl/core/CGlState.hpp"
#include "libgl/core/CGlViewport.hpp"

#include "lib/CError.hpp"

#include "libgl/CGlRenderCallbackTypes.hpp"

/**
 * extract front faces of mesh
 */
class CGlPeelingFrontFaces
{
	CGlViewport viewport;

public:
	CError error;						///< error messages

	CGlTexture front_texture;			///< texture for front faces
	CGlTexture front_normal_texture;	///< normal texture for front faces
	CGlTexture front_depth_texture;		///< depth texture for front faces
	CGlFbo front_fbo;					///< fbo for front faces

	GLsizei width;		///< width of peeling textures
	GLsizei height;		///< height of peeling textures

	CGlProgram cGlProgram;								///< program
	CGlUniform pvm_matrix_uniform;			///< uniform for pvm matrix
	CGlUniform view_model_matrix_uniform;		///< uniform for view-model matrix
	CGlUniform view_model_normal_matrix3_uniform;	///< uniform for view_model normal matrix

	/**
	 * initialize front and backface peeling
	 */
	CGlPeelingFrontFaces(
				GLsizei p_width,	///< width of viewport/textures
				GLsizei p_height	///< height of viewport/textures
			)	:
					front_texture(			GL_TEXTURE_2D, GL_RGBA32F, GL_RGBA, GL_FLOAT),
					front_normal_texture(	GL_TEXTURE_2D, GL_RGBA32F, GL_RGBA, GL_FLOAT),
					front_depth_texture(	GL_TEXTURE_2D, GL_DEPTH_COMPONENT, GL_DEPTH_COMPONENT, GL_FLOAT)
	{
		setup(p_width, p_height);
		
		CGlErrorCheck();
	}



	/**
	 * initialize depth peeling
	 */
	CGlPeelingFrontFaces()	:
					front_texture(			GL_TEXTURE_2D, GL_RGBA32F, GL_RGBA, GL_FLOAT),
					front_normal_texture(	GL_TEXTURE_2D, GL_RGBA32F, GL_RGBA, GL_FLOAT),
					front_depth_texture(	GL_TEXTURE_2D, GL_DEPTH_COMPONENT, GL_DEPTH_COMPONENT, GL_FLOAT)
	{
		setup(64, 64);

		CGlErrorCheck();
	}

	/**
	 * resize textures
	 */
	void resize(
					GLsizei p_width,		///< width of viewport for first pass
					GLsizei p_height		///< height of viewport for first pass
	)
	{
		/**
		 * resize first pass textures
		 */
		width = p_width;
		height = p_height;

		front_texture.bind();
		front_texture.resize(width, height);
		front_texture.unbind();

		front_normal_texture.bind();
		front_normal_texture.resize(width, height);
		front_normal_texture.unbind();

		front_depth_texture.bind();
		front_depth_texture.resize(width, height);
		front_depth_texture.unbind();

		CGlErrorCheck();
	}


	/**
	 * initialize front/back face extraction
	 */
	void setup(
				GLsizei p_width,	///< width of extraction textures
				GLsizei p_height	///< height of extraction textures
			)
	{
		CGlErrorCheck();

		resize(p_width, p_height);

		if (error())
			return;

		/*
		 * GLSL program for front pass - just pass through depth
		 */
		cGlProgram.initVertFragShadersFromDirectory("peeling_front_back_faces");

		cGlProgram.link();

		cGlProgram.setupUniform(pvm_matrix_uniform, "pvm_matrix");
		cGlProgram.setupUniform(view_model_matrix_uniform, "view_model_matrix");
		cGlProgram.setupUniform(view_model_normal_matrix3_uniform, "view_model_normal_matrix3");
		CGlErrorCheck();

		front_fbo.bind();
		front_fbo.bindTexture(front_texture, 0, 0);
		front_fbo.bindTexture(front_normal_texture, 1, 0);
		front_fbo.bindDepthTexture(front_depth_texture, 0, 0);
		front_fbo.unbind();
		CGlErrorCheck();
	}

	/**
	 * this function is called by the rendering programs to setup the matrices
	 */
	static void callback_setup_view_model_matrix(
											void *base_class,							///< pointer to this class
											const GLSL::mat4 &pvm_matrix,				///< projection,view,model matrix
											const GLSL::mat4 &view_model_matrix,			///< view-model matrix
											const GLSL::mat3 &view_model_normal_matrix3	///< normal matrix
										)
	{
		CGlPeelingFrontFaces &c = *(CGlPeelingFrontFaces*)base_class;
		c.pvm_matrix_uniform.set(pvm_matrix);
		c.view_model_matrix_uniform.set(view_model_matrix);
		c.view_model_normal_matrix3_uniform.set(view_model_normal_matrix3);
	}


	/**
	 * create depth peeling textures
	 *
	 * the callback function has to draw the objects without using a program or enabling/disabling a program!!!
	 */
	void create(	const GLSL::mat4 &projection_matrix,		///< projection matrix
					const GLSL::mat4 &view_matrix,			///< view matrix
					CGlRenderCallbackTypes::call_render_pass *callback,			///< callback to render function
					void *user_data							///< user specific data
		)
	{
		CGlErrorCheck();

		viewport.saveState();
		viewport.setSize(width, height);

		// clean with negative infinite number
		glClearColor(0, 0, -CMath::numeric_inf<float>(), 0);

		const GLenum draw_buffers_start[2] = {GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1};
		const GLenum draw_buffers_finish[2] = {GL_COLOR_ATTACHMENT0, GL_NONE};

		front_fbo.bind();
			cGlProgram.use();

			glDrawBuffers(2, draw_buffers_start);

					CGlErrorCheck();
					glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
					callback(projection_matrix, view_matrix, &callback_setup_view_model_matrix, this, user_data);

			glDrawBuffers(2, draw_buffers_finish);
			front_fbo.unbind();
			CGlErrorCheck();

			cGlProgram.disable();
		viewport.restoreState();

		CGlErrorCheck();
	}
};

#endif /* CGLDEPTHPEELING_HPP_ */
