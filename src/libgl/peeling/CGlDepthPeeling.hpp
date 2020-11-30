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
 * CGlDepthPeeling.hpp
 *
 *  Created on: Mar 9, 2010
 *      Author: martin
 */

#ifndef CGLDEPTHPEELING_HPP_
#define CGLDEPTHPEELING_HPP_

#include "libgl/core/CGlTexture.hpp"
#include "libgl/core/CGlFbo.hpp"
#include "libgl/core/CGlError.hpp"
#include "libgl/core/CGlState.hpp"
#include "libgl/core/CGlViewport.hpp"
#include "libgl/core/CGlProgram.hpp"

#include "lib/CError.hpp"

/**
 * depth peeling
 */
class CGlDepthPeeling
{
	bool first_pass_active;	///< true, if the first peeling pass is active
	bool clean;

	CGlViewport viewport;

public:
	CError error;					///< error messages

	CGlTexture peeling_texture;		///< texture stack
	CGlTexture peeling_normal_texture;	///< texture stack
	CGlTexture depth_texture;		///< depth textures

	int max_peelings;				///< maximum depth peeling textures


	CGlFbo *peeling_fbos;			///< fbo stack

	GLsizei width;	///< width of peeling textures
	GLsizei height;	///< height of peeling textures

	CGlProgram cGlProgramFirstPass;										///< program for first pass
	CGlUniform first_pass_pvm_matrix_uniform;					///< uniform for pvm matrix of used in first pass
	CGlUniform first_pass_view_model_normal_matrix3_uniform;	///< uniform for view_model normal matrix

	CGlProgram cGlProgramPeelingPass;										///< program for peeling passes
	CGlUniform peeling_pass_pvm_matrix_uniform;					///< uniform for pvm matrix of used in peeling pass
	CGlUniform peeling_pass_view_model_normal_matrix3_uniform;	///< uniform for view_model normal matrix
	CGlUniform peeling_pass_prev_depth_layer_uniform;				///< number of layer for previous depth within depth texture array



	/**
	 * initialize depth peeling
	 */
	CGlDepthPeeling(
				GLsizei p_width,		///< initial width of framebuffer texture
				GLsizei p_height,		///< initial height of framebuffer texture
				int p_max_peelings = 4	///< maximum depth peeling textures
			)	:
					peeling_texture(GL_TEXTURE_2D_ARRAY, GL_RGBA, GL_RGBA, GL_BYTE),
					peeling_normal_texture(GL_TEXTURE_2D_ARRAY, GL_RGBA, GL_RGBA, GL_BYTE),
					depth_texture(GL_TEXTURE_2D_ARRAY, GL_DEPTH_COMPONENT, GL_DEPTH_COMPONENT, GL_FLOAT)
	{
		CGlErrorCheck();

		clean = true;

		setup(p_width, p_height, p_max_peelings);
		
		CGlErrorCheck();
	}


	/**
	 * initialize depth peeling
	 */
	CGlDepthPeeling()	:
			peeling_texture(GL_TEXTURE_2D_ARRAY, GL_RGBA, GL_RGBA, GL_BYTE),
			peeling_normal_texture(GL_TEXTURE_2D_ARRAY, GL_RGBA, GL_RGBA, GL_BYTE),
			depth_texture(GL_TEXTURE_2D_ARRAY, GL_DEPTH_COMPONENT, GL_DEPTH_COMPONENT, GL_FLOAT)
	{
		clean = true;
		
		CGlErrorCheck();
	}


	/**
	 * initialize depth peeling
	 */
	void setup(
				GLsizei p_width,			///< initial width of framebuffer texture
				GLsizei p_height,			///< initial height of framebuffer texture
				int p_max_peelings = 4	///< maximum depth peeling textures
			)
	{
		cleanup();

		CGlErrorCheck();
		width = p_width;
		height = p_height;

		max_peelings = p_max_peelings;

		peeling_fbos = NULL;


		/*
		 * prepare peeling texures to store some extra data
		 */
		peeling_texture.bind();
		peeling_texture.resize(width, height, max_peelings);
		peeling_texture.unbind();
		CGlErrorCheck();

		peeling_normal_texture.bind();
		peeling_normal_texture.resize(width, height, max_peelings);
		peeling_normal_texture.unbind();
		CGlErrorCheck();

		/*
		 * prepare depth texures to store some extra data
		 */
		depth_texture.bind();
		depth_texture.resize(width, height, max_peelings);
		depth_texture.unbind();
		CGlErrorCheck();

		peeling_fbos = new CGlFbo[max_peelings];


		/*
		 * prepare FBOs
		 */
		peeling_fbos = new CGlFbo[max_peelings];

		for (int i = 0; i < max_peelings; i++)
		{
			peeling_fbos[i].bind();
			CGlErrorCheck();

			peeling_fbos[i].bindTextureLayer(peeling_texture, 0, 0, i);
			peeling_fbos[i].bindTextureLayer(peeling_normal_texture, 1, 0, i);
			peeling_fbos[i].bindDepthTextureLayer(depth_texture, 0, 0, i);	// TODO: 2 depth buffers are sufficient

			if (!peeling_fbos[i].checkStatus())
				error << "Framebuffer " << i << " for depth peeling not complete: " << peeling_fbos[i].error.getString() << CError::endl;
			peeling_fbos[i].unbind();
		}

		clean = false;

		if (error())
			return;

		/*
		 * GLSL program for first pass - just pass through depth
		 */
		cGlProgramFirstPass.initVertFragShadersFromDirectory("depth_peeling/first_pass");

		cGlProgramFirstPass.link();

		cGlProgramFirstPass.setupUniform(first_pass_pvm_matrix_uniform, "pvm_matrix");
		cGlProgramFirstPass.setupUniform(first_pass_view_model_normal_matrix3_uniform, "view_model_normal_matrix3");
		CGlErrorCheck();


		/*
		 * GLSL program for peeling pass - just pass through depth
		 */
		cGlProgramPeelingPass.initVertFragShadersFromDirectory("depth_peeling/peeling_pass");

		cGlProgramPeelingPass.link();

		cGlProgramPeelingPass.use();
		cGlProgramPeelingPass.setUniform1i("prev_depth_texture", 0);
		cGlProgramPeelingPass.disable();

		cGlProgramPeelingPass.setupUniform(peeling_pass_pvm_matrix_uniform, "pvm_matrix");
		cGlProgramPeelingPass.setupUniform(peeling_pass_view_model_normal_matrix3_uniform, "view_model_normal_matrix3");
		cGlProgramPeelingPass.setupUniform(peeling_pass_prev_depth_layer_uniform, "prev_depth_layer");
		CGlErrorCheck();
	}


	/**
	 * this function is called by the rendering programs to setup the matrices
	 */
	void callback_setup_view_model_matrix(	const GLSL::mat4 &pvm_matrix,				///< projection,view,model matrix
											const GLSL::mat3 &view_model_normal_matrix3	///< normal matrix
										)
	{
		if (first_pass_active)
		{
			first_pass_pvm_matrix_uniform.set(pvm_matrix);
			first_pass_view_model_normal_matrix3_uniform.set(view_model_normal_matrix3);
		}
		else
		{
			peeling_pass_pvm_matrix_uniform.set(pvm_matrix);
			peeling_pass_view_model_normal_matrix3_uniform.set(view_model_normal_matrix3);
		}
	}


	/**
	 * create depth peeling textures
	 *
	 * the callback function has to draw the objects without using a program or enabling/disabling a program!!!
	 */
	void create(	const GLSL::mat4 &projection_matrix,		///< projection matrix
					const GLSL::mat4 &view_matrix,			///< view matrix
					void (*callback)(const GLSL::mat4 &projection_matrix, const GLSL::mat4 &view_matrix, CGlDepthPeeling *dp, void *user_data),	///< callback to emit vertices
					void *user_data							///< user specific data
		)
	{
		CGlErrorCheck();

		viewport.saveState();
		viewport.setSize(width, height);

		// clean with minimum finite number
		glClearColor(CMath::numeric_min<float>(), CMath::numeric_min<float>(), CMath::numeric_min<float>(), CMath::numeric_min<float>());

		first_pass_active = true;

		const GLenum draw_buffers_start[2] = {GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1};

		peeling_fbos[0].bind();
		glDrawBuffers(2, draw_buffers_start);

		/**
		 * FIRST PASS
		 */
		cGlProgramFirstPass.use();

			CGlErrorCheck();
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			callback(projection_matrix, view_matrix, this, user_data);

		cGlProgramFirstPass.disable();
		peeling_fbos[0].unbind();
		CGlErrorCheck();

		first_pass_active = false;

		/**
		 * PEELING PASSES
		 */

		// disable back face culling for peeling passes
		CGlStateDisable cull_face(GL_CULL_FACE);

		cGlProgramPeelingPass.use();
		depth_texture.bind();
		for (int i = 1; i < max_peelings; i++)
		{
			peeling_pass_prev_depth_layer_uniform.set1i(i-1);

			peeling_fbos[i].bind();
			glDrawBuffers(2, draw_buffers_start);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			callback(projection_matrix, view_matrix, this, user_data);

			const GLenum draw_buffers_finish[2] = {GL_COLOR_ATTACHMENT0, GL_NONE};
			glDrawBuffers(2, draw_buffers_finish);
		}
		peeling_fbos[max_peelings-1].unbind();

		cGlProgramPeelingPass.disable();
		CGlErrorCheck();

		viewport.restoreState();

		CGlErrorCheck();
	}

	/**
	 * free unnecessary data
	 */
	void cleanup()
	{
		if (clean)	return;

		delete [] peeling_fbos;

		clean = true;
	}

	~CGlDepthPeeling()
	{
		cleanup();
	}
};

#endif /* CGLDEPTHPEELING_HPP_ */
