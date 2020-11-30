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

#ifndef CGL_DEPTHPEELING_VOXELS_HPP_
#define CGL_DEPTHPEELING_VOXELS_HPP_

#include "libgl/core/CGlTexture.hpp"
#include "libgl/core/CGlFbo.hpp"
#include "libgl/core/CGlError.hpp"
#include "libgl/core/CGlState.hpp"
#include "libgl/core/CGlViewport.hpp"

#include "lib/CError.hpp"

#include "libgl/CGlRenderCallbackTypes.hpp"

/**
 * depth peeling storing voxels
 *
 * this class creates a stack of depth peeled textures and a 'first' texture storing
 * the depth peeling with a different resolution.
 *
 * if the resolution of the 'first' texture equals to those of the peeling textures,
 * the first peeling texture equals the top texture layer of the peeling texture array.
 * otherwise, the first texture is stored scaled down to the top layer of the peeling
 * texture array.
 */
class CGlDepthPeelingVoxels
{
	int pass_number;	///< number of pass (0: first pass, 1: first peeling pass, 2: other passes)
	bool clean;

	CGlViewport viewport;

public:
	CError error;						///< error messages

	CGlTexture first_texture;			///< texture for first pass
	CGlTexture first_normal_texture;	///< normal texture for first pass
	CGlTexture first_depth_texture;		///< depth texture for first pass

	CGlTexture peeling_voxel_texture;			///< texture stack
	CGlTexture peeling_normal_texture;	///< texture stack
	CGlTexture peeling_depth_texture;	///< depth textures

	int max_peelings;				///< maximum depth peeling textures

	CGlFbo first_fbo;				///< fbo for rendering to the first texture
	CGlFbo *peeling_fbos;			///< fbo stack

	GLsizei first_pass_width;		///< width of peeling textures for first pass
	GLsizei first_pass_height;		///< height of peeling textures for first pass

	GLsizei peeling_pass_width;		///< width of peeling textures for peeling passes
	GLsizei peeling_pass_height;	///< height of peeling textures for peeling passes

	CGlProgram cGlProgramFirstPass;										///< program for first pass
	CGlUniform first_pass_pvm_matrix_uniform;					///< uniform for pvm matrix in first pass
	CGlUniform first_pass_view_model_matrix_uniform;			///< uniform for view-model matrix in first pass
	CGlUniform first_pass_view_model_normal_matrix3_uniform;	///< uniform for view_model normal matrix
/*
	CGlProgram cGlProgramFirstPeelingPass;										///< program for first peeling passe
	CGlUniform first_peeling_pass_pvm_matrix_uniform;					///< uniform for pvm matrix of used in first peeling pass
	CGlUniform first_peeling_pass_view_model_matrix_uniform;			///< uniform for view-model matrix in first peeling pass
	CGlUniform first_peeling_pass_view_model_normal_matrix3_uniform;	///< uniform for view_model normal matrix
	CGlUniform depth_texture_access_scale_uniform;					///< scaling to access depth texture from first pass
*/
	CGlProgram cGlProgramPeelingPass;										///< program for peeling passes
	CGlUniform peeling_pass_pvm_matrix_uniform;					///< uniform for pvm matrix of used in peeling pass
	CGlUniform peeling_pass_view_model_matrix_uniform;			///< uniform for view-model matrix in peeling pass
	CGlUniform peeling_pass_view_model_normal_matrix3_uniform;	///< uniform for view_model normal matrix
	CGlUniform peeling_pass_prev_depth_layer_uniform;				///< number of layer for previous depth within depth texture array



	/**
	 * initialize depth peeling
	 *
	 *
	 */
	CGlDepthPeelingVoxels(
				GLsizei p_first_pass_width,		///< width of viewport for first pass
				GLsizei p_first_pass_height,	///< height of viewport for first pass
				GLsizei p_peeling_pass_width,	///< width of viewport for peeling pass
				GLsizei p_peeling_pass_height,	///< height of viewport for peeling pass
				int p_max_peelings = 4	///< maximum depth peeling textures
			)	:
					first_texture(			GL_TEXTURE_2D, GL_RGBA32F, GL_RGBA, GL_FLOAT),
					first_normal_texture(	GL_TEXTURE_2D, GL_RGBA32F, GL_RGBA, GL_FLOAT),
					first_depth_texture(	GL_TEXTURE_2D, GL_DEPTH_COMPONENT, GL_DEPTH_COMPONENT, GL_FLOAT),

					peeling_voxel_texture(		GL_TEXTURE_2D_ARRAY, GL_RGBA32F, GL_RGBA, GL_FLOAT),
					peeling_normal_texture(	GL_TEXTURE_2D_ARRAY, GL_RGBA32F, GL_RGBA, GL_FLOAT),
					peeling_depth_texture(	GL_TEXTURE_2D_ARRAY, GL_DEPTH_COMPONENT, GL_DEPTH_COMPONENT, GL_FLOAT)
	{
		clean = true;
		setup(p_first_pass_width, p_first_pass_height, p_peeling_pass_width, p_peeling_pass_height, p_max_peelings);
		
		CGlErrorCheck();
	}



	/**
	 * initialize depth peeling
	 *
	 *
	 */
	CGlDepthPeelingVoxels(
				int p_max_peelings = 4	///< maximum depth peeling textures
			)	:
					first_texture(			GL_TEXTURE_2D, GL_RGBA32F, GL_RGBA, GL_FLOAT),
					first_normal_texture(	GL_TEXTURE_2D, GL_RGBA32F, GL_RGBA, GL_FLOAT),
					first_depth_texture(	GL_TEXTURE_2D, GL_DEPTH_COMPONENT, GL_DEPTH_COMPONENT, GL_FLOAT),

					peeling_voxel_texture(		GL_TEXTURE_2D_ARRAY, GL_RGBA32F, GL_RGBA, GL_FLOAT),
					peeling_normal_texture(	GL_TEXTURE_2D_ARRAY, GL_RGBA32F, GL_RGBA, GL_FLOAT),
					peeling_depth_texture(	GL_TEXTURE_2D_ARRAY, GL_DEPTH_COMPONENT, GL_DEPTH_COMPONENT, GL_FLOAT)
	{
		clean = true;
		setup(64, 64, 64, 64, p_max_peelings);

		CGlErrorCheck();
	}

	/**
	 * resize first textures
	 */
	void resizeViewport(
					GLsizei p_first_pass_width,		///< width of viewport for first pass
					GLsizei p_first_pass_height		///< height of viewport for first pass
	)
	{
		/**
		 * resize first pass textures
		 */
		first_pass_width = p_first_pass_width;
		first_pass_height = p_first_pass_height;

		first_texture.bind();
		first_texture.resize(first_pass_width, first_pass_height);
		first_texture.unbind();

		first_normal_texture.bind();
		first_normal_texture.resize(first_pass_width, first_pass_height);
		first_normal_texture.unbind();

		first_depth_texture.bind();
		first_depth_texture.resize(first_pass_width, first_pass_height);
		first_depth_texture.unbind();
	}


	/**
	 * resize peeling textures
	 */
	void resizePeelingTextures(
					GLsizei p_peeling_pass_width,	///< width of viewport for peeling pass
					GLsizei p_peeling_pass_height	///< height of viewport for peeling pass
	)
	{
		/**
		 * resize peeling pass textures
		 */
		peeling_pass_width = p_peeling_pass_width;
		peeling_pass_height = p_peeling_pass_height;

		peeling_voxel_texture.bind();
		peeling_voxel_texture.resize(peeling_pass_width, peeling_pass_height, max_peelings);
		peeling_voxel_texture.unbind();

		peeling_normal_texture.bind();
		peeling_normal_texture.resize(peeling_pass_width, peeling_pass_height, max_peelings);
		peeling_normal_texture.unbind();

		peeling_depth_texture.bind();
		peeling_depth_texture.resize(peeling_pass_width, peeling_pass_height, max_peelings);
		peeling_depth_texture.unbind();
	}


	/**
	 * resize textures (e.g. if viewport is resized)
	 */
	void resize(
				GLsizei p_first_pass_width,		///< width of viewport for first pass
				GLsizei p_first_pass_height,	///< height of viewport for first pass

				GLsizei p_peeling_pass_width,	///< width of viewport for peeling pass
				GLsizei p_peeling_pass_height	///< height of viewport for peeling pass
			)
	{
		resizeViewport(p_first_pass_width, p_first_pass_height);
		resizePeelingTextures(p_peeling_pass_width, p_peeling_pass_height);
	}


	/**
	 * initialize depth peeling
	 */
	void setup(
				GLsizei p_first_pass_width,		///< width of viewport for first pass
				GLsizei p_first_pass_height,	///< height of viewport for first pass

				GLsizei p_peeling_pass_width,	///< width of viewport for peeling pass
				GLsizei p_peeling_pass_height,	///< height of viewport for peeling pass

				int p_max_peelings = 4	///< maximum depth peeling textures
			)
	{
		cleanup();

		CGlErrorCheck();
		max_peelings = p_max_peelings;

		resize(p_first_pass_width, p_first_pass_height, p_peeling_pass_width, p_peeling_pass_height);


		/*
		 * prepare FBO for first rendering
		 */
		first_fbo.bind();
		first_fbo.bindTexture(first_texture, 0);
		first_fbo.bindTexture(first_normal_texture, 1);
		first_fbo.bindDepthTexture(first_depth_texture);
		first_fbo.unbind();

		/*
		 * prepare FBOs for peeling renderings
		 */
		peeling_fbos = new CGlFbo[max_peelings];

		for (int i = 0; i < max_peelings; i++)
		{
			peeling_fbos[i].bind();
			CGlErrorCheck();

			peeling_fbos[i].bindTextureLayer(peeling_voxel_texture, 0, 0, i);
			peeling_fbos[i].bindTextureLayer(peeling_normal_texture, 1, 0, i);
			peeling_fbos[i].bindDepthTextureLayer(peeling_depth_texture, 0, 0, i);

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
		cGlProgramFirstPass.initVertFragShadersFromDirectory("depth_peeling_voxels/first_pass");

		cGlProgramFirstPass.link();

		cGlProgramFirstPass.setupUniform(first_pass_pvm_matrix_uniform, "pvm_matrix");
		cGlProgramFirstPass.setupUniform(first_pass_view_model_matrix_uniform, "view_model_matrix");
		cGlProgramFirstPass.setupUniform(first_pass_view_model_normal_matrix3_uniform, "view_model_normal_matrix3");
		CGlErrorCheck();

#if 0
		/*
		 * GLSL program for first peeling pass
		 */
		cGlProgramFirstPeelingPass.initVertFragShadersFromDirectory("depth_peeling_voxels/first_peeling_pass");
		CError_AppendReturn(cGlProgramFirstPeelingPass);

		cGlProgramFirstPeelingPass.link();
		if (cGlProgramFirstPeelingPass.error())
		{
			error << "info Log: linking: " << cGlProgramFirstPeelingPass.getInfoLog() << std::endl;
			return;
		}

		cGlProgramFirstPeelingPass.use();
		cGlProgramFirstPeelingPass.setUniform1i("prev_depth_texture", 0);
		cGlProgramFirstPeelingPass.disable();

		first_peeling_pass_pvm_matrix_uniform.init(cGlProgramFirstPeelingPass, "pvm_matrix");
		first_peeling_pass_view_model_matrix_uniform.init(cGlProgramFirstPeelingPass, "view_model_matrix");
		first_peeling_pass_view_model_normal_matrix3_uniform.init(cGlProgramFirstPeelingPass, "view_model_normal_matrix3");
		depth_texture_access_scale_uniform.init(cGlProgramFirstPeelingPass, "depth_texture_access_scale");
		CGlErrorCheck();
#endif

		/*
		 * GLSL program for peeling pass - just pass through depth
		 */
		cGlProgramPeelingPass.initVertFragShadersFromDirectory("depth_peeling_voxels/peeling_pass");

		cGlProgramPeelingPass.link();

		cGlProgramPeelingPass.use();
		cGlProgramPeelingPass.setUniform1i("prev_depth_texture", 0);
		cGlProgramPeelingPass.disable();

		cGlProgramPeelingPass.setupUniform(peeling_pass_pvm_matrix_uniform, "pvm_matrix");
		cGlProgramPeelingPass.setupUniform(peeling_pass_view_model_matrix_uniform, "view_model_matrix");
		cGlProgramPeelingPass.setupUniform(peeling_pass_view_model_normal_matrix3_uniform, "view_model_normal_matrix3");
		cGlProgramPeelingPass.setupUniform(peeling_pass_prev_depth_layer_uniform, "prev_depth_layer");
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
		CGlDepthPeelingVoxels &c = *(CGlDepthPeelingVoxels*)base_class;

		switch(c.pass_number)
		{
			case 0:
				c.first_pass_pvm_matrix_uniform.set(pvm_matrix);
				c.first_pass_view_model_matrix_uniform.set(view_model_matrix);
				c.first_pass_view_model_normal_matrix3_uniform.set(view_model_normal_matrix3);
				break;
#if 0
			case 1:
				c.first_peeling_pass_pvm_matrix_uniform.set(pvm_matrix);
				c.first_peeling_pass_view_model_matrix_uniform.set(view_model_matrix);
				c.first_peeling_pass_view_model_normal_matrix3_uniform.set(view_model_normal_matrix3);
				break;
#endif
			case 2:
				c.peeling_pass_pvm_matrix_uniform.set(pvm_matrix);
				c.peeling_pass_view_model_matrix_uniform.set(view_model_matrix);
				c.peeling_pass_view_model_normal_matrix3_uniform.set(view_model_normal_matrix3);
				break;
		}
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
		viewport.setSize(first_pass_width, first_pass_height);

		// clean with negative infinite number
		glClearColor(0, 0, -CMath::numeric_inf<float>(), 0);


		const GLenum draw_buffers_start[2] = {GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1};
		const GLenum draw_buffers_finish[2] = {GL_COLOR_ATTACHMENT0, GL_NONE};


		/**
		 * FIRST PASS
		 */
		pass_number = 0;

		first_fbo.bind();
		glDrawBuffers(2, draw_buffers_start);
			cGlProgramFirstPass.use();

				CGlErrorCheck();
				glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
				callback(projection_matrix, view_matrix, &callback_setup_view_model_matrix, this, user_data);

			cGlProgramFirstPass.disable();
		glDrawBuffers(2, draw_buffers_finish);
		first_fbo.unbind();

		CGlErrorCheck();


		/**
		 * FIRST PEELING TEXTURE PASS
		 * (same as first pass, but with different resolution)
		 *
		 * TODO: copy from 'first' texture, if texture resolutions equal
		 */
		viewport.setSize(peeling_pass_width, peeling_pass_height);
		pass_number = 0;

		peeling_fbos[0].bind();
		glDrawBuffers(2, draw_buffers_start);
			cGlProgramFirstPass.use();

				CGlErrorCheck();
				glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
				callback(projection_matrix, view_matrix, &callback_setup_view_model_matrix, this, user_data);

			cGlProgramFirstPass.disable();
		glDrawBuffers(2, draw_buffers_finish);
		peeling_fbos[0].unbind();

		CGlErrorCheck();


		/**
		 * REMAINING PEELING PASSES
		 */
		{
			CGlStateDisable cull_face(GL_CULL_FACE);

			pass_number = 2;

			cGlProgramPeelingPass.use();
			peeling_depth_texture.bind();
			for (int i = 1; i < max_peelings; i++)
			{
				peeling_pass_prev_depth_layer_uniform.set1i(i-1);

				peeling_fbos[i].bind();
				glDrawBuffers(2, draw_buffers_start);
				glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

				callback(projection_matrix, view_matrix, &callback_setup_view_model_matrix, this, user_data);

				glDrawBuffers(2, draw_buffers_finish);
			}
			peeling_fbos[max_peelings-1].unbind();

			cGlProgramPeelingPass.disable();
			CGlErrorCheck();
		}

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
		peeling_fbos = NULL;

		clean = true;
	}

	~CGlDepthPeelingVoxels()
	{
		cleanup();
	}
};

#endif /* CGLDEPTHPEELING_HPP_ */
